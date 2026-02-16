#include "CodeN/core/Regex.h"

namespace pluss::coden {

std::regex_constants::syntax_option_type Regex::to_std_syntax(Options opt) {
  using namespace std::regex_constants;
  syntax_option_type s = ECMAScript;
  switch (opt.syntax) {
    case Syntax::ECMAScript: s = ECMAScript; break;
    case Syntax::Basic: s = basic; break;
    case Syntax::Extended: s = extended; break;
    case Syntax::Awk: s = awk; break;
    case Syntax::Grep: s = grep; break;
    case Syntax::Egrep: s = egrep; break;
  }
  if (opt.icase) s |= icase;
  return s;
}

Regex::Regex(std::string pattern, Options opt)
    : pattern_(std::move(pattern)), re_(pattern_, to_std_syntax(opt)) {}

bool Regex::match(std::string_view s) const {
  return std::regex_match(std::string(s), re_);
}

bool Regex::search(std::string_view s) const {
  return std::regex_search(std::string(s), re_);
}

std::string Regex::replace(std::string_view s, std::string_view repl) const {
  return std::regex_replace(std::string(s), re_, std::string(repl));
}

} // namespace pluss::coden
