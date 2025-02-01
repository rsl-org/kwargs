#include <kwargs.h>

template <typename T>
  requires requires(T const& kwargs) {
    { kwargs.x } -> std::convertible_to<int>;
  }
void foo(int y, slo::kwargs_t<T> const& kwargs) {
  std::println("y: {} x: {} kwarg y: {}", y, kwargs.x, get<"y">(kwargs, 42));
}

template <typename T>
void bar_impl(int x, int y, slo::kwargs_t<T> const& kwargs) {
  std::println("x: {} y: {} kwarg foo: {}", x, y, get<"foo">(kwargs, "bar"));
}
#define bar(x, y, ...) bar_impl(x, y, make_args(__VA_ARGS__))

int main() {
  foo(3, make_args(x = 3));
  foo(3, make_args(x = 3, y = 17));

  bar(1, 2, foo="zoinks");

  // note that kwarg x and function argument x are different entities!
  bar(1, 2, x=2, y=3);
}