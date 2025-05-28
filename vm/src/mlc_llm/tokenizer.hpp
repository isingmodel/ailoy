#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "object.hpp"

namespace ailoy {

class tokenizer_t : public object_t {
  using token_t = int32_t;

public:
  tokenizer_t(const std::filesystem::path &json_file_path);

  ~tokenizer_t();

  std::vector<token_t> encode(const std::string &text,
                              bool add_special_token = true);

  std::string decode(const std::vector<token_t> &ids,
                     bool skip_special_tokens = true);

private:
  /**
   * @brief tokenizer handle
   */
  void *handle_;
};

} // namespace ailoy
