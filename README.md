# kwargs

`erl::kwargs` is an experimental single-header library that allows for keyword arguments (also known as labeled or named arguments) through use of C++26 reflection ([P2996](https://wg21.link/p2996)).

You can find more information about it's implementation in my blog post [Fun with C++26 reflection - Keyword Arguments](https://pydong.org/posts/KwArgs/) (once I've published it :P).


# Usage
This is a single-header library, to use it simply copy [kwargs.h](include/kwargs.h) into your project's source tree.

In order to use this library, you need a compiler with [P2996 reflection](https://wg21.link/p2996) support. Currently only [Bloomberg's experimental clang fork](https://github.com/bloomberg/clang-p2996/tree/p2996) is supported, it's completely untested with EDG's implementation. If you require support for that, please reach out to me (ie via issue, email or discord). PRs are also welcome.

Compile with `-freflection` and compile and link against libc++ instead of libstdc++ (otherwise `<experimental/meta>` will not be found).

You can opt into wrappers around `std::print`, `std::println` and `std::format` with support for named arguments by defining `KWARGS_FORMATTING=1`.

# Example

[example/simple.cpp](example/simple.cpp)
```c++
#include <print>

#define KWARGS_FORMATTING 1
#include <kwargs.h>

template <typename T>
int test(int x, erl::kwargs_t<T> args){
  return x * get<"y">(args, 42);
}

int main(){
  std::println("test({}, {}) -> {}", 7, "y=42",
               test(7, make_args(y=23)));
  std::println("test({}, {}) -> {}", 7, "z=42",
               test(7, make_args(z=23)));

  // optional wrappers around `std::format`, `std::println` 
  // and `std::format`
  int y = 3;
  erl::println("foo: {foo} bar: {bar}", 
               make_args(bar=3, 
                         foo=test(7, make_args(y))));
}
```

Can be compiled and executed from this project's root by invoking
```bash
mkdir -p build
clang++ -std=c++26 -freflection -stdlib=libc++ -Iinclude example/simple.cpp -o build/simple
./build/simple
```

More examples can be found in the [example](example/) subdirectory of this repository.

# Why `erl`?
**E**xperimental **R**eflection **L**ibrary. Also I like Erlang.

# License 
[kwargs](https://github.com/tsche/kwargs) is provided under the [MIT License](LICENSE). Feel free to use and modify it in your projects.