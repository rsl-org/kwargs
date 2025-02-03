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
#include <string_view>
#include <vector>
#include <tuple>
#include <type_traits>
#include <utility>
#include <ranges>
#include <cstddef>

#include <experimental/meta>

#ifndef KWARGS_FORMATTING
#  define KWARGS_FORMATTING 0
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
  constexpr fixed_string(const char (&str)[N + 1]) noexcept {
    auto idx = std::size_t{0};
    for (char const chr : str) {
      data[idx++] = chr;
    }
  }

  constexpr fixed_string(std::string_view str) { str.copy(data, N); }

  [[nodiscard]] constexpr std::string_view to_sv() const { return std::string_view{data}; }

  char data[N + 1]{};
};

template <std::size_t N>
fixed_string(char const (&)[N]) -> fixed_string<N - 1>;

constexpr std::string utos(unsigned value) {
  std::string out{};
  do {
    out += '0' + static_cast<char>(value % 10);
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
  return -1;
}

template <typename T>
consteval std::vector<std::string_view> get_member_names() {
  return nonstatic_data_members_of(^^T) | std::views::transform(std::meta::identifier_of) |
         std::ranges::to<std::vector>();
}

template <typename T>
constexpr inline std::size_t member_count = nonstatic_data_members_of(^^T).size();

namespace impl {
template <auto... Vs>
struct Replicator {
  template <typename F>
  constexpr decltype(auto) operator>>(F fnc) const {
    return fnc.template operator()<Vs...>();
  }
};

template <auto... Vs>
constexpr static Replicator<Vs...> replicator{};
}  // namespace impl

template <std::ranges::range R>
consteval auto expand(R const& range) {
  std::vector<std::meta::info> args;
  for (auto item : range) {
    args.push_back(reflect_value(item));
  }
  return substitute(^^impl::replicator, args);
}

}  // namespace meta

namespace kwargs {
struct CaptureParser : util::Parser {
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
        // possibly default capture, reject
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
constexpr auto make(Ts&&... values){
    static_assert(CaptureParser{Names.to_sv()}.parse(), "Invalid keyword argument list");

    struct kwargs_impl;
    consteval { 
        std::vector<std::meta::info> types{^^Ts...};
        std::vector<std::meta::info> args;
        
        auto parser = CaptureParser{Names.to_sv()};
        parser.parse();

        // associate every argument with the corresponding name
        // retrieved by parsing the capture list

        // std::views::zip_transform could also be used for this
        for (auto [member, name] : std::views::zip(types, parser.names)) {
            args.push_back(data_member_spec(member, {.name = name}));
        }
        define_aggregate(^^kwargs_impl, args);
    };

  // ensure injecting the class worked
  static_assert(is_type(^^kwargs_impl));

  return kwargs_t<kwargs_impl>{{std::forward<Ts>(values)...}};
}

template <util::fixed_string Names, typename T>
auto from_lambda(T&& lambda) {
  using fnc_t = std::remove_cvref_t<T>;

  return [:meta::expand(nonstatic_data_members_of(^^fnc_t)):] 
  >> [&]<auto... member>() {
    return make<Names>(lambda.[:member:]...);
  };
}
}  // namespace kwargs

template <util::fixed_string Names, typename... Ts>
constexpr auto make_args(Ts&&... values){
    return kwargs::make<Names>(std::forward<Ts>(values)...);
}

// get by index

template <std::size_t I, typename T>
  requires(meta::member_count<T> > I)
constexpr auto get(kwargs_t<T> const& obj) noexcept {
  return obj.[:meta::get_nth_member(^^T, I):];
}

template <std::size_t I, typename T, typename R>
constexpr auto get(kwargs_t<T> const& obj, R default_) noexcept {
  if constexpr (meta::member_count<T> > I) {
    return get<I>(obj);
  } else {
    return default_;
  }
}

// get by name

template <util::fixed_string name, typename T>
  requires(meta::member_count<T> > meta::get_member_index<T>(name.to_sv()))
constexpr auto get(kwargs_t<T> const& obj) {
  return obj.[:meta::get_nth_member(^^T, meta::get_member_index<T>(name.to_sv())):];
}

template <util::fixed_string name, typename T, typename R>
constexpr R get(kwargs_t<T> const& obj, R default_) {
  if constexpr (meta::member_count<T> > meta::get_member_index<T>(name.to_sv())) {
    return get<name>(obj);
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
        // ++index;
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
  static constexpr auto fmt_fixed =
      std::meta::define_static_string(FmtParser{fmt.to_sv()}.transform(meta::get_member_names<typename Args::type>()));

  return [&]<std::size_t... Idx>(std::index_sequence<Idx...>) {
    using fmt_type = std::format_string<std::tuple_element<Idx, Args>...>;
    return std::format(fmt_fixed, get<Idx>(kwargs)...);
  }(std::make_index_sequence<std::tuple_size_v<Args>>{});
}

template <std::meta::info Args>
struct NamedFormatString {
  using format_type = std::string (*)([:Args:] const&);
  format_type format;

  template <typename Tp>
    requires std::convertible_to<Tp const&, std::string_view>
  consteval explicit(false) NamedFormatString(Tp const& str) {
    auto fmt = util::fixed_string{str};
    format   = extract<format_type>(substitute(^^format_impl, {std::meta::reflect_value(fmt), Args}));
  }
};
}  // namespace formatting

template <typename T>
  requires(is_kwargs<T>)
void print(erl::formatting::NamedFormatString<^^T> fmt, T const& kwargs) {
  fputs(fmt.format(kwargs).c_str(), stdout);
}

template <typename... Args>
  requires(sizeof...(Args) != 1 || (!is_kwargs<std::remove_cvref_t<Args>> && ...))
void print(std::format_string<Args...> fmt, Args&&... args) {
  std::print(fmt, std::forward<Args>(args)...);
}

template <typename T>
  requires(is_kwargs<T>)
void println(erl::formatting::NamedFormatString<^^T> fmt, T const& kwargs) {
  puts(fmt.format(kwargs).c_str());
}

template <typename... Args>
  requires(sizeof...(Args) != 1 || (!is_kwargs<std::remove_cvref_t<Args>> && ...))
void println(std::format_string<Args...> fmt, Args&&... args) {
  std::println(fmt, std::forward<Args>(args)...);
}

template <typename T>
  requires(is_kwargs<T>)
std::string format(erl::formatting::NamedFormatString<^^T> fmt, T const& kwargs) {
  return fmt.format(kwargs);
}

template <typename... Args>
  requires(sizeof...(Args) != 1 || (!is_kwargs<std::remove_cvref_t<Args>> && ...))
std::string format(std::format_string<Args...> fmt, Args&&... args) {
  return std::format(fmt, std::forward<Args>(args)...);
}
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