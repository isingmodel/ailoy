#pragma once

#include <minja/chat-template.hpp>
#include <nlohmann/json.hpp>

#include "module.hpp"

namespace ailoy {

class chat_template_engine_t : public object_t {
public:
  chat_template_engine_t(const std::string &chat_template,
                         const std::string &bos_token,
                         const std::string &eos_token,
                         const std::string &botc_token = "",
                         const std::string &eotc_token = "")
      : template_(std::make_unique<minja::chat_template>(chat_template,
                                                         bos_token, eos_token)),
        bos_token_(bos_token), eos_token_(eos_token), botc_token_(botc_token),
        eotc_token_(eotc_token) {}

  static std::shared_ptr<chat_template_engine_t>
  make_from_config_file(std::filesystem::path config_file_path);

  const std::string
  apply_chat_template(const nlohmann::json &messages,
                      const nlohmann::json &tools = {},
                      const bool enable_reasoning = false,
                      const bool add_generation_prompt = true);

  const std::string &get_bos_token() const { return bos_token_; }

  const std::string &get_eos_token() const { return eos_token_; }

  const std::string &get_botc_token() const { return botc_token_; }

  const std::string &get_eotc_token() const { return eotc_token_; }

  bool is_bos_token(const std::string &token) const {
    return token == bos_token_;
  }

  bool is_eos_token(const std::string &token) const {
    return token == eos_token_;
  }

  bool is_botc_token(const std::string &token) const {
    return token == botc_token_;
  }

  bool is_eotc_token(const std::string &token) const {
    return token == eotc_token_;
  }

  const std::optional<std::string>
  get_json_str_if_valid(const std::vector<std::string> &tokens);

private:
  std::unique_ptr<minja::chat_template> template_;
  const std::string bos_token_;
  const std::string eos_token_;
  const std::string botc_token_;
  const std::string eotc_token_;
};

} // namespace ailoy
