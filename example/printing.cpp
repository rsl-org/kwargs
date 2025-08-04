#include <vector>
#include <iostream>

#define KWARGS_FORMATTING 1
#include <kwargs.h>

int main(){
  int x = 3;
  std::vector<int> list = {1, 2, 3, 4};

  rsl::print("{} {}\n", 42, x);
  rsl::print("{1} {0}\n", x, 42);
  rsl::print("{x} {y}\n", make_args(x=42, y=x));

  rsl::print("x: {} list: {} str: {}", x, list, "foo");
  rsl::print("x: {0} list: {2} str: {1}", x, "foo", list);
  rsl::println("x: {x} list: {list} str: {str}", make_args(x=x, str="foo", list=list));
  rsl::println("x: {x} list: {list} str: {str}", make_args(x, str="foo", list));
  
  std::cout << rsl::format("{x} {y}", make_args(x, y=3)) << '\n';
}