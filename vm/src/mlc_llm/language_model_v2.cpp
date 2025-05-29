#include "language_model_v2.hpp"

#include <dlpack/dlpack.h>
#include <nlohmann/json.hpp>

#include "chat_template_engine.hpp"
#include "model_cache.hpp"
#include "tokenizer.hpp"
#include "tvm_model.hpp"

#include <iostream>

using namespace tvm;
using namespace tvm::runtime;

namespace ailoy {

class kv_cache_t {
public:
  kv_cache_t(std::shared_ptr<ailoy::tvm_model_t> engine) {
    auto fn = engine->get_vm_function("create_tir_paged_kv_cache");
    kv_cache_ = fn(
        IntTuple{1}, // max_num_sequence
        IntTuple{engine->get_metadata()["context_window_size"].operator int()},
        IntTuple{engine->get_metadata()["prefill_chunk_size"].operator int()},
        IntTuple{16}, // page size
        IntTuple{engine->get_metadata()["sliding_window_size"].operator int() !=
                 -1});
    fkv_state_add_sequence_ =
        engine->get_function("vm.builtin.kv_state_add_sequence");
    fkv_state_remove_sequence_ =
        engine->get_function("vm.builtin.kv_state_remove_sequence");
    fkv_state_fork_sequence_ =
        engine->get_function("vm.builtin.kv_state_fork_sequence");
    fkv_state_begin_forward_ =
        engine->get_function("vm.builtin.kv_state_begin_forward");
    fkv_state_end_forward_ =
        engine->get_function("vm.builtin.kv_state_end_forward");
  }

  ObjectRef get() { return kv_cache_; }

  void add_sequence(size_t sequence_id) {
    fkv_state_add_sequence_(kv_cache_, sequence_id);
  }

  void remove_sequence(size_t sequence_id) {
    fkv_state_remove_sequence_(kv_cache_, sequence_id);
  }

  void begin_forward(size_t sequence_id, size_t sequence_length) {
    fkv_state_begin_forward_(kv_cache_,
                             IntTuple{static_cast<int32_t>(sequence_id)},
                             IntTuple{static_cast<int32_t>(sequence_length)});
  }

  void end_forward() { fkv_state_end_forward_(kv_cache_); }

private:
  ObjectRef kv_cache_;

  PackedFunc fkv_state_add_sequence_;

  PackedFunc fkv_state_fork_sequence_;

  PackedFunc fkv_state_remove_sequence_;

  PackedFunc fkv_state_begin_forward_;

  PackedFunc fkv_state_end_forward_;
};

void temp() {
  auto engine =
      create<tvm_model_t>("Qwen/Qwen3-0.6B", "q4f16_1",
                          DLDevice{.device_type = kDLMetal, .device_id = 0});

  auto template_engine = chat_template_engine_t::make_from_config_file(
      engine->get_model_path() / "chat-template-config.json");

  auto tokenizer =
      create<tokenizer_t>(engine->get_model_path() / "tokenizer.json");

  // Create kv-cache
  kv_cache_t kv_cache{engine};

  // Example input
  nlohmann::json messages = nlohmann::json::array();
  {
    auto object = nlohmann::json::object();
    object["role"] = "user";
    object["content"] = "Introduce yourself";
    messages.push_back(object);
  }

  // Apply chat template
  auto templated_input = template_engine->apply_chat_template(messages);

  // Tokenization
  auto tokens = tokenizer->encode(templated_input);

  int32_t tokens_length = tokens.size();
  DLDataType I32 = DLDataType{.code = kDLInt, .bits = 32, .lanes = 1};
  DLDataType F32 = DLDataType{.code = kDLFloat, .bits = 32, .lanes = 1};

  NDArray token_ids =
      NDArray::Empty({tokens_length}, I32, engine->get_device());
  token_ids.CopyFromBytes(tokens.data(),
                          tokens.size() * sizeof(decltype(tokens)::value_type));

  NDArray embed =
      engine->get_vm_function("embed")(token_ids, engine->get_params());
  NDArray embed_reshaped = embed.CreateView(
      ShapeTuple{1, embed->shape[0], embed->shape[1]}, embed.DataType());
  std::cout << embed.Shape() << std::endl;

  kv_cache.add_sequence(0);

  kv_cache.begin_forward(0, tokens_length);
  engine->get_vm_function("prefill")(embed_reshaped, kv_cache.get(),
                                     engine->get_params());
  kv_cache.end_forward();

  // {
  //   tvm::IntTuple sequence_ids_tuple{1L};
  //   tvm::IntTuple input_length_shape{1};
  //   kv_cache.begin_forward(0, tokens_length);
  //   tvm::runtime::NDArray embed = fembed_(inputs, engine->get_params());
  //   tvm::runtime::ObjectRef return_value;
  //   tvm::runtime::NDArray embed_reshaped = embed.CreateView(
  //       tvm::ShapeTuple{1, 1, embed->shape[1]}, embed.DataType());
  //   return_value =
  //       fdecode_(embed_reshaped, kv_cache.get(), engine->get_params());
  //   kv_cache.end_forward();
  // }

  kv_cache.remove_sequence(0);
}

} // namespace ailoy
