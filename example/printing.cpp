#include <vector>
#include <iostream>

#include <kwargs.h>

int main(){
  int x = 3;
  std::vector<int> list = {1, 2, 3, 4};

  slo::print("{} {}\n", 42, x);
  slo::print("{1} {0}\n", x, 42);
  slo::print("{x} {y}\n", make_args(x=42, y=x));

  slo::print("x: {} list: {} str: {}", x, list, "foo");
  slo::print("x: {0} list: {2} str: {1}", x, "foo", list);
  slo::println("x: {x} list: {list} str: {str}", make_args(x=x, str="foo", list=list));
  slo::println("x: {x} list: {list} str: {str}", make_args(x, str="foo", list));
  
  std::cout << slo::format("{x} {y}", make_args(x, y=3)) << '\n';
}