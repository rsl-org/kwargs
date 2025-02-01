#include <print>

#define KWARGS_FORMATTING 1
#include <kwargs.h>

template <typename T>
int test(int x, slo::kwargs_t<T> args){
  return x * get<"y">(args, 42);
}

int main(){
  std::println("test({}, {}) -> {}", 7, "y=42",
               test(7, make_args(y=23)));
  std::println("test({}, {}) -> {}", 7, "z=42",
               test(7, make_args(z=23)));

  int y = 3;
  slo::println("foo: {foo} bar: {bar}", 
               make_args(bar=3, 
                         foo=test(7, make_args(y))));
}