#pragma once

#include <regex>
#include <string>
#include <string_view>

namespace pluss::coden {

// Regex is an engine hook: today it wraps std::regex (ECMAScript), but the
// surface is designed so we can swap to ICU/PCRE2 later without changing the
// rest of CodeN.
class Regex {
public:
  enum class Syntax {
    ECMAScript,
    Basic,
    Extended,
    Awk,
    Grep,
    Egrep,
  };

  struct Options {
    Syntax syntax = Syntax::ECMAScript;
    bool icase = false;
  };

  Regex() = default;
  explicit Regex(std::string pattern) : Regex(std::move(pattern), Options{}) {}
  Regex(std::string pattern, Options opt);

  bool match(std::string_view s) const;
  bool search(std::string_view s) const;
  std::string replace(std::string_view s, std::string_view repl) const;

  const std::string& pattern() const noexcept { return pattern_; }

private:
  std::regex_constants::syntax_option_type to_std_syntax(Options opt);

  std::string pattern_;
  std::regex re_;
};

} // namespace pluss::coden
