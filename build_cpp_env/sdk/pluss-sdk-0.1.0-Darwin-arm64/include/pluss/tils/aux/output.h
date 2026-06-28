#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <mutex>
#include <optional>
#include <ostream>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#if PLUSSQIT_HAS_FMT
#include <fmt/format.h>
#endif

namespace pluss::aux {

enum class OutputLevel : std::size_t {
  Trace = 0,
  Debug = 1,
  Info = 2,
  Warn = 3,
  Error = 4
};

struct OutputOptions {
  std::string separator{" "};
  bool levels_enabled{true};
  std::array<std::string, 5> level_prefixes{{"[TRACE]", "[DEBUG]", "[INFO]", "[WARNING]", "[ERROR]"}};
  OutputLevel min_level{OutputLevel::Trace};
  bool auto_flush{false};
};

namespace detail {

template <class>
inline constexpr bool always_false_v = false;

template <class T>
concept stream_insertable = requires(std::ostream& os, const T& v) {
  { os << v } -> std::same_as<std::ostream&>;
};

template <class T>
concept adl_output_renderable = requires(std::ostream& os, const T& v) {
  output_render(os, v);
};

template <class T>
concept member_show_renderable = requires(std::ostream& os, const T& v) {
  v.show(os);
};

template <class T>
concept member_to_string_renderable = requires(const T& v) {
  v.to_string();
};

template <class T>
concept string_like = std::is_convertible_v<T, std::string_view>;

template <class T>
struct is_optional : std::false_type {};

template <class U>
struct is_optional<std::optional<U>> : std::true_type {};

template <class T>
inline constexpr bool is_optional_v = is_optional<std::remove_cvref_t<T>>::value;

template <class T>
struct is_variant : std::false_type {};

template <class... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};

template <class T>
inline constexpr bool is_variant_v = is_variant<std::remove_cvref_t<T>>::value;

template <class T>
struct is_pair : std::false_type {};

template <class A, class B>
struct is_pair<std::pair<A, B>> : std::true_type {};

template <class T>
inline constexpr bool is_pair_v = is_pair<std::remove_cvref_t<T>>::value;

template <class T, class = void>
struct is_tuple_like : std::false_type {};

template <class T>
struct is_tuple_like<T, std::void_t<decltype(std::tuple_size<std::remove_cvref_t<T>>::value)>> : std::true_type {};

template <class T>
inline constexpr bool is_tuple_like_v = is_tuple_like<T>::value;

template <class T>
concept map_like = std::ranges::range<T> &&
    requires {
      typename std::remove_cvref_t<T>::key_type;
      typename std::remove_cvref_t<T>::mapped_type;
    };

template <class T>
concept plain_range_like = std::ranges::range<T> && !string_like<T> && !map_like<T>;

template <class T>
void render_value(std::ostream& os, const T& value);

template <class Tuple, std::size_t... I>
void render_tuple(std::ostream& os, const Tuple& value, std::index_sequence<I...>) {
  os << "(";
  bool first = true;
  auto emit = [&](const auto& item) {
    if (!first) os << ", ";
    first = false;
    render_value(os, item);
  };
  (emit(std::get<I>(value)), ...);
  os << ")";
}

template <class T>
void render_value(std::ostream& os, const T& value) {
  if constexpr (string_like<T>) {
    os << value;
  } else if constexpr (is_optional_v<T>) {
    if (value.has_value()) {
      render_value(os, *value);
    } else {
      os << "nullopt";
    }
  } else if constexpr (is_variant_v<T>) {
    std::visit([&](const auto& v) { render_value(os, v); }, value);
  } else if constexpr (is_pair_v<T>) {
    os << "(";
    render_value(os, value.first);
    os << ", ";
    render_value(os, value.second);
    os << ")";
  } else if constexpr (is_tuple_like_v<T> && !is_pair_v<T> && !std::ranges::range<T>) {
    constexpr auto n = std::tuple_size_v<std::remove_cvref_t<T>>;
    render_tuple(os, value, std::make_index_sequence<n>{});
  } else if constexpr (map_like<T>) {
    os << "{";
    bool first = true;
    for (const auto& [k, v] : value) {
      if (!first) os << ", ";
      first = false;
      render_value(os, k);
      os << ": ";
      render_value(os, v);
    }
    os << "}";
  } else if constexpr (plain_range_like<T>) {
    os << "[";
    bool first = true;
    for (const auto& item : value) {
      if (!first) os << ", ";
      first = false;
      render_value(os, item);
    }
    os << "]";
  } else if constexpr (adl_output_renderable<T>) {
    output_render(os, value);
  } else if constexpr (member_show_renderable<T>) {
    value.show(os);
  } else if constexpr (member_to_string_renderable<T>) {
    render_value(os, value.to_string());
  } else if constexpr (stream_insertable<T>) {
    os << value;
  } else {
    static_assert(always_false_v<T>,
                  "OutputEngine: value is not stream-insertable and has no supported container rendering path");
  }
}

template <class T>
std::string render_to_string(const T& value) {
  std::ostringstream oss;
  render_value(oss, value);
  return oss.str();
}

inline std::string simple_brace_format(std::string_view fmt, std::initializer_list<std::string> args) {
  std::string out;
  out.reserve(fmt.size() + 32);

  auto it = args.begin();
  const auto end = args.end();

  for (std::size_t i = 0; i < fmt.size();) {
    const char c = fmt[i];
    if (c == '{') {
      if (i + 1 < fmt.size() && fmt[i + 1] == '{') {
        out.push_back('{');
        i += 2;
        continue;
      }
      if (i + 1 < fmt.size() && fmt[i + 1] == '}') {
        if (it != end) {
          out += *it++;
        } else {
          out += "{}";
        }
        i += 2;
        continue;
      }
    }
    if (c == '}' && i + 1 < fmt.size() && fmt[i + 1] == '}') {
      out.push_back('}');
      i += 2;
      continue;
    }
    out.push_back(c);
    ++i;
  }
  return out;
}

#if PLUSSQIT_HAS_FMT
template <class T>
decltype(auto) fmt_arg(T&& value) {
  using U = std::remove_cvref_t<T>;
  if constexpr (fmt::is_formattable<U, char>::value) {
    return std::forward<T>(value);
  } else {
    return render_to_string(value);
  }
}
#endif

} // namespace detail

class OutputEngine {
public:
  explicit OutputEngine(std::ostream& os = std::cout) noexcept
      : sink_(&os) {}

  explicit OutputEngine(OutputOptions opts)
      : sink_(&std::cout) {
    apply_options_unlocked(std::move(opts));
  }

  OutputEngine(std::ostream& os, OutputOptions opts)
      : sink_(&os) {
    apply_options_unlocked(std::move(opts));
  }

  OutputEngine(const OutputEngine&) = delete;
  OutputEngine& operator=(const OutputEngine&) = delete;

  OutputEngine& to(std::ostream& os) noexcept {
    std::scoped_lock lock(mu_);
    sink_ = &os;
    return *this;
  }

  OutputEngine& sep(std::string s) {
    std::scoped_lock lock(mu_);
    separator_ = std::move(s);
    return *this;
  }

  OutputEngine& prefix(OutputLevel level, std::string p) {
    std::scoped_lock lock(mu_);
    prefixes_[to_index(level)] = std::move(p);
    return *this;
  }

  OutputEngine& enable_levels(bool enabled = true) noexcept {
    std::scoped_lock lock(mu_);
    levels_enabled_ = enabled;
    return *this;
  }

  OutputEngine& set_options(OutputOptions opts) {
    std::scoped_lock lock(mu_);
    apply_options_unlocked(std::move(opts));
    return *this;
  }

  [[nodiscard]] OutputOptions options() const {
    std::scoped_lock lock(mu_);
    OutputOptions opts;
    opts.separator = separator_;
    opts.levels_enabled = levels_enabled_;
    opts.level_prefixes = prefixes_;
    opts.min_level = min_level_;
    opts.auto_flush = auto_flush_;
    return opts;
  }

  template <class... Args>
  OutputEngine& write(const Args&... args) {
    std::scoped_lock lock(mu_);
    write_locked(args...);
    return *this;
  }

  template <class... Args>
  OutputEngine& writeln(const Args&... args) {
    std::scoped_lock lock(mu_);
    write_locked(args...);
    (*sink_) << '\n';
    if (auto_flush_) sink_->flush();
    return *this;
  }

  template <class... Args>
  OutputEngine& writef(std::string_view pattern, Args&&... args) {
    const std::string text = format_message(pattern, std::forward<Args>(args)...);
    std::scoped_lock lock(mu_);
    (*sink_) << text;
    return *this;
  }

  template <class... Args>
  OutputEngine& writefln(std::string_view pattern, Args&&... args) {
    const std::string text = format_message(pattern, std::forward<Args>(args)...);
    std::scoped_lock lock(mu_);
    (*sink_) << text << '\n';
    if (auto_flush_) sink_->flush();
    return *this;
  }

  OutputEngine& endln() {
    std::scoped_lock lock(mu_);
    (*sink_) << '\n';
    if (auto_flush_) sink_->flush();
    return *this;
  }

  OutputEngine& flush() {
    std::scoped_lock lock(mu_);
    sink_->flush();
    return *this;
  }

  template <class... Args>
  OutputEngine& log(OutputLevel level, const Args&... args) {
    std::scoped_lock lock(mu_);
    if (!should_log_locked(level)) return *this;
    write_level_prefix_locked(level);
    write_locked(args...);
    (*sink_) << '\n';
    if (auto_flush_) sink_->flush();
    return *this;
  }

  template <class... Args>
  OutputEngine& logf(OutputLevel level, std::string_view pattern, Args&&... args) {
    const std::string text = format_message(pattern, std::forward<Args>(args)...);
    std::scoped_lock lock(mu_);
    if (!should_log_locked(level)) return *this;
    write_level_prefix_locked(level);
    (*sink_) << text << '\n';
    if (auto_flush_) sink_->flush();
    return *this;
  }

  template <class... Args>
  OutputEngine& trace(const Args&... args) {
    return log(OutputLevel::Trace, args...);
  }

  template <class... Args>
  OutputEngine& debug(const Args&... args) {
    return log(OutputLevel::Debug, args...);
  }

  template <class... Args>
  OutputEngine& info(const Args&... args) {
    return log(OutputLevel::Info, args...);
  }

  template <class... Args>
  OutputEngine& warn(const Args&... args) {
    return log(OutputLevel::Warn, args...);
  }

  template <class... Args>
  OutputEngine& error(const Args&... args) {
    return log(OutputLevel::Error, args...);
  }

  template <class... Args>
  OutputEngine& tracef(std::string_view pattern, Args&&... args) {
    return logf(OutputLevel::Trace, pattern, std::forward<Args>(args)...);
  }

  template <class... Args>
  OutputEngine& debugf(std::string_view pattern, Args&&... args) {
    return logf(OutputLevel::Debug, pattern, std::forward<Args>(args)...);
  }

  template <class... Args>
  OutputEngine& infof(std::string_view pattern, Args&&... args) {
    return logf(OutputLevel::Info, pattern, std::forward<Args>(args)...);
  }

  template <class... Args>
  OutputEngine& warnf(std::string_view pattern, Args&&... args) {
    return logf(OutputLevel::Warn, pattern, std::forward<Args>(args)...);
  }

  template <class... Args>
  OutputEngine& errorf(std::string_view pattern, Args&&... args) {
    return logf(OutputLevel::Error, pattern, std::forward<Args>(args)...);
  }

private:
  template <class... Args>
  void write_locked(const Args&... args) {
    bool first = true;
    auto emit = [&](const auto& value) {
      if (!first) (*sink_) << separator_;
      first = false;
      detail::render_value(*sink_, value);
    };
    (emit(args), ...);
  }

  void write_level_prefix_locked(OutputLevel level) {
    if (!levels_enabled_) return;
    (*sink_) << prefixes_[to_index(level)] << " ";
  }

  bool should_log_locked(OutputLevel level) const noexcept {
    return to_index(level) >= to_index(min_level_);
  }

  void apply_options_unlocked(OutputOptions opts) {
    separator_ = std::move(opts.separator);
    levels_enabled_ = opts.levels_enabled;
    prefixes_ = std::move(opts.level_prefixes);
    min_level_ = opts.min_level;
    auto_flush_ = opts.auto_flush;
  }

  static constexpr std::size_t to_index(OutputLevel level) {
    return static_cast<std::size_t>(level);
  }

  template <class... Args>
  static std::string format_message(std::string_view pattern, Args&&... args) {
#if PLUSSQIT_HAS_FMT
    return fmt::format(fmt::runtime(pattern), detail::fmt_arg(std::forward<Args>(args))...);
#else
    return detail::simple_brace_format(
        pattern, {detail::render_to_string(std::forward<Args>(args))...});
#endif
  }

  std::ostream* sink_{nullptr};
  mutable std::mutex mu_;
  std::string separator_{" "};
  std::array<std::string, 5> prefixes_{{"[TRACE]", "[DEBUG]", "[INFO]", "[WARNING]", "[ERROR]"}};
  bool levels_enabled_{true};
  OutputLevel min_level_{OutputLevel::Trace};
  bool auto_flush_{false};
};

} // namespace pluss::aux

namespace pluss {
using aux::OutputEngine;
using aux::OutputOptions;
using aux::OutputLevel;
} // namespace pluss
