#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <tvm/runtime/ndarray.h>
#include <tvm/runtime/packed_func.h>

#include "exception.hpp"
#include "object.hpp"

namespace ailoy {

struct tvm_model_t;

struct chat_template_engine_t;

struct tokenizer_t;

struct kv_cache_t;

struct tokenizer_info_t;

struct grammar_t;

struct grammar_matcher_t;

struct context_length_limit {
  context_length_limit() = default;

  static constexpr const char *what() noexcept {
    return "Context length limit exceeded.";
  }
};

static_assert(is_exception_reason<context_length_limit>);

class tvm_language_model_t : public object_t {
public:
  struct config_t {
    double temperature;
    double top_p;
  };

  /**
   * The term "stream mode" refers to a way of indicating that the model is in a
   * specific state during decoding. For example, when a `<tool_call>` token is
   * generated during inference, we can assume that the model is about to begin
   * generating a (formatted) tool calling request, starting from the next
   * token. Same behavior can also be applied to <reasoning> token or even
   * user-defined patterns. Stream mode serves as a marker for this state.
   *
   * Stream mode is useful for restricting the output format. When the model is
   * in "tool calling mode", its output should conform to a predefined schema.
   * This can be enforced by applying a corresponding grammar.
   */
  struct stream_mode_t {
    stream_mode_t(tvm_language_model_t *model,
                  const std::string &open_indicator,
                  const std::string &close_indicator);

    bool check_indecator(const std::string &indicator_type,
                         const std::vector<int32_t> &history) const;

    std::vector<int32_t> open_indicator;

    std::vector<int32_t> close_indicator;

    /**
     * 해당 stream mode에 적용되는 grammar, model에서 set_grammar 함수에 의해
     * 설정됨
     */
    std::shared_ptr<grammar_t> grammar;

    /**
     * Model이 grammar 적용 모드로 들어갈 때 생성됨
     */
    std::shared_ptr<grammar_matcher_t> matcher;
  };

  /**
   * Constructor
   */
  tvm_language_model_t(const std::string &model,
                       const std::string &quantization, DLDevice device);

  void clear();

  std::string apply_chat_template(const nlohmann::json &messages,
                                  const nlohmann::json &tools = {},
                                  bool enable_reasoning = false,
                                  bool add_generation_prompt = true) const;

  /** Begin of reasoning */
  bool is_bor(const std::string &tok) const;

  bool is_bor(int32_t tok) const;

  /** End of reasoning */
  bool is_eor(const std::string &tok) const;

  bool is_eor(int32_t tok) const;

  bool is_bos(const std::string &tok) const;

  bool is_eos(const std::string &tok) const;

  bool is_botc(const std::string &tok) const;

  bool is_botc(int32_t tok) const;

  bool is_eotc(const std::string &tok) const;

  bool is_eotc(int32_t tok) const;

  std::vector<int32_t> tokenize(const std::string &prompt) const;

  int32_t prefill(const std::vector<int32_t> &tokens);

  int32_t decode(int32_t last_token);

  std::optional<std::string> detokenize(int32_t token);

  const stream_mode_t &get_mode(std::string mode_name) const;

  void add_mode(std::string mode_name, const std::string &open_indicator,
                const std::string &close_indicator);

  void remove_mode(std::string mode_name);

  std::shared_ptr<grammar_matcher_t> get_current_grammar_matcher();

  void set_builtin_grammar(const std::string &mode_name,
                           const std::string &grammar_type);

  void set_json_schema_grammar(const std::string &mode_name,
                               const std::string &json_schema);

  void set_regex_grammar(const std::string &mode_name,
                         const std::string &regex);

  void set_ebnf_grammar(const std::string &mode_name, const std::string &ebnf);

  void reset_grammar(const std::string &mode_name);

  config_t config;

  const config_t &get_default_config() const { return default_config_; }

private:
  std::shared_ptr<tvm_model_t> model_;

  std::shared_ptr<chat_template_engine_t> template_engine_;

  std::shared_ptr<tokenizer_t> tokenizer_;

  std::shared_ptr<kv_cache_t> kv_cache_;

  std::shared_ptr<tokenizer_info_t> tokenizer_info_;

  config_t default_config_;

  std::vector<int32_t> history_;

  std::vector<int32_t> output_stream_;

  std::string current_stream_mode_;

  std::unordered_map<std::string, stream_mode_t> stream_modes_;

  tvm::runtime::PackedFunc fembed_;

  tvm::runtime::PackedFunc fprefill_;

  tvm::runtime::PackedFunc fdecode_;

  tvm::runtime::PackedFunc fapply_bitmask_inplace_;

  tvm::runtime::PackedFunc fsample_top_p_from_logits_;
};

} // namespace ailoy
