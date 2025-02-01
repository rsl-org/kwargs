#include <gtest/gtest.h>
#include <kwargs.h>


template <typename T>
requires requires(T const& kwargs) {
    { kwargs.x } -> std::convertible_to<int>;
}
int member_access(slo::kwargs_t<T> const& kwargs) {
  return kwargs.x;
}

template <typename T>
int get_by_name(slo::kwargs_t<T> const& kwargs) {
  return get<"x">(kwargs);
}

template <typename T>
int get_by_name_default(slo::kwargs_t<T> const& kwargs) {
  return get<"x">(kwargs, 42);
}

template <typename T>
int get_by_idx(slo::kwargs_t<T> const& kwargs) {
  return get<0>(kwargs);
}

template <typename T>
int get_by_idx_default(slo::kwargs_t<T> const& kwargs) {
  return get<0>(kwargs, 42);
}

TEST(KwArgs, Simple) {
  EXPECT_EQ(member_access(make_args(x=10)), 10);
  EXPECT_EQ(get_by_idx(make_args(x=10)), 10);
  EXPECT_EQ(get_by_name(make_args(x=10)), 10);
}

TEST(KwArgs, Default) {
  EXPECT_EQ(get_by_name_default(make_args(x=10)), 10);
  EXPECT_EQ(get_by_idx_default(make_args(x=10)), 10);
  EXPECT_EQ(get_by_name_default(make_args()), 42);
  EXPECT_EQ(get_by_idx_default(make_args()), 42);
}