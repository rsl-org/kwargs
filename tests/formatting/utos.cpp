#include <gtest/gtest.h>
#include <kwargs.h>

TEST(Util, Utos) {
  ASSERT_EQ(rsl::_kwargs_impl::utos(1),   "1");
  ASSERT_EQ(rsl::_kwargs_impl::utos(10),  "10");
  ASSERT_EQ(rsl::_kwargs_impl::utos(111), "111");
}
