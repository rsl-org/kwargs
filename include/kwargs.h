/*
MIT License

Copyright (c) 2025 Tsche

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <algorithm>
#include <concepts>
#include <string_view>
#include <vector>
#include <tuple>
#include <type_traits>
#include <utility>
#include <ranges>
#include <cstddef>

#include <experimental/meta>

#ifndef KWARGS_FORMATTING
#  define KWARGS_FORMATTING 1
#endif

#if KWARGS_FORMATTING == 1
#  include <format>
#  include <print>
#  include <cstdio>
#  include <string>
#endif

namespace erl {

template <typename Impl>
struct [[nodiscard]] kwargs_t : Impl {
  using type = Impl;
};

template <typename T>
concept is_kwargs = has_template_arguments(^^T) && template_of(^^T) == ^^kwargs_t;

namespace util {

template <std::size_t N>
struct fixed_string {
  constexpr static auto size = N;

  constexpr fixed_string() = default;
  constexpr explicit(false) fixed_string(const char (&str)[N + 1]) noexcept {
    auto idx = std::size_t{0};
    for (char const chr : str) {
      data[idx++] = chr;
    }
  }
  constexpr explicit fixed_string(std::same_as<char> auto... Vs) : data{Vs...} {}
  constexpr explicit fixed_string(std::string_view str) { str.copy(data, N); }
  constexpr explicit(false) operator std::string_view() const { return std::string_view{data}; }

  char data[N + 1]{};
};

template <std::size_t N>
fixed_string(char const (&)[N]) -> fixed_string<N - 1>;

constexpr std::string utos(unsigned value) {
  std::string out{};
  do {
    out += static_cast<char>('0' + (value % 10U));
    value /= 10;
  } while (value > 0);
  return {out.rbegin(), out.rend()};
}

constexpr bool is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

struct Parser {
  std::string_view data;
  std::size_t cursor{0};

  constexpr void skip_whitespace() {
    while (is_valid()) {
      if (char c = current(); is_whitespace(c) || c == '\\') {
        ++cursor;
      } else {
        break;
      }
    }
  }

  constexpr void skip_to(std::same_as<char> auto... needles) {
    int brace_count = 0;
    while (is_valid()) {
      if (char c = current(); brace_count == 0 && ((c == needles) || ...)) {
        break;
      } else if (c == '[' || c == '{' || c == '(') {
        ++brace_count;
      } else if (c == ']' || c == '}' || c == ')') {
        --brace_count;
      }
      ++cursor;
    }
  }

  [[nodiscard]] constexpr char current() const { return data[cursor]; }
  [[nodiscard]] constexpr bool is_valid() const { return cursor < data.length(); }
};
}  // namespace util

namespace meta {

consteval std::meta::info get_nth_member(std::meta::info reflection, std::size_t n) {
  return nonstatic_data_members_of(reflection)[n];
}

template <typename T>
consteval std::size_t get_member_index(std::string_view name) {
  std::vector<std::string_view> names =
      nonstatic_data_members_of(^^T) | std::views::transform(std::meta::identifier_of) | std::ranges::to<std::vector>();
  if (auto it = std::ranges::find(names, name); it != names.end()) {
    return std::distance(names.begin(), it);
  }
  return -1UZ;
}

template <typename T>
consteval bool has_member(std::string_view name) {
  return get_member_index<T>(name) != -1ULL;
}

template <typename T>
consteval std::vector<std::string_view> get_member_names() {
  return nonstatic_data_members_of(^^T) | std::views::transform(std::meta::identifier_of) |
         std::ranges::to<std::vector>();
}

template <typename T>
constexpr inline std::size_t member_count = nonstatic_data_members_of(^^T).size();

template <char... Vs>
constexpr inline auto static_string = util::fixed_string<sizeof...(Vs)>{Vs...};

template <typename T, T... Vs>
constexpr inline T static_array[sizeof...(Vs)]{Vs...};

consteval auto intern(std::string_view str) {
  std::vector<std::meta::info> args;
  for (auto character : str) {
    args.push_back(std::meta::reflect_value(character));
  }
  return substitute(^^static_string, args);
}

template <std::ranges::input_range R>
  requires (!std::same_as<std::ranges::range_value_t<R>, char>)
consteval auto intern(R&& iterable) {
  std::vector args = {^^std::ranges::range_value_t<R>};
  for (auto element : iterable) {
    args.push_back(std::meta::reflect_value(element));
  }
  return substitute(^^static_array, args);
}

namespace impl {
template <auto... Vs>
struct Replicator {
  template <typename F>
  constexpr decltype(auto) operator>>(F fnc) const {
    return fnc.template operator()<Vs...>();
  }

  template <typename F>
  constexpr void operator>>=(F fnc) const {
    (fnc.template operator()<Vs>(), ...);
  }
};

template <auto... Vs>
constexpr static Replicator<Vs...> replicator{};
}  // namespace impl

template <std::ranges::range R>
consteval auto expand(R const& range) {
  std::vector<std::meta::info> args;
  for (auto item : range) {
    args.push_back(std::meta::reflect_value(item));
  }
  return substitute(^^impl::replicator, args);
}

template <std::ranges::range R>
consteval auto enumerate(R range) {
  std::vector<std::meta::info> args;

  // could also use std::views::enumerate(range)
  for (auto idx = 0; auto item : range) {
    args.push_back(std::meta::reflect_value(std::pair{idx++, item}));
  }
  return substitute(^^impl::replicator, args);
}

consteval auto sequence(unsigned maximum) {
  return expand(std::ranges::iota_view{0U, maximum});
}

}  // namespace meta

namespace kwargs {
struct NameParser : util::Parser {
  std::vector<std::string_view> names;

  constexpr bool parse() {
    cursor = 0;

    while (is_valid()) {
      skip_whitespace();

      if (current() == '&') {
        // might be captured by reference
        ++cursor;
        skip_whitespace();
      }

      if (current() == '.') {
        // pack captured, reject
        return false;
      }

      std::size_t start = cursor;

      // find '=', ',' or whitespace
      skip_to('=', ',', ' ', '\n', '\r', '\t');
      if (cursor - start == 0) {
        // default capture or invalid name
        return false;
      }

      auto name = data.substr(start, cursor - start);
      if (name == "this" || name == "*this") {
        // this captured, reject
        return false;
      }
      names.emplace_back(name);

      // skip ahead to next capture
      // if the current character is already ',', this will not move the cursor
      skip_to(',');
      ++cursor;

      skip_whitespace();
    }
    return true;
  }
};

template <util::fixed_string Names, typename... Ts>
constexpr auto make(Ts&&... values) {
  struct kwargs_impl;
  consteval {
    std::vector<std::meta::info> types{^^Ts...};
    std::vector<std::meta::info> args;

    auto parser = NameParser{Names};

    // with P3068 parser.parse() could throw to provide better diagnostics at this point
    if (!parser.parse()) {
      return;
    }

    // associate every argument with the corresponding name
    // retrieved by parsing the capture list

    // std::views::zip_transform could also be used for this
    for (auto [member, name] : std::views::zip(types, parser.names)) {
      args.push_back(data_member_spec(member, {.name = name}));
    }
    define_aggregate(^^kwargs_impl, args);
  };

  // ensure injecting the class worked
  static_assert(is_type(^^kwargs_impl), std::string{"Invalid keyword arguments `"} + Names + "`");

  return kwargs_t<kwargs_impl>{{std::forward<Ts>(values)...}};
}

template <util::fixed_string Names, typename T>
auto from_lambda(T&& lambda) {
  using fnc_t = std::remove_cvref_t<T>;

  return [:meta::expand(nonstatic_data_members_of(^^fnc_t)):] >> [&]<auto... member>() {
    return make<Names>(std::forward<T>(lambda).[:member:]...);
  };
}
}  // namespace kwargs

template <util::fixed_string Names, typename... Ts>
constexpr auto make_args(Ts&&... values) {
  return kwargs::make<Names>(std::forward<Ts>(values)...);
}

template <typename T>
consteval bool has_arg(kwargs_t<T> const&, std::string_view name) {
  return meta::has_member<T>(name);
}

// get

template <std::size_t I, typename T>
  requires is_kwargs<std::remove_cvref_t<T>>
constexpr auto get(T&& kwargs) noexcept {
  using kwarg_tuple = typename std::remove_cvref_t<T>::type;
  static_assert(meta::member_count<kwarg_tuple> > I);

  return std::forward<T>(kwargs).[:meta::get_nth_member(^^kwarg_tuple, I):];
}

template <util::fixed_string name, typename T>
  requires is_kwargs<std::remove_cvref_t<T>>
constexpr auto get(T&& kwargs) {
  using kwarg_tuple = typename std::remove_cvref_t<T>::type;
  static_assert(meta::has_member<kwarg_tuple>(name), "Keyword argument `" + std::string(name) + "` not found.");

  return std::forward<T>(kwargs).[:meta::get_nth_member(^^kwarg_tuple, meta::get_member_index<kwarg_tuple>(name)):];
}

// get_or
template <std::size_t I, typename T, typename R>
  requires is_kwargs<std::remove_cvref_t<T>>
constexpr auto get_or(T&& kwargs, R default_) noexcept {
  using kwarg_tuple = typename std::remove_cvref_t<T>::type;
  if constexpr (meta::member_count<kwarg_tuple> > I) {
    return get<I>(std::forward<T>(kwargs));
  } else {
    return default_;
  }
}

template <util::fixed_string name, typename T, typename R>
  requires is_kwargs<std::remove_cvref_t<T>>
constexpr auto get_or(T&& kwargs, R default_) {
  using kwarg_tuple = typename std::remove_cvref_t<T>::type;
  if constexpr (meta::member_count<kwarg_tuple> > meta::get_member_index<kwarg_tuple>(name)) {
    return get<name>(std::forward<T>(kwargs));
  } else {
    return default_;
  }
}

#if KWARGS_FORMATTING == 1
namespace formatting {
struct FmtParser : util::Parser {
  constexpr std::string transform(std::ranges::input_range auto const& names) {
    std::string out;
    while (is_valid()) {
      out += current();
      if (current() == '{') {
        ++cursor;
        if (current() == '{') {
          // double curly braces means escaped curly braces
          // => treat the content as text
          auto start = cursor;
          // skip to first unbalanced }
          // this will match the outer {
          skip_to('}');
          out += data.substr(start, cursor - start);
          continue;
        }

        // find name
        std::size_t start = cursor;
        skip_to('}', ':');
        auto name = data.substr(start, cursor - start);

        // replace name
        auto it  = std::find(names.begin(), names.end(), name);
        auto idx = std::distance(names.begin(), it);
        out += util::utos(idx);

        out += current();
      }
      ++cursor;
    }
    return out;
  }
};

template <util::fixed_string fmt, typename Args>
std::string format_impl(Args const& kwargs) {
  return [:meta::sequence(std::tuple_size_v<Args>):]
  >> [&]<std::size_t... Idx>() {
    return std::format(fmt, get<Idx>(kwargs)...);
  };
}

template <typename Args>
struct NamedFormatString {
  using format_type = std::string (*)(Args const&);
  format_type format;

  template <typename Tp>
    requires std::convertible_to<Tp const&, std::string_view>
  consteval explicit(false) NamedFormatString(Tp const& str) {
    auto parser = FmtParser{str};
    auto fmt    = parser.transform(meta::get_member_names<typename Args::type>());
    format      = extract<format_type>(substitute(^^format_impl, {meta::intern(fmt), ^^Args}));
  }
};
}  // namespace formatting

template <typename T>
using named_format_string = formatting::NamedFormatString<std::type_identity_t<T>>;

template <typename T>
  requires(is_kwargs<T>)
void print(erl::named_format_string<T> fmt, T const& kwargs) {
  fputs(fmt.format(kwargs).c_str(), stdout);
}

template <typename... Args>
  requires(sizeof...(Args) != 1 || (!is_kwargs<std::remove_cvref_t<Args>> && ...))
void print(std::format_string<Args...> fmt, Args&&... args) {
  std::print(fmt, std::forward<Args>(args)...);
}

template <typename T>
  requires(is_kwargs<T>)
void println(erl::named_format_string<T> fmt, T const& kwargs) {
  puts(fmt.format(kwargs).c_str());
}

template <typename... Args>
  requires(sizeof...(Args) != 1 || (!is_kwargs<std::remove_cvref_t<Args>> && ...))
void println(std::format_string<Args...> fmt, Args&&... args) {
  std::println(fmt, std::forward<Args>(args)...);
}

template <typename T>
  requires(is_kwargs<T>)
std::string format(erl::named_format_string<T> fmt, T const& kwargs) {
  return fmt.format(kwargs);
}

template <typename... Args>
  requires(sizeof...(Args) != 1 || (!is_kwargs<std::remove_cvref_t<Args>> && ...))
std::string format(std::format_string<Args...> fmt, Args&&... args) {
  return std::format(fmt, std::forward<Args>(args)...);
}
#endif

#if __has_feature(parameter_reflection)
namespace kwargs {
template <std::meta::info F>
  requires(is_function(F))
struct Wrap {
  template <typename T, std::size_t PosOnly = 0>
  static constexpr void check_args() {
    [:erl::meta::expand(parameters_of(F) | std::views::take(PosOnly)):] >>= [&]<auto Param> {
      static_assert(!erl::meta::has_member<T>(identifier_of(Param)),
                    "In call to `" + std::string(identifier_of(F)) + "`: Positional argument `" + identifier_of(Param) +
                        "` repeated as keyword argument.");
    };

    [:erl::meta::expand(parameters_of(F) | std::views::drop(PosOnly)):] >>= [&]<auto Param> {
      static_assert(
          erl::meta::has_member<T>(identifier_of(Param)),
          "In call to `" + std::string(identifier_of(F)) + "`: Argument `" + identifier_of(Param) + "` missing.");
    };
  }

  static constexpr decltype(auto) operator()()
    requires requires { [:F:](); }
  {
    return [:F:]();
  }

  template <typename... Args>
    requires(sizeof...(Args) > 0)
  static constexpr decltype(auto) operator()(Args&&... args) {
    static constexpr std::size_t args_size = sizeof...(Args) - 1;
    using T                                = std::remove_cvref_t<Args...[args_size]>;

    if constexpr (erl::is_kwargs<T>) {
      check_args<typename T::type, args_size>();

      return [:erl::meta::expand(parameters_of(F) | std::views::drop(args_size)):] >> [&]<auto... Params> {
        return [:erl::meta::sequence(args_size):] >> [&]<std::size_t... Idx> {
          return [:F:](
              /* positional arguments */
              std::forward<Args...[Idx]>(args...[Idx])...,
              /* keyword arguments */
              get<erl::meta::get_member_index<typename T::type>(identifier_of(Params))>(args...[args_size])...);
        };
      };
    } else {
      // no keyword arguments
      return [:F:](std::forward<Args>(args)...);
    }
  }
};

template <auto F>
constexpr inline Wrap<F> invoke{};
}  // namespace kwargs
#endif

}  // namespace erl

template <typename T>
struct std::tuple_size<erl::kwargs_t<T>>
    : public integral_constant<size_t, erl::meta::member_count<std::remove_cvref_t<T>>> {};

template <std::size_t I, typename T>
struct std::tuple_element<I, erl::kwargs_t<T>> {
  using type = [:erl::meta::get_nth_member(^^T, I):];
};

#define make_args(...) ::erl::kwargs::from_lambda<#__VA_ARGS__>([__VA_ARGS__] {})