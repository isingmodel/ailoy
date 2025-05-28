#include <gtest/gtest.h>

#include "mlc_llm/language_model_v2.hpp"

#include <iostream>

TEST(TestLangModelV2, TestTemp) {
  ailoy::temp();
  std::cout << "done" << std::endl;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
