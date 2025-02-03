#include <vector>
#include <iostream>

#define KWARGS_FORMATTING 1
#include <kwargs.h>

int main(){
  int x = 3;
  std::vector<int> list = {1, 2, 3, 4};

  erl::print("{} {}\n", 42, x);
  erl::print("{1} {0}\n", x, 42);
  erl::print("{x} {y}\n", make_args(x=42, y=x));

  erl::print("x: {} list: {} str: {}", x, list, "foo");
  erl::print("x: {0} list: {2} str: {1}", x, "foo", list);
  erl::println("x: {x} list: {list} str: {str}", make_args(x=x, str="foo", list=list));
  erl::println("x: {x} list: {list} str: {str}", make_args(x, str="foo", list));
  
  std::cout << erl::format("{x} {y}", make_args(x, y=3)) << '\n';
}