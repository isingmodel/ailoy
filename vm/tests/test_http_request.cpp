#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "http_request.hpp"

TEST(HttpRequestTest, Get_Frankfurter) {
  auto resp = ailoy::run_http_request(ailoy::http_request_t{
      .url = "https://api.frankfurter.dev/v1/latest?base=USD&symbols=KRW",
      .method = "GET",
      .headers = {},
  });
  ASSERT_EQ(resp.status_code, 200);

  auto j = nlohmann::json::parse(resp.body);
  // body is like:
  // {"amount":1.0,"base":"USD","date":"2025-04-17","rates":{"KRW":1416.48}}
  ASSERT_EQ(j["amount"], 1.0);
  ASSERT_EQ(j["base"], "USD");
  ASSERT_EQ(j["rates"].contains("KRW"), true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
