#include <gtest/gtest.h>

#include "mlc_llm/language_model_v2.hpp"
#include "mlc_llm/tvm_model.hpp"
#include "value.hpp"

#include <iostream>

std::shared_ptr<ailoy::component_t> get_model() {
  static std::shared_ptr<ailoy::component_t> model;
  if (!model) {
    auto in = ailoy::create<ailoy::map_t>();
    in->insert_or_assign("model",
                         ailoy::create<ailoy::string_t>("Qwen/Qwen3-0.6B"));
    auto model_opt = ailoy::create_tvm_language_model_v2_component(in);
    if (model_opt.index() != 0)
      return nullptr;
    model = std::get<0>(model_opt);
  }
  return model;
}

std::string
infer(std::shared_ptr<ailoy::component_t> model,
      std::shared_ptr<ailoy::value_t> messages,
      std::shared_ptr<ailoy::value_t> tools = ailoy::create<ailoy::array_t>(),
      bool enable_reasoning = false) {
  auto in = ailoy::create<ailoy::map_t>();
  in->insert_or_assign("messages", messages);
  if (!tools->as<ailoy::array_t>()->empty())
    in->insert_or_assign("tools", tools);
  in->insert_or_assign("temperature", ailoy::create<ailoy::double_t>(0.));
  in->insert_or_assign("top_p", ailoy::create<ailoy::double_t>(0.));
  auto init_out_opt = model->get_operator("infer")->initialize(in);
  if (init_out_opt.has_value())
    return init_out_opt.value().reason;
  std::string agg_out = "";
  while (true) {
    ailoy::output_t out_opt = model->get_operator("infer")->step();
    if (out_opt.index() != 0)
      return std::get<1>(out_opt).reason;
    auto out = std::get<0>(out_opt);
    auto resp = out.val->as<ailoy::map_t>();
    if (resp->at("content")->is_type_of<ailoy::string_t>())
      agg_out += *resp->at<ailoy::string_t>("content");
    else
      agg_out += resp->at("content")
                     ->encode(ailoy::encoding_method_t::json)
                     ->operator std::string();
    if (out.finish)
      break;
  }
  return agg_out;
}

TEST(TestLangModelV2, TestSimple) {
  auto messages_str = R"([
  {"role": "user", "content":  "Introduce yourself"}
])";

  std::shared_ptr<ailoy::component_t> model = get_model();
  auto messages = ailoy::decode(messages_str, ailoy::encoding_method_t::json);
  auto out = infer(model, messages);
  ASSERT_EQ(
      out,
      R"(Hello! I'm an AI assistant, and I'm here to help you with your questions. If you have any questions or need assistance, feel free to ask me. I'm always happy to be a helpful and friendly companion! 😊)");
}

TEST(TestLangModelV2, TestMultiTurn) {
  auto messages_str1 = R"([
  {"role": "system", "content": "You are Qwen, created by Alibaba Cloud. You are a helpful assistant."},
  {"role": "user", "content":  "Who made you? Answer it simply."}
])";
  std::shared_ptr<ailoy::component_t> model = get_model();
  auto messages1 = ailoy::decode(messages_str1, ailoy::encoding_method_t::json);
  auto answer1 = infer(model, messages1);
  ASSERT_EQ(answer1, "I was created by Alibaba Cloud.");

  auto messages_str2 = R"([
  {"role": "system", "content": "You are Qwen, created by Alibaba Cloud. You are a helpful assistant."},
  {"role": "user", "content":  "Who made you? Answer it simply."},
  {"role": "assistant", "content":  "I was created by Alibaba Cloud."},
  {"role": "user", "content":  "Based on your first answer, describe the company simply."}
])";
  auto messages2 = ailoy::decode(messages_str2, ailoy::encoding_method_t::json);
  auto answer2 = infer(model, messages2);
  ASSERT_EQ(
      answer2,
      R"(Alibaba Cloud is a leading technology company in the cloud computing and software development sectors. We focus on providing innovative and scalable solutions to businesses and individuals.)");
}

TEST(TestLangModelV2, TestGrammar) {
  auto messages_str = R"([
  {"role": "system", "content": "You are Qwen, created by Alibaba Cloud. You are a helpful assistant."},
  {"role": "user", "content": "Hello, how is the current weather in my city Seoul?"},
  {"role": "assistant", "content": "I can certainly help with that! Would you like to know the current temperature or the current wind speed in Seoul, South Korea?"},
  {"role": "user", "content": "Yes, both please!"}
])";
  const std::string tools_str = R"([
  {"type": "function", "function": {"name": "get_current_temperature", "description": "Get the current temperature at a location.", "parameters": {"type": "object", "properties": {"location": {"type": "string", "description": "The location to get the temperature for, in the format \"City, Country\""}, "unit": {"type": "string", "enum": ["celsius", "fahrenheit"], "description": "The unit to return the temperature in."}}, "required": ["location", "unit"]}, "return": {"type": "number", "description": "The current temperature at the specified location in the specified units, as a float."}}},
  {"type": "function", "function": {"name": "get_current_wind_speed", "description": "Get the current wind speed in km/h at a given location.", "parameters": {"type": "object", "properties": {"location": {"type": "string", "description": "The location to get the temperature for, in the format \"City, Country\""}}, "required": ["location"]}, "return": {"type": "number", "description": "The current wind speed at the given location in km/h, as a float."}}}
])";

  std::shared_ptr<ailoy::component_t> model = get_model();
  auto messages = ailoy::decode(messages_str, ailoy::encoding_method_t::json);
  auto tools = ailoy::decode(tools_str, ailoy::encoding_method_t::json);

  // model->set_builtin_grammar("tool_call", "json");
  auto answer = infer(model, messages, tools);
  // std::cout << answer << std::endl;
  // model->reset_grammar("tool_call");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
