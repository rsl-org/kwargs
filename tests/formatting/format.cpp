#include <string_view>
#include <gtest/gtest.h>

#define KWARGS_FORMATTING 1
#include <kwargs.h>

using namespace std::string_view_literals;

TEST(Format, Formatting){
  EXPECT_EQ(rsl::format("{foo} {bar}", make_args(foo=1, bar=2)), "1 2"sv);
  EXPECT_EQ(rsl::format("{bar} {foo}", make_args(foo=1, bar=2)), "2 1"sv);
}