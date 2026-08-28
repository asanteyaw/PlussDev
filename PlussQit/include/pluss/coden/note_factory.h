#pragma once

#include <array>
#include <algorithm>
#include <concepts>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "MSharp/RangeView.h"
#include "pluss/coden/coden.h"
#include "pluss/coden/internal/note_factory_detail.h"
#include "pluss/coden/internal/note_text_reader.h"
#include "pluss/coden/note_content.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_note_literal.h"
#include "pluss/detail/select_core.h"

// `Note` (capital N) names the type; `pluss::note::` (lowercase) is its
// dedicated factory namespace -- the construction surface, kept separate
// from the type itself the same way `pluss::timeline::make`/
// `pluss::duration::*` are kept separate from `Timeline`/`Duration`.
//
// pluss::note::make("hi")                   -> scalar Note (0D)
// pluss::note::make({"a","b"})               -> 1D Note
// pluss::note::make(flat, sizes)             -> N-D Note (up to 3D)
// pluss::note::bytes(...)                    -> bytes-mode Note (raw bytes, no UTF-8 guarantees)
//
// Doctrine: once a Note exists, transformations are methods on Note. This
// header should only expose Note creation factories (no free-function Note
// ops).
//
// make/bytes each take a single NoteTextReader parameter covering
// scalar/1D/nested-N-D construction via implicit conversion (see
// internal/note_text_reader.h for why this isn't built on Series' DataReader
// -- text vs numeric domains, same reasoning as CodeH's TimelineTextReader),
// plus one explicit-shape (flat, sizes) overload per source container type
// (two call-site arguments can't fold into one converted reader parameter).
// This collapses what was previously 12 make_note overloads (and 7
// bytes_content + 3 make_bytes_note overloads) down to 3 each.
namespace pluss::note {

class FormatValue {
public:
  using scalar_type = std::variant<bool, std::int64_t, std::uint64_t, double, std::string>;
  using list_type = std::vector<scalar_type>;
  using storage_type = std::variant<bool, std::int64_t, std::uint64_t, double, std::string, list_type>;

  FormatValue(bool value) : storage_(value) {}
  FormatValue(char value) : storage_(std::string(1, value)) {}
  FormatValue(const char* value) : storage_(std::string(value != nullptr ? value : "")) {}
  FormatValue(std::string_view value) : storage_(std::string(value)) {}
  FormatValue(std::string value) : storage_(std::move(value)) {}
  FormatValue(scalar_type value) {
    storage_ = std::visit(
        [](auto&& cell) -> storage_type {
          return storage_type(std::forward<decltype(cell)>(cell));
        },
        std::move(value));
  }
  FormatValue(list_type value) : storage_(std::move(value)) {}

  template <std::signed_integral Int>
    requires (!std::same_as<std::remove_cvref_t<Int>, bool> &&
              !std::same_as<std::remove_cvref_t<Int>, char>)
  FormatValue(Int value) : storage_(static_cast<std::int64_t>(value)) {}

  template <std::unsigned_integral UInt>
    requires (!std::same_as<std::remove_cvref_t<UInt>, bool> &&
              !std::same_as<std::remove_cvref_t<UInt>, char>)
  FormatValue(UInt value) : storage_(static_cast<std::uint64_t>(value)) {}

  template <std::floating_point Float>
  FormatValue(Float value) : storage_(static_cast<double>(value)) {}

  template <class T, class Alloc>
  FormatValue(const std::vector<T, Alloc>& values) {
    list_type list;
    list.reserve(values.size());
    for (const auto& value : values) {
      list.push_back(to_scalar(value));
    }
    storage_ = std::move(list);
  }

  const storage_type& storage() const noexcept { return storage_; }

private:
  template <class T>
  static scalar_type to_scalar(T&& value) {
    FormatValue wrapped(std::forward<T>(value));
    return std::visit(
        [](auto&& cell) -> scalar_type {
          using U = std::decay_t<decltype(cell)>;
          if constexpr (std::same_as<U, list_type>) {
            PLUSS_FAIL("pluss::note::format: nested vector bindings are not supported");
          } else {
            return scalar_type(std::forward<decltype(cell)>(cell));
          }
        },
        std::move(wrapped.storage_));
  }

  storage_type storage_;
};

namespace detail {

template <class T, class = void>
struct note_format_function_traits;

template <class R, class... Args>
struct note_format_function_traits<R (*)(Args...), void> {
  using return_type = R;
  using args_tuple = std::tuple<Args...>;
  static constexpr std::size_t arity = sizeof...(Args);
};

template <class R, class... Args>
struct note_format_function_traits<R(Args...), void>
    : note_format_function_traits<R (*)(Args...)> {};

template <class R, class... Args>
struct note_format_function_traits<std::function<R(Args...)>, void>
    : note_format_function_traits<R (*)(Args...)> {};

template <class C, class R, class... Args>
struct note_format_function_traits<R (C::*)(Args...), void>
    : note_format_function_traits<R (*)(Args...)> {};

template <class C, class R, class... Args>
struct note_format_function_traits<R (C::*)(Args...) const, void>
    : note_format_function_traits<R (*)(Args...)> {};

template <class F>
struct note_format_function_traits<F, std::void_t<decltype(&F::operator())>>
    : note_format_function_traits<decltype(&F::operator())> {};

template <class F, class = void>
struct is_note_format_callable : std::false_type {};

template <class F>
struct is_note_format_callable<F, std::void_t<typename note_format_function_traits<F>::return_type>>
    : std::true_type {};

template <class F>
inline constexpr bool is_note_format_callable_v =
    is_note_format_callable<std::remove_cvref_t<F>>::value;

template <class T>
inline constexpr bool is_vector_v = false;

template <class T, class Alloc>
inline constexpr bool is_vector_v<std::vector<T, Alloc>> = true;

template <class T>
struct vector_value_type;

template <class T, class Alloc>
struct vector_value_type<std::vector<T, Alloc>> {
  using type = T;
};

inline std::string note_format_scalar_to_string(const FormatValue::scalar_type& value) {
  return std::visit(
      [](const auto& cell) -> std::string {
        using T = std::decay_t<decltype(cell)>;
        if constexpr (std::same_as<T, bool>) {
          return cell ? "true" : "false";
        } else if constexpr (std::same_as<T, std::string>) {
          return cell;
        } else if constexpr (std::same_as<T, std::int64_t>) {
          return std::to_string(cell);
        } else if constexpr (std::same_as<T, std::uint64_t>) {
          return std::to_string(cell);
        } else {
          std::ostringstream oss;
          oss << std::setprecision(15) << cell;
          return oss.str();
        }
      },
      value);
}

template <class T>
T note_format_convert_scalar(const FormatValue::scalar_type& value, std::string_view expr) {
  using U = std::remove_cvref_t<T>;
  if constexpr (std::same_as<U, FormatValue::scalar_type>) {
    return value;
  } else if constexpr (std::same_as<U, std::string>) {
    return note_format_scalar_to_string(value);
  } else if constexpr (std::same_as<U, std::string_view>) {
    const auto* text = std::get_if<std::string>(&value);
    PLUSS_CHECK(text != nullptr,
                "pluss::note::format: lambda string_view argument requires a string value in expression {" +
                    std::string(expr) + "}");
    return std::string_view(*text);
  } else if constexpr (std::same_as<U, bool>) {
    return std::visit(
        [&](const auto& cell) -> bool {
          using V = std::decay_t<decltype(cell)>;
          if constexpr (std::same_as<V, bool>) {
            return cell;
          } else if constexpr (std::same_as<V, std::int64_t> || std::same_as<V, std::uint64_t> ||
                               std::same_as<V, double>) {
            return cell != 0;
          } else if constexpr (std::same_as<V, std::string>) {
            return !cell.empty();
          }
        },
        value);
  } else if constexpr (std::integral<U> && !std::same_as<U, bool> && !std::same_as<U, char>) {
    return static_cast<U>(std::visit(
        [&](const auto& cell) -> double {
          using V = std::decay_t<decltype(cell)>;
          if constexpr (std::same_as<V, bool>) {
            return cell ? 1.0 : 0.0;
          } else if constexpr (std::same_as<V, std::int64_t> || std::same_as<V, std::uint64_t> ||
                               std::same_as<V, double>) {
            return static_cast<double>(cell);
          } else {
            PLUSS_FAIL("pluss::note::format: lambda argument is not numeric in expression {" +
                       std::string(expr) + "}");
          }
        },
        value));
  } else if constexpr (std::floating_point<U>) {
    return static_cast<U>(std::visit(
        [&](const auto& cell) -> double {
          using V = std::decay_t<decltype(cell)>;
          if constexpr (std::same_as<V, bool>) {
            return cell ? 1.0 : 0.0;
          } else if constexpr (std::same_as<V, std::int64_t> || std::same_as<V, std::uint64_t> ||
                               std::same_as<V, double>) {
            return static_cast<double>(cell);
          } else {
            PLUSS_FAIL("pluss::note::format: lambda argument is not numeric in expression {" +
                       std::string(expr) + "}");
          }
        },
        value));
  } else {
    static_assert(!sizeof(T), "unsupported pluss::note::format lambda argument type");
  }
}

template <class T>
auto note_format_convert_arg(const FormatValue& value, std::string_view expr) {
  using U = std::remove_cvref_t<T>;
  if constexpr (std::same_as<U, FormatValue>) {
    return value;
  } else if constexpr (is_vector_v<U>) {
    using Item = typename vector_value_type<U>::type;
    const auto* list = std::get_if<FormatValue::list_type>(&value.storage());
    PLUSS_CHECK(list != nullptr,
                "pluss::note::format: lambda argument requires a vector/list value in expression {" +
                    std::string(expr) + "}");
    U out;
    out.reserve(list->size());
    for (const auto& item : *list) {
      out.push_back(note_format_convert_scalar<Item>(item, expr));
    }
    return out;
  } else {
    return std::visit(
        [&](const auto& cell) -> U {
          using V = std::decay_t<decltype(cell)>;
          if constexpr (std::same_as<V, FormatValue::list_type>) {
            PLUSS_FAIL("pluss::note::format: lambda argument expected a scalar in expression {" +
                       std::string(expr) + "}");
          } else {
            return note_format_convert_scalar<U>(FormatValue::scalar_type(cell), expr);
          }
        },
        value.storage());
  }
}

template <class F, class Tuple, std::size_t... I>
FormatValue note_format_invoke_callable(F& fn,
                                        const std::vector<FormatValue>& args,
                                        std::string_view expr,
                                        std::index_sequence<I...>) {
  using R = std::invoke_result_t<F&, std::tuple_element_t<I, Tuple>...>;
  static_assert(!std::same_as<R, void>, "pluss::note::format lambdas must return a value");
  return FormatValue(std::invoke(fn, note_format_convert_arg<std::tuple_element_t<I, Tuple>>(args[I], expr)...));
}

template <class F>
auto note_format_make_callable(F&& fn) {
  // Decay a free function to a copyable function pointer before capturing it.
  using Fn = std::decay_t<F>;
  using Traits = note_format_function_traits<Fn>;
  using Args = typename Traits::args_tuple;
  return [callable = Fn(std::forward<F>(fn))](const std::vector<FormatValue>& args,
                                             std::string_view expr) mutable -> FormatValue {
    PLUSS_CHECK(args.size() == Traits::arity,
                "pluss::note::format: callable expected " + std::to_string(Traits::arity) +
                    " argument(s) in expression {" + std::string(expr) + "}");
    return note_format_invoke_callable<Fn, Args>(
        callable, args, expr, std::make_index_sequence<Traits::arity>{});
  };
}

} // namespace detail

struct Binding {
  using Callable = std::function<FormatValue(const std::vector<FormatValue>&, std::string_view)>;

  std::string name;
  FormatValue value;
  std::optional<Callable> callable;

  template <class T>
    requires (!detail::is_note_format_callable_v<T>)
  Binding(std::string_view key, T&& bound_value)
      : name(key), value(std::forward<T>(bound_value)) {}

  template <class F>
    requires detail::is_note_format_callable_v<F>
  Binding(std::string_view key, F&& bound_callable)
      : name(key),
        value("<callable>"),
        callable(detail::note_format_make_callable(std::forward<F>(bound_callable))) {}
};

namespace detail {

inline bool note_format_is_ident_start(char ch) {
  const unsigned char u = static_cast<unsigned char>(ch);
  return std::isalpha(u) || ch == '_';
}

inline bool note_format_is_ident_continue(char ch) {
  const unsigned char u = static_cast<unsigned char>(ch);
  return std::isalnum(u) || ch == '_';
}

inline std::string_view note_format_trim(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return text.substr(begin, end - begin);
}

inline std::string note_format_double_to_string(double value) {
  std::ostringstream oss;
  oss << std::setprecision(15) << value;
  return oss.str();
}

inline std::string note_format_scalar_stringify(const FormatValue::scalar_type& value) {
  return std::visit(
      [](const auto& cell) -> std::string {
        using T = std::decay_t<decltype(cell)>;
        if constexpr (std::same_as<T, bool>) {
          return cell ? "true" : "false";
        } else if constexpr (std::same_as<T, std::string>) {
          return cell;
        } else if constexpr (std::same_as<T, std::int64_t>) {
          return std::to_string(cell);
        } else if constexpr (std::same_as<T, std::uint64_t>) {
          return std::to_string(cell);
        } else {
          return note_format_double_to_string(cell);
        }
      },
      value);
}

inline std::string note_format_stringify(const FormatValue& value) {
  return std::visit(
      [](const auto& cell) -> std::string {
        using T = std::decay_t<decltype(cell)>;
        if constexpr (std::same_as<T, bool>) {
          return cell ? "true" : "false";
        } else if constexpr (std::same_as<T, std::string>) {
          return cell;
        } else if constexpr (std::same_as<T, std::int64_t>) {
          return std::to_string(cell);
        } else if constexpr (std::same_as<T, std::uint64_t>) {
          return std::to_string(cell);
        } else if constexpr (std::same_as<T, FormatValue::list_type>) {
          std::string out = "[";
          for (std::size_t i = 0; i < cell.size(); ++i) {
            if (i != 0) out += ", ";
            out += note_format_scalar_stringify(cell[i]);
          }
          out += "]";
          return out;
        } else {
          return note_format_double_to_string(cell);
        }
      },
      value.storage());
}

inline double note_format_as_numeric(const FormatValue& value, std::string_view expr) {
  return std::visit(
      [&](const auto& cell) -> double {
        using T = std::decay_t<decltype(cell)>;
        if constexpr (std::same_as<T, std::int64_t> || std::same_as<T, std::uint64_t> ||
                      std::same_as<T, double>) {
          return static_cast<double>(cell);
        } else if constexpr (std::same_as<T, bool>) {
          return cell ? 1.0 : 0.0;
        } else {
          PLUSS_FAIL("pluss::note::format: value is not numeric in expression {" +
                     std::string(expr) + "}");
        }
      },
      value.storage());
}

inline double note_format_as_numeric(const Binding& binding, std::string_view expr) {
  try {
    return note_format_as_numeric(binding.value, expr);
  } catch (const std::exception&) {
    PLUSS_FAIL("pluss::note::format: binding '" + binding.name +
               "' is not numeric and cannot be used in expression {" +
               std::string(expr) + "}");
  }
}

inline bool note_format_truthy(const FormatValue& value) {
  return std::visit(
      [](const auto& cell) -> bool {
        using T = std::decay_t<decltype(cell)>;
        if constexpr (std::same_as<T, bool>) {
          return cell;
        } else if constexpr (std::same_as<T, std::int64_t> || std::same_as<T, std::uint64_t> ||
                             std::same_as<T, double>) {
          return cell != 0;
        } else if constexpr (std::same_as<T, std::string>) {
          return !cell.empty();
        } else {
          return !cell.empty();
        }
      },
      value.storage());
}

inline const FormatValue::list_type& note_format_as_list(const FormatValue& value,
                                                         std::string_view expr) {
  const auto* list = std::get_if<FormatValue::list_type>(&value.storage());
  PLUSS_CHECK(list != nullptr,
              "pluss::note::format: expected a vector/list value in expression {" +
                  std::string(expr) + "}");
  return *list;
}

inline FormatValue note_format_number_result(double value) {
  return FormatValue(value);
}

inline FormatValue note_format_bool_result(bool value) {
  return FormatValue(value);
}

inline const Binding& note_format_lookup_binding(
    const std::unordered_map<std::string_view, const Binding*>& bindings,
    std::string_view name) {
  const auto it = bindings.find(name);
  PLUSS_CHECK(it != bindings.end(),
              "pluss::note::format: missing binding '" + std::string(name) + "'");
  return *it->second;
}

class NoteFormatExprParser {
public:
  NoteFormatExprParser(std::string_view expr,
                       const std::unordered_map<std::string_view, const Binding*>& bindings)
      : expr_(expr), bindings_(bindings) {}

  FormatValue parse() {
    const FormatValue value = parse_ternary();
    skip_ws();
    PLUSS_CHECK(pos_ == expr_.size(),
                "pluss::note::format: unexpected trailing input in expression {" +
                    std::string(expr_) + "}");
    return value;
  }

private:
  FormatValue parse_ternary() {
    FormatValue condition = parse_logical_or();
    skip_ws();
    if (!match('?')) return condition;

    FormatValue when_true = parse_ternary();
    skip_ws();
    PLUSS_CHECK(match(':'),
                "pluss::note::format: expected ':' in conditional expression {" +
                    std::string(expr_) + "}");
    FormatValue when_false = parse_ternary();
    return note_format_truthy(condition) ? when_true : when_false;
  }

  FormatValue parse_logical_or() {
    FormatValue lhs = parse_logical_and();
    while (true) {
      skip_ws();
      if (match_string("||")) {
        const bool left = note_format_truthy(lhs);
        FormatValue rhs = parse_logical_and();
        lhs = note_format_bool_result(left || note_format_truthy(rhs));
      } else {
        return lhs;
      }
    }
  }

  FormatValue parse_logical_and() {
    FormatValue lhs = parse_equality();
    while (true) {
      skip_ws();
      if (match_string("&&")) {
        const bool left = note_format_truthy(lhs);
        FormatValue rhs = parse_equality();
        lhs = note_format_bool_result(left && note_format_truthy(rhs));
      } else {
        return lhs;
      }
    }
  }

  FormatValue parse_equality() {
    FormatValue lhs = parse_comparison();
    while (true) {
      skip_ws();
      if (match_string("==")) {
        FormatValue rhs = parse_comparison();
        lhs = note_format_bool_result(note_format_stringify(lhs) == note_format_stringify(rhs));
      } else if (match_string("!=")) {
        FormatValue rhs = parse_comparison();
        lhs = note_format_bool_result(note_format_stringify(lhs) != note_format_stringify(rhs));
      } else {
        return lhs;
      }
    }
  }

  FormatValue parse_comparison() {
    FormatValue lhs = parse_expr();
    while (true) {
      skip_ws();
      if (match_string("<=")) {
        FormatValue rhs = parse_expr();
        lhs = note_format_bool_result(note_format_as_numeric(lhs, expr_) <=
                                      note_format_as_numeric(rhs, expr_));
      } else if (match_string(">=")) {
        FormatValue rhs = parse_expr();
        lhs = note_format_bool_result(note_format_as_numeric(lhs, expr_) >=
                                      note_format_as_numeric(rhs, expr_));
      } else if (match('<')) {
        FormatValue rhs = parse_expr();
        lhs = note_format_bool_result(note_format_as_numeric(lhs, expr_) <
                                      note_format_as_numeric(rhs, expr_));
      } else if (match('>')) {
        FormatValue rhs = parse_expr();
        lhs = note_format_bool_result(note_format_as_numeric(lhs, expr_) >
                                      note_format_as_numeric(rhs, expr_));
      } else {
        return lhs;
      }
    }
  }

  FormatValue parse_expr() {
    FormatValue lhs = parse_term();
    while (true) {
      skip_ws();
      if (match('+')) {
        FormatValue rhs = parse_term();
        if (std::holds_alternative<std::string>(lhs.storage()) ||
            std::holds_alternative<std::string>(rhs.storage())) {
          lhs = FormatValue(note_format_stringify(lhs) + note_format_stringify(rhs));
        } else {
          lhs = note_format_number_result(note_format_as_numeric(lhs, expr_) +
                                          note_format_as_numeric(rhs, expr_));
        }
      } else if (match('-')) {
        FormatValue rhs = parse_term();
        lhs = note_format_number_result(note_format_as_numeric(lhs, expr_) -
                                        note_format_as_numeric(rhs, expr_));
      } else {
        return lhs;
      }
    }
  }

  FormatValue parse_term() {
    FormatValue lhs = parse_factor();
    while (true) {
      skip_ws();
      if (match('*')) {
        FormatValue rhs = parse_factor();
        lhs = note_format_number_result(note_format_as_numeric(lhs, expr_) *
                                        note_format_as_numeric(rhs, expr_));
      } else if (match('/')) {
        const double rhs = note_format_as_numeric(parse_factor(), expr_);
        PLUSS_CHECK(rhs != 0.0,
                    "pluss::note::format: division by zero in expression {" +
                        std::string(expr_) + "}");
        lhs = note_format_number_result(note_format_as_numeric(lhs, expr_) / rhs);
      } else if (match('%')) {
        const double rhs = note_format_as_numeric(parse_factor(), expr_);
        PLUSS_CHECK(rhs != 0.0,
                    "pluss::note::format: modulo by zero in expression {" +
                        std::string(expr_) + "}");
        lhs = note_format_number_result(std::fmod(note_format_as_numeric(lhs, expr_), rhs));
      } else {
        return lhs;
      }
    }
  }

  FormatValue parse_factor() {
    skip_ws();
    if (match('+')) return parse_factor();
    if (match('-')) return note_format_number_result(-note_format_as_numeric(parse_factor(), expr_));
    if (match('!')) return note_format_bool_result(!note_format_truthy(parse_factor()));

    return parse_postfix(parse_primary());
  }

  FormatValue parse_primary() {
    skip_ws();
    if (match('(')) {
      const FormatValue value = parse_ternary();
      skip_ws();
      PLUSS_CHECK(match(')'),
                  "pluss::note::format: expected ')' in expression {" +
                      std::string(expr_) + "}");
      return value;
    }

    if (peek_is_number_start()) return parse_number();
    if (peek_is_quote()) return parse_string_literal();
    if (peek_is_identifier_start()) return parse_identifier();

    PLUSS_FAIL("pluss::note::format: expected number, string, identifier, or '(' in expression {" +
               std::string(expr_) + "}");
  }

  FormatValue parse_postfix(FormatValue value) {
    while (true) {
      skip_ws();
      if (match('[')) {
        const auto index_value = parse_ternary();
        skip_ws();
        PLUSS_CHECK(match(']'),
                    "pluss::note::format: expected ']' in expression {" +
                        std::string(expr_) + "}");
        value = apply_index(value, index_value);
        continue;
      }
      if (match('.')) {
        const std::string_view method = parse_member_name();
        skip_ws();
        PLUSS_CHECK(match('('),
                    "pluss::note::format: expected '(' after member function '" +
                        std::string(method) + "' in expression {" + std::string(expr_) + "}");
        value = parse_member_call(std::move(value), method);
        continue;
      }
      return value;
    }
  }

  FormatValue parse_number() {
    const std::size_t begin = pos_;
    bool saw_digit = false;

    while (pos_ < expr_.size() && std::isdigit(static_cast<unsigned char>(expr_[pos_]))) {
      ++pos_;
      saw_digit = true;
    }
    if (pos_ < expr_.size() && expr_[pos_] == '.') {
      ++pos_;
      while (pos_ < expr_.size() && std::isdigit(static_cast<unsigned char>(expr_[pos_]))) {
        ++pos_;
        saw_digit = true;
      }
    }
    if (pos_ < expr_.size() && (expr_[pos_] == 'e' || expr_[pos_] == 'E')) {
      const std::size_t exp_pos = pos_;
      ++pos_;
      if (pos_ < expr_.size() && (expr_[pos_] == '+' || expr_[pos_] == '-')) ++pos_;
      const std::size_t exp_digits = pos_;
      while (pos_ < expr_.size() && std::isdigit(static_cast<unsigned char>(expr_[pos_]))) {
        ++pos_;
      }
      PLUSS_CHECK(pos_ > exp_digits,
                  "pluss::note::format: malformed exponent in expression {" +
                      std::string(expr_) + "}");
      saw_digit = true;
      (void)exp_pos;
    }

    PLUSS_CHECK(saw_digit,
                "pluss::note::format: malformed numeric literal in expression {" +
                    std::string(expr_) + "}");
    return FormatValue(std::stod(std::string(expr_.substr(begin, pos_ - begin))));
  }

  FormatValue parse_string_literal() {
    const char quote = expr_[pos_++];
    std::string out;
    while (pos_ < expr_.size() && expr_[pos_] != quote) {
      if (expr_[pos_] == '\\') {
        ++pos_;
        PLUSS_CHECK(pos_ < expr_.size(),
                    "pluss::note::format: dangling escape in string literal {" +
                        std::string(expr_) + "}");
        const char escaped = expr_[pos_++];
        switch (escaped) {
          case 'n': out.push_back('\n'); break;
          case 't': out.push_back('\t'); break;
          case '\\': out.push_back('\\'); break;
          case '"': out.push_back('"'); break;
          case '\'': out.push_back('\''); break;
          default: out.push_back(escaped); break;
        }
      } else {
        out.push_back(expr_[pos_++]);
      }
    }
    PLUSS_CHECK(pos_ < expr_.size() && expr_[pos_] == quote,
                "pluss::note::format: unterminated string literal {" +
                    std::string(expr_) + "}");
    ++pos_;
    return FormatValue(std::move(out));
  }

  FormatValue parse_identifier() {
    const std::string_view name = parse_qualified_identifier();
    skip_ws();
    if (match('(')) return parse_function_call(name);
    const auto& binding = note_format_lookup_binding(bindings_, name);
    PLUSS_CHECK(!binding.callable.has_value(),
                "pluss::note::format: callable binding '" + binding.name +
                    "' must be called in expression {" + std::string(expr_) + "}");
    return binding.value;
  }

  FormatValue parse_function_call(std::string_view name) {
    std::vector<FormatValue> args;
    skip_ws();
    if (!match(')')) {
      while (true) {
        args.push_back(parse_ternary());
        skip_ws();
        if (match(')')) break;
        PLUSS_CHECK(match(','),
                    "pluss::note::format: expected ',' or ')' in function call {" +
                        std::string(expr_) + "}");
      }
    }

    const auto callable_it = bindings_.find(name);
    if (callable_it != bindings_.end() && callable_it->second->callable.has_value()) {
      return (*(callable_it->second->callable))(args, expr_);
    }

    const auto require_arity = [&](std::size_t n) {
      PLUSS_CHECK(args.size() == n,
                  "pluss::note::format: function '" + std::string(name) +
                      "' expected " + std::to_string(n) + " argument(s) in expression {" +
                      std::string(expr_) + "}");
    };
    const auto require_min_arity = [&](std::size_t n) {
      PLUSS_CHECK(args.size() >= n,
                  "pluss::note::format: function '" + std::string(name) +
                      "' expected at least " + std::to_string(n) +
                      " argument(s) in expression {" + std::string(expr_) + "}");
    };

    if (name == "std::abs" || name == "std::fabs") {
      require_arity(1);
      return note_format_number_result(std::fabs(note_format_as_numeric(args[0], expr_)));
    }
    if (name == "std::sqrt") {
      require_arity(1);
      const double value = note_format_as_numeric(args[0], expr_);
      PLUSS_CHECK(value >= 0.0,
                  "pluss::note::format: std::sqrt() received a negative value in expression {" +
                      std::string(expr_) + "}");
      return note_format_number_result(std::sqrt(value));
    }
    if (name == "std::floor") {
      require_arity(1);
      return note_format_number_result(std::floor(note_format_as_numeric(args[0], expr_)));
    }
    if (name == "std::ceil") {
      require_arity(1);
      return note_format_number_result(std::ceil(note_format_as_numeric(args[0], expr_)));
    }
    if (name == "std::round") {
      require_arity(1);
      return note_format_number_result(std::round(note_format_as_numeric(args[0], expr_)));
    }
    if (name == "std::pow") {
      require_arity(2);
      return note_format_number_result(std::pow(note_format_as_numeric(args[0], expr_),
                                                note_format_as_numeric(args[1], expr_)));
    }
    if (name == "std::min" || name == "std::max") {
      require_min_arity(2);
      double best = note_format_as_numeric(args.front(), expr_);
      for (std::size_t i = 1; i < args.size(); ++i) {
        const double value = note_format_as_numeric(args[i], expr_);
        best = name == "std::min" ? std::min(best, value) : std::max(best, value);
      }
      return note_format_number_result(best);
    }
    if (name == "std::accumulate") {
      require_arity(2);
      const auto& list = note_format_as_list(args[0], expr_);
      double total = note_format_as_numeric(args[1], expr_);
      for (const auto& item : list) total += note_format_as_numeric(FormatValue(item), expr_);
      return note_format_number_result(total);
    }
    if (name == "pluss::join") {
      PLUSS_CHECK(args.size() == 1 || args.size() == 2,
                  "pluss::note::format: pluss::join() expected one list and optional delimiter in expression {" +
                      std::string(expr_) + "}");
      const auto& list = note_format_as_list(args[0], expr_);
      const std::string delim = args.size() == 2 ? note_format_stringify(args[1]) : std::string(", ");
      std::string out;
      for (std::size_t i = 0; i < list.size(); ++i) {
        if (i != 0) out += delim;
        out += note_format_scalar_stringify(list[i]);
      }
      return FormatValue(std::move(out));
    }
    if (name == "std::to_string" || name == "pluss::str") {
      require_arity(1);
      return FormatValue(note_format_stringify(args[0]));
    }

    PLUSS_FAIL("pluss::note::format: unknown function '" + std::string(name) +
               "' in expression {" + std::string(expr_) + "}");
  }

  std::string_view parse_member_name() {
    skip_ws();
    PLUSS_CHECK(peek_is_identifier_start(),
                "pluss::note::format: expected member function name in expression {" +
                    std::string(expr_) + "}");
    const std::size_t begin = pos_;
    ++pos_;
    while (pos_ < expr_.size() && note_format_is_ident_continue(expr_[pos_])) ++pos_;
    return expr_.substr(begin, pos_ - begin);
  }

  std::string_view parse_qualified_identifier() {
    const std::size_t begin = pos_;
    ++pos_;
    while (pos_ < expr_.size() && note_format_is_ident_continue(expr_[pos_])) ++pos_;

    // A qualified name may use `::`; a lone ':' belongs to the ternary operator.
    while (pos_ + 1 < expr_.size() && expr_[pos_] == ':' && expr_[pos_ + 1] == ':') {
      pos_ += 2;
      PLUSS_CHECK(pos_ < expr_.size() && note_format_is_ident_start(expr_[pos_]),
                  "pluss::note::format: expected identifier after '::' in expression {" +
                      std::string(expr_) + "}");
      ++pos_;
      while (pos_ < expr_.size() && note_format_is_ident_continue(expr_[pos_])) ++pos_;
    }
    return expr_.substr(begin, pos_ - begin);
  }

  FormatValue parse_member_call(FormatValue value, std::string_view method) {
    std::vector<FormatValue> args;
    skip_ws();
    if (!match(')')) {
      while (true) {
        args.push_back(parse_ternary());
        skip_ws();
        if (match(')')) break;
        PLUSS_CHECK(match(','),
                    "pluss::note::format: expected ',' or ')' in member function call {" +
                        std::string(expr_) + "}");
      }
    }

    const auto require_arity = [&](std::size_t n) {
      PLUSS_CHECK(args.size() == n,
                  "pluss::note::format: member function '" + std::string(method) +
                      "' expected " + std::to_string(n) + " argument(s) in expression {" +
                      std::string(expr_) + "}");
    };

    if (method == "size") {
      require_arity(0);
      if (const auto* list = std::get_if<FormatValue::list_type>(&value.storage())) {
        return FormatValue(static_cast<std::int64_t>(list->size()));
      }
      if (const auto* text = std::get_if<std::string>(&value.storage())) {
        return FormatValue(static_cast<std::int64_t>(text->size()));
      }
      PLUSS_FAIL("pluss::note::format: .size() requires a string or vector/list in expression {" +
                 std::string(expr_) + "}");
    }
    if (method == "empty") {
      require_arity(0);
      if (const auto* list = std::get_if<FormatValue::list_type>(&value.storage())) {
        return FormatValue(list->empty());
      }
      if (const auto* text = std::get_if<std::string>(&value.storage())) {
        return FormatValue(text->empty());
      }
      PLUSS_FAIL("pluss::note::format: .empty() requires a string or vector/list in expression {" +
                 std::string(expr_) + "}");
    }
    if (method == "at") {
      require_arity(1);
      return apply_index(value, args[0]);
    }

    PLUSS_FAIL("pluss::note::format: unknown member function '" + std::string(method) +
               "' in expression {" + std::string(expr_) + "}");
  }

  FormatValue apply_index(const FormatValue& value, const FormatValue& index_value) {
    const double raw = note_format_as_numeric(index_value, expr_);
    const auto index = static_cast<std::int64_t>(raw);
    PLUSS_CHECK(static_cast<double>(index) == raw && index >= 0,
                "pluss::note::format: index must be a non-negative integer in expression {" +
                    std::string(expr_) + "}");
    if (const auto* list = std::get_if<FormatValue::list_type>(&value.storage())) {
      PLUSS_CHECK(static_cast<std::size_t>(index) < list->size(),
                  "pluss::note::format: vector/list index out of range in expression {" +
                      std::string(expr_) + "}");
      return FormatValue((*list)[static_cast<std::size_t>(index)]);
    }
    if (const auto* text = std::get_if<std::string>(&value.storage())) {
      PLUSS_CHECK(static_cast<std::size_t>(index) < text->size(),
                  "pluss::note::format: string index out of range in expression {" +
                      std::string(expr_) + "}");
      return FormatValue(std::string(1, (*text)[static_cast<std::size_t>(index)]));
    }
    PLUSS_FAIL("pluss::note::format: indexing requires a string or vector/list in expression {" +
               std::string(expr_) + "}");
  }

  bool peek_is_number_start() const {
    if (pos_ >= expr_.size()) return false;
    const char ch = expr_[pos_];
    if (std::isdigit(static_cast<unsigned char>(ch))) return true;
    return ch == '.' && pos_ + 1 < expr_.size() &&
           std::isdigit(static_cast<unsigned char>(expr_[pos_ + 1]));
  }

  bool peek_is_identifier_start() const {
    return pos_ < expr_.size() && note_format_is_ident_start(expr_[pos_]);
  }

  bool peek_is_quote() const {
    return pos_ < expr_.size() && (expr_[pos_] == '"' || expr_[pos_] == '\'');
  }

  void skip_ws() {
    while (pos_ < expr_.size() && std::isspace(static_cast<unsigned char>(expr_[pos_]))) ++pos_;
  }

  bool match(char ch) {
    if (pos_ < expr_.size() && expr_[pos_] == ch) {
      ++pos_;
      return true;
    }
    return false;
  }

  bool match_string(std::string_view text) {
    if (expr_.substr(pos_, text.size()) == text) {
      pos_ += text.size();
      return true;
    }
    return false;
  }

  std::string_view expr_;
  const std::unordered_map<std::string_view, const Binding*>& bindings_;
  std::size_t pos_ = 0;
};

inline bool note_format_is_plain_identifier(std::string_view expr) {
  const auto trimmed = note_format_trim(expr);
  if (trimmed.empty() || !note_format_is_ident_start(trimmed.front())) return false;
  for (std::size_t i = 1; i < trimmed.size(); ++i) {
    if (!note_format_is_ident_continue(trimmed[i])) return false;
  }
  return true;
}

inline std::string note_format_eval_placeholder(
    std::string_view placeholder,
    const std::unordered_map<std::string_view, const Binding*>& bindings) {
  const auto trimmed = note_format_trim(placeholder);
  PLUSS_CHECK(!trimmed.empty(), "pluss::note::format: empty placeholder is not allowed");

  if (note_format_is_plain_identifier(trimmed)) {
    const auto& binding = note_format_lookup_binding(bindings, trimmed);
    PLUSS_CHECK(!binding.callable.has_value(),
                "pluss::note::format: callable binding '" + binding.name +
                    "' must be called in expression {" + std::string(trimmed) + "}");
    return note_format_stringify(binding.value);
  }

  NoteFormatExprParser parser(trimmed, bindings);
  return note_format_stringify(parser.parse());
}

inline std::string note_format_render(
    std::string_view tpl,
    const std::unordered_map<std::string_view, const Binding*>& bindings) {
  std::string out;
  out.reserve(tpl.size());

  for (std::size_t i = 0; i < tpl.size(); ++i) {
    const char ch = tpl[i];
    if (ch == '{') {
      if (i + 1 < tpl.size() && tpl[i + 1] == '{') {
        out.push_back('{');
        ++i;
        continue;
      }

      const std::size_t open = i + 1;
      std::size_t close = open;
      while (close < tpl.size() && tpl[close] != '}') {
        PLUSS_CHECK(tpl[close] != '{',
                    "pluss::note::format: nested '{' is not allowed inside placeholders");
        ++close;
      }
      PLUSS_CHECK(close < tpl.size(),
                  "pluss::note::format: unmatched '{' in format template");
      out += note_format_eval_placeholder(tpl.substr(open, close - open), bindings);
      i = close;
      continue;
    }

    if (ch == '}') {
      if (i + 1 < tpl.size() && tpl[i + 1] == '}') {
        out.push_back('}');
        ++i;
        continue;
      }
      PLUSS_FAIL("pluss::note::format: unmatched '}' in format template");
    }

    out.push_back(ch);
  }

  return out;
}

} // namespace detail

inline pluss::Note make(pluss::coden::detail::NoteTextReader data) {
  return pluss::coden::factory_detail::make_note_from_textspan(data.flat(), data.shape());
}
inline pluss::Note make(pluss::msharp::RangeView::Of<std::string_view> flat,
                        pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return make(pluss::coden::detail::NoteTextReader(flat, sizes));
}
inline pluss::Note make(pluss::msharp::RangeView::Of<std::string> flat,
                        pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return make(pluss::coden::detail::NoteTextReader(flat, sizes));
}

// ---- Bytes-mode factories ----

inline pluss::Note bytes(pluss::coden::detail::NoteTextReader data) {
  return pluss::coden::factory_detail::make_bytes_note_from_textspan(data.flat(), data.shape());
}
inline pluss::Note bytes(pluss::msharp::RangeView::Of<std::string_view> flat,
                         pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return bytes(pluss::coden::detail::NoteTextReader(flat, sizes));
}
inline pluss::Note bytes(pluss::msharp::RangeView::Of<std::string> flat,
                         pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return bytes(pluss::coden::detail::NoteTextReader(flat, sizes));
}
// A raw byte buffer read as one opaque scalar "string" -- not text-domain
// input, so it doesn't go through NoteTextReader like the overloads above.
inline pluss::Note bytes(pluss::msharp::RangeView::Of<std::byte> raw) {
  const std::string_view sv{reinterpret_cast<const char*>(raw.data()), raw.size()};
  const std::array<std::string_view, 1> flat{sv};
  return pluss::coden::ops_internal::note_from_bytes(flat, {});
}

// ---- Shape factories ----

inline pluss::Note full(pluss::msharp::RangeView::Of<std::int64_t> sizes, std::string_view value) {
  const pluss::detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const auto n = pluss::coden::factory_detail::checked_numel(shape);
  std::vector<std::string_view> flat(static_cast<std::size_t>(n), value);
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), shape.span());
}

inline pluss::Note empty(pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return full(sizes, std::string_view{});
}

inline pluss::Note full_like(const pluss::Note& like, std::string_view value) {
  PLUSS_CHECK(like.defined(), "pluss::note::full_like requires a defined Note");
  const auto sizes = like.sizes();
  return full(pluss::msharp::RangeView::Of<std::int64_t>(sizes), value);
}

inline pluss::Note empty_like(const pluss::Note& like) {
  return full_like(like, std::string_view{});
}

inline pluss::Note rep(std::string_view value, pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return full(sizes, value);
}
inline pluss::Note rep(std::string_view value, std::initializer_list<std::int64_t> sizes) {
  return rep(value, pluss::msharp::RangeView::Of<std::int64_t>(sizes));
}

inline pluss::Note rand(pluss::msharp::RangeView::Of<std::int64_t> sizes,
                        bool capitalize = true,
                        std::uint64_t seed = 0) {
  const pluss::detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const auto n = pluss::coden::factory_detail::checked_numel(shape);
  const auto flat = pluss::coden::factory_detail::random_chars_from_charset(
      n,
      capitalize ? std::string_view{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"}
                 : std::string_view{"abcdefghijklmnopqrstuvwxyz"},
      seed);
  return pluss::coden::factory_detail::make_note_from_textspan(
      pluss::detail::TextSpan::from_strings(flat),
      shape);
}
inline pluss::Note rand(std::initializer_list<std::int64_t> sizes,
                        bool capitalize = true,
                        std::uint64_t seed = 0) {
  return rand(pluss::msharp::RangeView::Of<std::int64_t>(sizes), capitalize, seed);
}

inline pluss::Note rand_between(std::string_view lo,
                                std::string_view hi,
                                pluss::msharp::RangeView::Of<std::int64_t> sizes,
                                std::uint64_t seed = 0) {
  const pluss::detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const auto n = pluss::coden::factory_detail::checked_numel(shape);
  const auto flat = pluss::coden::factory_detail::random_chars_between(
      n, lo, hi, seed, "pluss::note::rand_between");
  return pluss::coden::factory_detail::make_note_from_textspan(
      pluss::detail::TextSpan::from_strings(flat),
      shape);
}
inline pluss::Note rand_between(std::string_view lo,
                                std::string_view hi,
                                std::initializer_list<std::int64_t> sizes,
                                std::uint64_t seed = 0) {
  return rand_between(lo, hi, pluss::msharp::RangeView::Of<std::int64_t>(sizes), seed);
}

inline pluss::Note arange(std::string_view start,
                          std::string_view end,
                          std::int64_t step = 1,
                          bool inclusive = true) {
  const auto flat = pluss::coden::factory_detail::char_arange(
      start, end, step, inclusive, "pluss::note::arange");
  return pluss::coden::factory_detail::make_note_from_owned_strings_1d(flat);
}

using WordSelectMode = pluss::coden::factory_detail::WordSelectMode;

inline pluss::Note perm(std::string_view word,
                        std::int64_t pick,
                        WordSelectMode mode = WordSelectMode::Combination,
                        bool with_replacement = false,
                        bool unique_results = true) {
  const auto flat = pluss::coden::factory_detail::word_select(
      word, pick, mode, with_replacement, unique_results);
  return pluss::coden::factory_detail::make_note_from_owned_strings_1d(flat);
}

inline pluss::Note full_bytes_like(const pluss::Note& like, std::string_view value) {
  PLUSS_CHECK(like.defined(), "pluss::note::full_bytes_like requires a defined Note");
  const auto sizes = like.sizes();
  const pluss::detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  const auto n = pluss::coden::factory_detail::checked_numel(shape);
  std::vector<std::string_view> flat(static_cast<std::size_t>(n), value);
  return pluss::coden::ops_internal::note_from_bytes(
      std::span<const std::string_view>(flat.data(), flat.size()), shape.span());
}

inline pluss::Note empty_bytes_like(const pluss::Note& like) {
  return full_bytes_like(like, std::string_view{});
}

// ---- Text-splitting factories ----

inline pluss::Note from_lines(std::string_view text, bool keep_ends = false, bool drop_empty = false) {
  auto flat = pluss::coden::factory_detail::split_lines_views(text, keep_ends, drop_empty);
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(flat.size())};
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), sizes);
}

inline pluss::Note from_words(std::string_view text) {
  auto flat = pluss::coden::factory_detail::split_words_views(text);
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(flat.size())};
  return pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(flat.data(), flat.size()), sizes);
}

inline pluss::Note format(std::string_view tpl,
                          std::initializer_list<Binding> bindings) {
  std::unordered_map<std::string_view, const Binding*> binding_map;
  binding_map.reserve(bindings.size());
  for (const auto& binding : bindings) {
    PLUSS_CHECK(!binding.name.empty(),
                "pluss::note::format: binding names must not be empty");
    const bool inserted = binding_map.emplace(std::string_view(binding.name), &binding).second;
    PLUSS_CHECK(inserted,
                "pluss::note::format: duplicate binding '" + binding.name + "'");
  }
  return make(detail::note_format_render(tpl, binding_map));
}

namespace detail {

inline bool select_scalar_like(const pluss::Series& value) {
  return value.numel() == 1;
}

inline bool select_scalar_like(const pluss::Note& value) {
  return value.numel() == 1;
}

template <class Value>
inline void select_capture_shape(std::vector<std::int64_t>& target_shape,
                                 bool& have_shape,
                                 const Value& value,
                                 std::string_view where,
                                 std::string_view label) {
  if (select_scalar_like(value)) {
    return;
  }
  const auto shape = value.sizes();
  const std::vector<std::int64_t> shape_vec(shape.begin(), shape.end());
  if (!have_shape) {
    target_shape = shape_vec;
    have_shape = true;
    return;
  }
  PLUSS_CHECK(shape_vec == target_shape,
              std::string(where) + ": " + std::string(label)
                  + " must be scalar-like or match the selected output shape");
}

inline std::int64_t select_numel(const std::vector<std::int64_t>& shape) {
  std::int64_t out = 1;
  for (const auto dim : shape) {
    out *= dim;
  }
  return out;
}

inline std::vector<std::int64_t> select_note_shape(
    std::string_view where,
    pluss::msharp::RangeView::Of<pluss::Series> conditions,
    pluss::msharp::RangeView::Of<pluss::Note> choices,
    const pluss::Note& default_value) {
  std::vector<std::int64_t> target_shape;
  bool have_shape = false;

  for (const auto& cond : conditions) {
    select_capture_shape(target_shape, have_shape, cond, where, "each condition");
  }
  for (const auto& choice : choices) {
    select_capture_shape(target_shape, have_shape, choice, where, "each choice");
  }
  select_capture_shape(target_shape, have_shape, default_value, where, "default value");

  return target_shape;
}

inline std::string_view select_note_value_at(std::span<const pluss::Note::value_type> flat,
                                             bool scalar_like,
                                             std::int64_t linear) {
  return flat[scalar_like ? 0 : static_cast<std::size_t>(linear)];
}

inline pluss::Note select_note_impl(std::string_view where,
                                    pluss::msharp::RangeView::Of<pluss::Series> conditions,
                                    pluss::msharp::RangeView::Of<pluss::Note> choices,
                                    const pluss::Note& default_value) {
  PLUSS_CHECK(!conditions.empty(), std::string(where) + ": at least one condition is required");
  PLUSS_CHECK(conditions.size() == choices.size(),
              std::string(where) + ": condition/choice count mismatch");
  PLUSS_CHECK(default_value.defined(), std::string(where) + ": default value is undefined");

  const auto target_shape = select_note_shape(where, conditions, choices, default_value);
  const auto target_numel = select_numel(target_shape);
  const bool default_scalar = select_scalar_like(default_value);
  const auto default_flat = default_value.vector();

  std::vector<std::string> out(static_cast<std::size_t>(target_numel));
  for (std::int64_t linear = 0; linear < target_numel; ++linear) {
    out[static_cast<std::size_t>(linear)] = select_note_value_at(default_flat, default_scalar, linear);
  }

  for (std::size_t i = conditions.size(); i > 0; --i) {
    const auto& cond = conditions[i - 1];
    const auto& choice = choices[i - 1];

    PLUSS_CHECK(cond.defined(), std::string(where) + ": condition is undefined");
    PLUSS_CHECK(choice.defined(), std::string(where) + ": choice is undefined");
    PLUSS_CHECK(cond.scalar_type() == pluss::ScalarType::Bool,
                std::string(where) + ": conditions must use Bool dtype");

    const bool cond_scalar = select_scalar_like(cond);
    const bool choice_scalar = select_scalar_like(choice);
    const auto choice_flat = choice.vector();
    const auto mask_flat = cond.reshape({cond.numel()}).to_vec<bool>();

    for (std::int64_t linear = 0; linear < target_numel; ++linear) {
      const bool take_choice = mask_flat[cond_scalar ? 0 : static_cast<std::size_t>(linear)];
      if (take_choice) {
        out[static_cast<std::size_t>(linear)] = select_note_value_at(choice_flat, choice_scalar, linear);
      }
    }
  }

  return make(out, target_shape);
}

} // namespace detail

inline pluss::Note select(pluss::msharp::RangeView::Of<pluss::Series> conditions,
                          pluss::msharp::RangeView::Of<pluss::Note> choices,
                          const pluss::Note& default_value) {
  return pluss::detail::select_core::select_range(
      "pluss::note::select",
      conditions,
      choices,
      default_value,
      [](std::string_view where,
         pluss::msharp::RangeView::Of<pluss::Series> conds,
         pluss::msharp::RangeView::Of<pluss::Note> picks,
         const pluss::Note& fallback) {
        return detail::select_note_impl(where, conds, picks, fallback);
      });
}

inline pluss::Note select(std::initializer_list<pluss::Series> conditions,
                          std::initializer_list<pluss::Note> choices,
                          const pluss::Note& default_value) {
  return pluss::detail::select_core::select_init(
      "pluss::note::select",
      conditions,
      choices,
      default_value,
      [](std::string_view where,
         pluss::msharp::RangeView::Of<pluss::Series> conds,
         pluss::msharp::RangeView::Of<pluss::Note> picks,
         const pluss::Note& fallback) {
        return detail::select_note_impl(where, conds, picks, fallback);
      });
}

inline pluss::Note select(const pluss::Series& condition,
                          const pluss::Note& choice,
                          const pluss::Note& default_value) {
  return pluss::detail::select_core::select_scalar(
      "pluss::note::select",
      condition,
      choice,
      default_value,
      [](std::string_view where,
         pluss::msharp::RangeView::Of<pluss::Series> conds,
         pluss::msharp::RangeView::Of<pluss::Note> picks,
         const pluss::Note& fallback) {
        return detail::select_note_impl(where, conds, picks, fallback);
      });
}

inline pluss::Note select(pluss::msharp::RangeView::Of<pluss::Series> conditions,
                          pluss::msharp::RangeView::Of<pluss::Note> choices,
                          std::string_view default_value) {
  return select(conditions, choices, make(default_value));
}

inline pluss::Note select(std::initializer_list<pluss::Series> conditions,
                          std::initializer_list<pluss::Note> choices,
                          std::string_view default_value) {
  return select(conditions, choices, make(default_value));
}

inline pluss::Note select(const pluss::Series& condition,
                          std::string_view choice,
                          std::string_view default_value) {
  return select(condition, make(choice), make(default_value));
}

// pluss::note::print(n) -- human-readable rendering, mirroring
// pluss::series::print/pluss::timeline::print. `std::cout << n` also works
// directly via pluss::coden's Note operator<<.
inline std::string print(const pluss::Note& n) {
  return pluss::coden::print_note(n);
}
inline std::string print(const pluss::Note& n, const pluss::NotePrintOptions& opt) {
  return pluss::coden::print_note(n, opt);
}
inline std::ostream& print(std::ostream& os, const pluss::Note& n,
                           const pluss::NotePrintOptions& opt = {}) {
  return pluss::coden::print_note(os, n, opt);
}

} // namespace pluss::note

// Keep the backward-compatible flat pluss::* spellings transitively
// available from note_factory.h. Kept in a separate file since the
// namespaced surface above plus the full compat mirror (12 make_note
// overloads, 7 bytes_content, 3 make_bytes_note, plus the shape/splitting
// factories) would make one file unwieldy.
#include "pluss/coden/note_factory_compat.h"
