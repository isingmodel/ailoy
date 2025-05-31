#include <gtest/gtest.h>

#include "mlc_llm/language_model_v2.hpp"
#include "mlc_llm/tvm_model.hpp"

#include <iostream>

std::shared_ptr<ailoy::tvm_language_model_t> get_model() {
  auto device = ailoy::get_tvm_device(0);
  if (!device.has_value())
    return nullptr;
  static auto model = ailoy::create<ailoy::tvm_language_model_t>(
      "Qwen/Qwen3-0.6B", "q4f16_1", device.value());
  model->config.temperature = 0.;
  model->config.top_p = 0.;
  return model;
}

std::string infer(std::shared_ptr<ailoy::tvm_language_model_t> model,
                  nlohmann::json messages, const nlohmann::json &tools = {},
                  bool enable_reasoning = false) {
  std::string output_agg = "";
  auto prompt = model->apply_chat_template(messages, tools, enable_reasoning);
  auto tokens = model->tokenize(prompt);
  auto out_token = model->prefill(tokens);

  while (true) {
    out_token = model->decode(out_token);
    auto out = model->detokenize(out_token);
    if (out.has_value())
      if (model->is_eos(out.value()))
        break;
    if (out.has_value())
      output_agg += out.value();
  }
  return output_agg;
}

TEST(TestLangModelV2, TestSimple) {
  auto model = get_model();

  // Example input
  nlohmann::json messages = nlohmann::json::array();
  {
    auto object = nlohmann::json::object();
    object["role"] = "user";
    object["content"] = "Introduce yourself";
    messages.push_back(object);
  }

  auto answer = infer(model, messages);

  ASSERT_EQ(
      answer,
      R"(Hello! I'm an AI assistant, and I'm here to help you with your questions. If you have any questions or need assistance, feel free to ask me. I'm always happy to be a helpful and friendly companion! 😊)");

  model->clear();
}

TEST(TestLangModelV2, TestMultiTurn) {
  auto model = get_model();

  // Example input
  nlohmann::json messages = nlohmann::json::array();
  {
    auto object = nlohmann::json::object();
    object["role"] = "system";
    object["content"] =
        "You are Qwen, created by Alibaba Cloud. You are a helpful assistant.";
    messages.push_back(object);
  }
  {
    auto object = nlohmann::json::object();
    object["role"] = "user";
    object["content"] = "Who made you? Answer it simply.";
    messages.push_back(object);
  }
  auto answer1 = infer(model, messages);
  ASSERT_EQ(answer1, "I was created by Alibaba Cloud.");
  {
    auto object = nlohmann::json::object();
    object["role"] = "assistant";
    object["content"] = answer1;
    messages.push_back(object);
  }
  {
    auto object = nlohmann::json::object();
    object["role"] = "user";
    object["content"] =
        "Based on your first answer, describe the company simply.";
    messages.push_back(object);
  }
  auto answer2 = infer(model, messages);
  ASSERT_EQ(
      answer2,
      R"(Alibaba Cloud is a leading technology company in the cloud computing and software development sectors. We focus on providing innovative and scalable solutions to businesses and individuals.)");
}

TEST(TestLangModelV2, TestGrammar) {
  auto model = get_model();
  nlohmann::json messages = nlohmann::json::array();
  nlohmann::json tools = nlohmann::json::array();

  {
    static const std::string message_str = R"([
  {"role": "system", "content": "You are Qwen, created by Alibaba Cloud. You are a helpful assistant."},
  {"role": "user", "content": "Hello, how is the current weather in my city Seoul?"},
  {"role": "assistant", "content": "I can certainly help with that! Would you like to know the current temperature or the current wind speed in Seoul, South Korea?"},
  {"role": "user", "content": "Yes, both please!"}
])";
    messages = nlohmann::json::parse(message_str);
  }

  {
    const std::string tools_str = R"([
  {"type": "function", "function": {"name": "get_current_temperature", "description": "Get the current temperature at a location.", "parameters": {"type": "object", "properties": {"location": {"type": "string", "description": "The location to get the temperature for, in the format \"City, Country\""}, "unit": {"type": "string", "enum": ["celsius", "fahrenheit"], "description": "The unit to return the temperature in."}}, "required": ["location", "unit"]}, "return": {"type": "number", "description": "The current temperature at the specified location in the specified units, as a float."}}},
  {"type": "function", "function": {"name": "get_current_wind_speed", "description": "Get the current wind speed in km/h at a given location.", "parameters": {"type": "object", "properties": {"location": {"type": "string", "description": "The location to get the temperature for, in the format \"City, Country\""}}, "required": ["location"]}, "return": {"type": "number", "description": "The current wind speed at the given location in km/h, as a float."}}}
])";
    tools = nlohmann::json::parse(tools_str);
  }

  model->set_builtin_grammar("tool_call", "json");
  auto answer = infer(model, messages, tools);
  model->reset_grammar("tool_call");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
