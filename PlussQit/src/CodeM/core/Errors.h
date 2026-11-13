#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <functional>
#include <typeinfo>
#if defined(__APPLE__) || defined(__unix__)
#include <cxxabi.h>
#endif
#include <cstdio>
#include <memory>
#include <mutex>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <string_view>
#include <type_traits>
#include <utility>

#include "CodeM/core/StackTrace.h"

// pluss::exception: the error object and everything that makes it informative (tracebacks, chaining, notes,
// groups, "Did you mean"). pluss::Error and pluss::ErrorGroup are aliases of the types declared here.
namespace pluss::exception {

// Internal helpers behind Error (suggestions and name vocabularies); not part of the documented surface.

// The closest of `options` to `given`, or an empty string when nothing is close (Python's "Did you
// mean" rule: a few edits at most, fewer for short words, case differences count little).
inline std::string closest_match(const std::string& given, const std::vector<std::string>& options) {
  auto distance = [](const std::string& a, const std::string& b) {
    std::vector<std::size_t> prev(b.size() + 1), cur(b.size() + 1);
    for (std::size_t j = 0; j <= b.size(); ++j) prev[j] = j * 2;
    for (std::size_t i = 1; i <= a.size(); ++i) {
      cur[0] = i * 2;
      for (std::size_t j = 1; j <= b.size(); ++j) {
        const char ca = a[i - 1], cb = b[j - 1];
        const std::size_t sub = ca == cb ? 0 : (std::tolower(static_cast<unsigned char>(ca)) == std::tolower(static_cast<unsigned char>(cb)) ? 1 : 2);
        cur[j] = std::min({prev[j] + 2, cur[j - 1] + 2, prev[j - 1] + sub});
      }
      std::swap(prev, cur);
    }
    return prev[b.size()];
  };
  std::string best;
  std::size_t best_cost = static_cast<std::size_t>(-1);
  for (const auto& option : options) {
    if (option == given) continue;
    const std::size_t limit = (given.size() + option.size() + 3) * 2 / 6;
    const std::size_t cost = distance(given, option);
    if (cost <= limit && cost < best_cost) {
      best = option;
      best_cost = cost;
    }
  }
  return best;
}

// "unknown <what> 'x'. Did you mean 'y'?  Choose one of: a, b, c" (the suggestion only when one is close).
inline std::string unknown_name_message(const std::string& what, const std::string& given, const std::vector<std::string>& options) {
  std::string msg = "unknown " + what + " '" + given + "'";
  const std::string near = closest_match(given, options);
  if (!near.empty()) msg += ". Did you mean '" + near + "'?";
  if (!options.empty()) {
    msg += "  Valid values: ";
    for (std::size_t i = 0; i < options.size(); ++i) msg += (i ? ", " : "") + options[i];
  }
  return msg;
}

// The message with a suggestion appended when `given` is close to one of `options`.
template <typename Options>
inline std::string with_suggestion(std::string msg, std::string_view given, const Options& options) {
  std::vector<std::string> names;
  for (const auto& option : options) names.emplace_back(option);
  const std::string near = closest_match(std::string(given), names);
  if (!near.empty()) msg += " Did you mean '" + near + "'?";
  return msg;
}
inline std::string with_suggestion(std::string msg) { return msg; }

// The keys of any map-like container as strings: the candidates for "Did you mean" at a failed lookup,
//   PLUSS_CHECK(it != names.end(), "unknown feature '" + key + "'", key, pluss::exception::map_keys(names));
template <typename Container>
inline std::vector<std::string> map_keys(const Container& container) {
  std::vector<std::string> keys;
  for (const auto& entry : container) {
    using Entry = std::decay_t<decltype(entry)>;
    if constexpr (requires { entry.first; }) {
      keys.emplace_back(entry.first);      // a map: its keys
    } else if constexpr (std::is_convertible_v<const Entry&, std::string_view>) {
      keys.emplace_back(entry);            // a list of names
    } else if constexpr (requires { entry.name; }) {
      keys.emplace_back(entry.name);       // a list of things that carry a name
    }
  }
  return keys;
}

// ---------------------------------------------------------------------------------------------------
// INTERNAL for now (not part of the documented public surface): pluss::names is for library authors.
// Suggestions. The error object finds "Did you mean" candidates itself: when a message names a value in
// quotes ("unknown catalog 'sale'"), it looks the value up in the vocabularies registered for the kind of
// thing the message names, and appends the closest valid one. Code that raises errors does nothing; the
// owner of a vocabulary declares it once:
//   static const bool kPalettes = pluss::exception::names::define("palette", {"thermal", "ocean", "forest"});
// and a container whose names change registers them for as long as it lives:
//   pluss::exception::names::Scope scope("catalog", [this] { return names(); });
// A raise site that already holds the candidates can still pass them: PLUSS_CHECK(cond, msg, given, options).
// ---------------------------------------------------------------------------------------------------
namespace names {

namespace detail {
struct Provider {
  std::string kind;  // lower case, e.g. "catalog", "store kind"
  std::function<std::vector<std::string>()> names;
  const void* owner{nullptr};
};
inline std::mutex& mutex() { static std::mutex m; return m; }
inline std::vector<Provider>& providers() { static std::vector<Provider> p; return p; }
inline std::unordered_map<const void*, Provider>& scopes() { static std::unordered_map<const void*, Provider> s; return s; }
inline std::string lower(std::string s) {
  for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}
}  // namespace detail

// Declare a fixed set of valid names for a kind of thing (an enum's spellings, the options of a parameter).
inline bool define(std::string kind, std::vector<std::string> names) {
  std::lock_guard<std::mutex> lock(detail::mutex());
  detail::providers().push_back({detail::lower(std::move(kind)), [n = std::move(names)] { return n; }, nullptr});
  return true;
}

// Declare the names a live object currently holds; they are offered while the object exists.
class Scope {
 public:
  Scope(std::string kind, std::function<std::vector<std::string>()> names) {
    std::lock_guard<std::mutex> lock(detail::mutex());
    detail::scopes()[this] = {detail::lower(std::move(kind)), std::move(names), this};
  }
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;
  ~Scope() {
    std::lock_guard<std::mutex> lock(detail::mutex());
    detail::scopes().erase(this);
  }
};

}  // namespace names


// The suggestion for an error message, without the caller doing anything: finds the first quoted value in
// the message and the registered vocabularies whose kind the message names, and returns the closest valid
// name, or an empty string.
inline std::string auto_suggestion(const std::string& message) {
  std::string given;
  for (const char quote : {'\'', '"'}) {
    const auto open = message.find(quote);
    if (open == std::string::npos) continue;
    const auto close = message.find(quote, open + 1);
    if (close == std::string::npos || close == open + 1) continue;
    given = message.substr(open + 1, close - open - 1);
    break;
  }
  if (given.empty()) return {};
  const std::string lowered = names::detail::lower(message);
  std::vector<std::string> candidates;
  {
    std::lock_guard<std::mutex> lock(names::detail::mutex());
    auto gather = [&](const names::detail::Provider& provider) {
      if (lowered.find(provider.kind) == std::string::npos) return;
      try {
        auto names = provider.names();
        candidates.insert(candidates.end(), std::make_move_iterator(names.begin()), std::make_move_iterator(names.end()));
      } catch (...) {
      }
    };
    for (const auto& provider : names::detail::providers()) gather(provider);
    for (const auto& entry : names::detail::scopes()) gather(entry.second);
  }
  return closest_match(given, candidates);
}

// Candidates a raise site may hand to the error when it holds them already (optional; the error finds
// its own through pluss::names when it is given none).
struct Hint {
  std::string given;
  std::vector<std::string> options;
  bool present{false};
};
inline Hint make_hint() { return {}; }
template <typename Options>
inline Hint make_hint(std::string_view given, const Options& options) {
  Hint hint;
  hint.given = std::string(given);
  for (const auto& option : options) hint.options.emplace_back(std::string(std::string_view(option)));
  hint.present = true;
  return hint;
}

// The text the error appends to its message: " Did you mean 'x'?" or nothing.
inline std::string suggestion_for(const std::string& message, const Hint& hint) {
  if (message.find("Did you mean") != std::string::npos) return {};
  const std::string near = hint.present ? closest_match(hint.given, hint.options) : auto_suggestion(message);
  return near.empty() ? std::string{} : " Did you mean '" + near + "'?";
}

// Every error the libraries raise. what() is a Python-style traceback: the frames from the program's
// main() down to the line that raised, each with its file, line, function and source text, then the
// error type and message. The frames are captured as raw addresses when the error is thrown (cheap) and
// are only resolved to files and lines the first time what() is read, so an error that is caught and
// handled costs almost nothing. Source lines appear when the program was built with debug information
// (-g); without it, the raising line is still exact and the other frames show function names.
// Set PLUSS_ERROR_TRACE=off to make what() the message alone.
class Error : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;  // a plain error: what() is the message

  Error(const std::string& message,
        std::vector<const void*> frames,
        const char* file,
        std::uint32_t line,
        const char* function,
        const char* source = nullptr,
        std::exception_ptr cause = nullptr,
        bool direct_cause = false,
        std::string suggestion = {})
      : std::runtime_error(message), trace_(std::make_shared<Trace>()) {
    trace_->suggestion = std::move(suggestion);
    trace_->source = source ? source : "";
    trace_->cause = std::move(cause);
    trace_->direct_cause = direct_cause;
    trace_->frames = std::move(frames);
    trace_->file = file ? file : "";
    trace_->line = line;
    trace_->function = function ? function : "";
  }

  // The exception that was being handled (inside a catch block) when this one was raised, or null.
  // what() prints it first, as Python does for "During handling of the above exception".
  [[nodiscard]] std::exception_ptr cause() const noexcept { return trace_ ? trace_->cause : nullptr; }

  // The message alone: no traceback.
  [[nodiscard]] std::string message() const {
    return trace_ ? std::string(std::runtime_error::what()) + trace_->suggestion : std::string(std::runtime_error::what());
  }

  // The traceback alone ("Traceback (most recent call last): ..."), empty for a plain error.
  [[nodiscard]] std::string traceback() const {
    if (!trace_) return {};
    std::call_once(trace_->once, [this] {
      trace_->text = ::pluss::codem::StackTrace::format_traceback(
          trace_->frames.data(), trace_->frames.size(), trace_->file.c_str(), trace_->line, trace_->function.c_str(),
          trace_->source.c_str());
    });
    return trace_->text;
  }

  // Attach extra context to an error that is being handled, and rethrow it with `throw;`
  // (Python's add_note). Notes print after the message; read what() again to see them.
  Error& add_note(std::string note) {
    if (!trace_) trace_ = std::make_shared<Trace>();
    std::lock_guard<std::mutex> lock(trace_->mutex);
    trace_->notes.push_back(std::move(note));
    trace_->what_valid = false;
    return *this;
  }
  [[nodiscard]] const std::vector<std::string>& notes() const noexcept {
    static const std::vector<std::string> none;
    return trace_ ? trace_->notes : none;
  }

  [[nodiscard]] const char* what() const noexcept override {
    if (!trace_) return std::runtime_error::what();
    try {
      const std::string tb = traceback();  // outside the lock: it has its own once flag
      std::lock_guard<std::mutex> lock(trace_->mutex);
      if (!trace_->what_valid) {
        std::string text;
        if (trace_->cause) {
          text = describe(trace_->cause) +
                 (trace_->direct_cause
                      ? "\n\nThe above exception was the direct cause of the following exception:\n\n"
                      : "\n\nDuring handling of the above exception, another exception occurred:\n\n");
        }
        trace_->what_text = text + tb + "pluss::Error: " + message();
        for (const auto& note : trace_->notes) trace_->what_text += "\n" + note;
        trace_->what_valid = true;
      }
      return trace_->what_text.c_str();
    } catch (...) {
      return std::runtime_error::what();
    }
  }

 public:
  // "Type: message" for any exception; a pluss::Error prints its whole traceback instead.
  static std::string describe(const std::exception_ptr& ptr) {
    try {
      std::rethrow_exception(ptr);
    } catch (const Error& e) {
      return e.what();
    } catch (const std::exception& e) {
      std::string type = typeid(e).name();
#if defined(__APPLE__) || defined(__unix__)
      int status = 0;
      if (char* name = abi::__cxa_demangle(type.c_str(), nullptr, nullptr, &status)) {
        if (status == 0) type = name;
        std::free(name);
      }
#endif
      return type + ": " + e.what();
    } catch (...) {
      return "unknown exception";
    }
  }

 protected:
  // The "During handling..." / "direct cause" text that precedes this error, or empty.
  [[nodiscard]] std::string chain_prefix() const {
    if (!trace_ || !trace_->cause) return {};
    return describe(trace_->cause) +
           (trace_->direct_cause
                ? "\n\nThe above exception was the direct cause of the following exception:\n\n"
                : "\n\nDuring handling of the above exception, another exception occurred:\n\n");
  }

 private:
  struct Trace {
    std::vector<const void*> frames;
    std::string file;
    std::uint32_t line{0};
    std::string function;
    bool direct_cause{false};  // raised "from" the cause on purpose, rather than while handling it
    std::exception_ptr cause;  // the exception being handled when this one was raised, if any
    std::string suggestion;  // " Did you mean 'x'?" or empty, decided by the error itself when raised
    std::string source;  // the text of the raising statement, kept so it can be shown without the source file
    std::once_flag once;
    std::string text;
    std::mutex mutex;
    bool what_valid{false};
    std::string what_text;
    std::vector<std::string> notes;
  };
  std::shared_ptr<Trace> trace_;
};

// Several errors raised together (Python's ExceptionGroup): each one keeps its own traceback.
//   what():
//     + Exception Group Traceback (most recent call last):
//     |   File "...", line N, in f
//     | pluss::ErrorGroup: message (2 sub-exceptions)
//     +-+---------------- 1 ----------------
//       | <first error, with its traceback>
//       +---------------- 2 ----------------
//       | <second error>
//       +------------------------------------
class ErrorGroup : public Error {
 public:
  ErrorGroup(const std::string& message,
             std::vector<std::exception_ptr> errors,
             std::vector<const void*> frames,
             const char* file,
             std::uint32_t line,
             const char* function,
             const char* source = nullptr,
             std::exception_ptr cause = nullptr)
      : Error(message, std::move(frames), file, line, function, source, std::move(cause)),
        errors_(std::make_shared<std::vector<std::exception_ptr>>(std::move(errors))),
        text_(std::make_shared<Text>()) {}

  [[nodiscard]] const std::vector<std::exception_ptr>& errors() const noexcept { return *errors_; }

  [[nodiscard]] const char* what() const noexcept override {
    try {
      std::lock_guard<std::mutex> lock(text_->mutex);
      if (!text_->valid) {
        auto indent = [](const std::string& body, const std::string& prefix) {
          std::string out;
          std::size_t start = 0;
          while (start <= body.size()) {
            const auto end = body.find('\n', start);
            const std::string line = body.substr(start, end == std::string::npos ? std::string::npos : end - start);
            out += prefix + line + "\n";
            if (end == std::string::npos) break;
            start = end + 1;
          }
          while (out.size() > 1 && out[out.size() - 1] == '\n' && out[out.size() - 2] == '\n') out.pop_back();
          return out;
        };
        std::string text = chain_prefix();
        std::string frames = traceback();
        const auto header_end = frames.find('\n');
        if (header_end != std::string::npos) frames = frames.substr(header_end + 1);  // drop "Traceback ..." line
        text += "  + Exception Group Traceback (most recent call last):\n";
        text += indent(frames + "pluss::ErrorGroup: " + message() + " (" + std::to_string(errors_->size()) +
                           (errors_->size() == 1 ? " sub-exception)" : " sub-exceptions)"),
                       "  | ");
        for (std::size_t i = 0; i < errors_->size(); ++i) {
          text += (i == 0 ? "  +-+---------------- " : "    +---------------- ") + std::to_string(i + 1) + " ----------------\n";
          text += indent(describe((*errors_)[i]), "    | ");
        }
        text += "    +------------------------------------";
        text_->what = std::move(text);
        text_->valid = true;
      }
      return text_->what.c_str();
    } catch (...) {
      return Error::what();
    }
  }

 private:
  struct Text {
    std::mutex mutex;
    bool valid{false};
    std::string what;
  };
  std::shared_ptr<std::vector<std::exception_ptr>> errors_;
  std::shared_ptr<Text> text_;
};


} // namespace pluss::exception

namespace pluss {
using Error = exception::Error;
using ErrorGroup = exception::ErrorGroup;
} // namespace pluss

namespace pluss::codem {

using Error = ::pluss::Error;

namespace detail {

inline void oss_args(std::ostringstream&) {}

template <typename First, typename... Rest>
inline void oss_args(std::ostringstream& oss, First&& first, Rest&&... rest) {
  oss << std::forward<First>(first);
  if constexpr (sizeof...(rest) > 0) {
    oss << " ";
    oss_args(oss, std::forward<Rest>(rest)...);
  }
}

} // namespace detail

inline bool trace_enabled_() {
  const char* raw = std::getenv("PLUSS_ERROR_TRACE");
  if (!raw || *raw == '\0') return true;
  return !(std::strcmp(raw, "0") == 0 || std::strcmp(raw, "off") == 0 || std::strcmp(raw, "OFF") == 0 ||
           std::strcmp(raw, "false") == 0 || std::strcmp(raw, "FALSE") == 0);
}

inline std::vector<const void*> capture_frames_() {
  void* raw[256];
  const std::size_t n = StackTrace::capture_addresses(raw, 256);
  return std::vector<const void*>(raw, raw + n);
}

[[noreturn]] inline void throw_error(
    const std::string& msg,
    const std::source_location& loc = std::source_location::current(),
    const char* source = nullptr,
    const ::pluss::exception::Hint& hint = {}) {
  const std::string suggestion = ::pluss::exception::suggestion_for(msg, hint);
  if (!trace_enabled_()) {
    throw Error(msg + suggestion);
  }
  throw Error(msg, capture_frames_(), loc.file_name(), loc.line(), loc.function_name(), source, std::current_exception(), false, suggestion);
}

// Raise an error that is the direct result of another one (Python's "raise ... from ..."): the cause is
// printed first, joined by "The above exception was the direct cause of the following exception:".
[[noreturn]] inline void throw_error_from(
    const std::string& msg,
    std::exception_ptr cause,
    const std::source_location& loc = std::source_location::current(),
    const char* source = nullptr) {
  const std::string suggestion = ::pluss::exception::suggestion_for(msg, ::pluss::exception::Hint{});
  if (!trace_enabled_()) {
    throw Error(msg + suggestion);
  }
  throw Error(msg, capture_frames_(), loc.file_name(), loc.line(), loc.function_name(), source, std::move(cause), true, suggestion);
}

// Raise an error without the exception being handled (Python's "raise ... from None"): it is a new
// error, and the one being handled is not printed with it.
[[noreturn]] inline void throw_error_no_context(
    const std::string& msg,
    const std::source_location& loc = std::source_location::current(),
    const char* source = nullptr) {
  const std::string suggestion = ::pluss::exception::suggestion_for(msg, ::pluss::exception::Hint{});
  if (!trace_enabled_()) {
    throw Error(msg + suggestion);
  }
  throw Error(msg, capture_frames_(), loc.file_name(), loc.line(), loc.function_name(), source, nullptr, false, suggestion);
}

// Raise several errors as one group (Python's ExceptionGroup).
[[noreturn]] inline void throw_error_group(
    const std::string& msg,
    std::vector<std::exception_ptr> errors,
    const std::source_location& loc = std::source_location::current(),
    const char* source = nullptr) {
  throw ::pluss::exception::ErrorGroup(msg, std::move(errors), capture_frames_(), loc.file_name(), loc.line(), loc.function_name(), source,
                   std::current_exception());
}

inline void warn(
    const std::string& msg,
    const std::source_location& loc = std::source_location::current()) {
  std::string full;
  if (trace_enabled_()) {
    const auto frames = capture_frames_();
    full = StackTrace::format_traceback(frames.data(), frames.size(), loc.file_name(), loc.line(), loc.function_name());
  }
  full += "pluss warning: " + msg;
  std::fwrite(full.data(), 1, full.size(), stderr);
  std::fwrite("\n", 1, 1, stderr);
}

// Torch-like check macro for internal invariants.
#define PLUSS_CHECK(cond, msg, ...) \
  do {                         \
    if (!(cond)) {             \
      ::pluss::codem::throw_error((msg), std::source_location::current(), "PLUSS_CHECK(" #cond ", " #msg ")", ::pluss::exception::make_hint(__VA_ARGS__)); \
    }                          \
  } while (false)

// Fail immediately with an error message. Optionally name the offending value and the valid ones,
//   PLUSS_FAIL("unknown palette 'thermel'", "thermel", palettes)
// and the message gains "Did you mean 'thermal'?" when one of them is close. Works the same on
// PLUSS_CHECK(cond, msg, given, options).
#define PLUSS_FAIL(msg, ...)   \
  do {                         \
    ::pluss::codem::throw_error((msg), std::source_location::current(), "PLUSS_FAIL(" #msg ")", ::pluss::exception::make_hint(__VA_ARGS__)); \
  } while (false)

// Fail with a message, naming the exception that caused it (a std::exception_ptr, e.g. from
// std::current_exception() inside a catch block).
#define PLUSS_FAIL_FROM(msg, cause) \
  do { \
    ::pluss::codem::throw_error_from((msg), (cause), std::source_location::current(), "PLUSS_FAIL_FROM(" #msg ", " #cause ")"); \
  } while (false)

// Fail with a message and no context: the exception being handled, if any, is not attached.
#define PLUSS_FAIL_NO_CONTEXT(msg) \
  do { \
    ::pluss::codem::throw_error_no_context((msg), std::source_location::current(), "PLUSS_FAIL_NO_CONTEXT(" #msg ")"); \
  } while (false)

// Fail with several errors grouped (a std::vector<std::exception_ptr>).
#define PLUSS_FAIL_GROUP(msg, errors) \
  do { \
    ::pluss::codem::throw_error_group((msg), (errors), std::source_location::current(), "PLUSS_FAIL_GROUP(" #msg ", " #errors ")"); \
  } while (false)

// Fail with "unknown <what> 'given'. Did you mean '...'?" listing the valid values.
#define PLUSS_FAIL_UNKNOWN(what, given, options) \
  do { \
    ::pluss::codem::throw_error(::pluss::exception::unknown_name_message((what), (given), (options)), std::source_location::current(), "PLUSS_FAIL_UNKNOWN(" #what ", " #given ", " #options ")"); \
  } while (false)

// Warn to stderr with call-site context and stack trace when available.
#define PLUSS_WARN(msg)        \
  do {                         \
    ::pluss::codem::warn((msg)); \
  } while (false)

// Equality check with single evaluation of both operands.
#define PLUSS_CHECK_EQ(a, b)   \
  do {                         \
    const auto& _pluss_lhs = (a); \
    const auto& _pluss_rhs = (b); \
    if (!(_pluss_lhs == _pluss_rhs)) { \
      std::ostringstream _pluss_oss; \
      _pluss_oss << "Expected equality but got (" \
                 << #a << " = " << _pluss_lhs << ", " \
                 << #b << " = " << _pluss_rhs << ")"; \
      ::pluss::codem::throw_error(_pluss_oss.str(), std::source_location::current(), "PLUSS_CHECK_EQ(" #a ", " #b ")"); \
    } \
  } while (false)

// Variadic condition check that streams arbitrary context pieces.
#define PLUSS_INSPECT(cond, ...) \
  do { \
    if (!(cond)) { \
      std::ostringstream _pluss_oss; \
      ::pluss::codem::detail::oss_args(_pluss_oss __VA_OPT__(,) __VA_ARGS__); \
      ::pluss::codem::throw_error(_pluss_oss.str(), std::source_location::current(), "PLUSS_INSPECT(" #cond __VA_OPT__(", " #__VA_ARGS__) ")"); \
    } \
  } while (false)

} // namespace pluss::codem

namespace pluss {

[[noreturn]] inline void throw_error(
    const std::string& msg,
    const std::source_location& loc = std::source_location::current()) {
  ::pluss::codem::throw_error(msg, loc);
}

inline void warn(
    const std::string& msg,
    const std::source_location& loc = std::source_location::current()) {
  ::pluss::codem::warn(msg, loc);
}

} // namespace pluss

#define PLUSS_CALL_GUARD() ::pluss::codem::CallGuard PLUSS__call_guard_{}
