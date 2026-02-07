#include "EdAsmNg/app.hpp"

#include <gtest/gtest.h>

TEST(GreetTests, DefaultsToWorld) {
  EXPECT_EQ(EdAsmNg::greet(), "Hello, World!");
}

TEST(GreetTests, UsesProvidedName) {
  EXPECT_EQ(EdAsmNg::greet("Kai"), "Hello, Kai!");
}
