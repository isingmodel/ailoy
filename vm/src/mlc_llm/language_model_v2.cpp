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
    fkv_state_begin_forward_ =
        engine->get_vm_function("vm.builtin.kv_state_begin_forward");
    fkv_state_end_forward_ =
        engine->get_vm_function("vm.builtin.kv_state_end_forward");
  }

  void begin_forward(const std::vector<int64_t> &token_ids) {
    fkv_state_begin_forward_(kv_cache_, IntTuple{token_ids},
                             IntTuple{static_cast<int>(token_ids.size())});
  }

  void begin_forward(const std::vector<int> &token_ids) {
    std::vector<int64_t> token_ids_i64;
    token_ids_i64.reserve(token_ids.size());
    for (size_t i = 0; i < token_ids.size(); i++)
      token_ids_i64.push_back(token_ids[i]);
    begin_forward(token_ids_i64);
  }

  void step_forward() {}

  void end_forward() { fkv_state_end_forward_(kv_cache_); }

private:
  ObjectRef kv_cache_;

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
  auto fn = engine->get_vm_function("create_tir_paged_kv_cache");
  ObjectRef kv_cache = fn(
      IntTuple{1}, // max_num_sequence
      IntTuple{engine->get_metadata()["context_window_size"].operator int()},
      IntTuple{engine->get_metadata()["prefill_chunk_size"].operator int()},
      IntTuple{16}, // page size
      IntTuple{engine->get_metadata()["sliding_window_size"].operator int() !=
               -1});

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
}

} // namespace ailoy
