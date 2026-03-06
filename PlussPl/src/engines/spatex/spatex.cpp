#include "plotism/spatex/spatex.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

namespace pluss::pl::plotism::spatex {
namespace {

struct Utf8AsciiReplacement {
  std::string_view utf8;
  std::string_view ascii;
};

constexpr auto kUtf8AsciiMathFallbacks = std::to_array<Utf8AsciiReplacement>({
    {"\xE2\x89\xA4", "<="},       // <=
    {"\xE2\x89\xA5", ">="},       // >=
    {"\xE2\x89\xA0", "!="},       // !=
    {"\xE2\x89\x88", "~="},       // ~=
    {"\xE2\x88\x92", "-"},        // -
    {"\xE2\x88\x9A", "sqrt"},     // sqrt
    {"\xE2\x88\x9E", "inf"},      // inf
    {"\xE2\x88\x82", "d"},        // d
    {"\xE2\x8B\x85", "*"},        // *
    {"\xE2\x88\x99", "*"},        // *
    {"\xC3\x97", "x"},            // x
    {"\xC2\xB1", "+/-"},          // +/-
    {"\xCE\x93", "Gamma"},        // Gamma
    {"\xCE\x94", "Delta"},        // Delta
    {"\xCE\x9B", "Lambda"},       // Lambda
    {"\xCE\xA3", "Sigma"},        // Sigma
    {"\xCE\xA6", "Phi"},          // Phi
    {"\xCE\xA8", "Psi"},          // Psi
    {"\xCE\xA9", "Omega"},        // Omega
    {"\xCE\xB1", "alpha"},        // alpha
    {"\xCE\xB2", "beta"},         // beta
    {"\xCE\xB3", "gamma"},        // gamma
    {"\xCE\xB4", "delta"},        // delta
    {"\xCE\xB5", "epsilon"},      // epsilon
    {"\xCE\xB6", "zeta"},         // zeta
    {"\xCE\xB7", "eta"},          // eta
    {"\xCE\xB8", "theta"},        // theta
    {"\xCE\xBA", "kappa"},        // kappa
    {"\xCE\xBB", "lambda"},       // lambda
    {"\xCE\xBC", "mu"},           // mu
    {"\xCE\xBD", "nu"},           // nu
    {"\xCE\xBE", "xi"},           // xi
    {"\xCF\x80", "pi"},           // pi
    {"\xCF\x81", "rho"},          // rho
    {"\xCF\x82", "sigma"},        // sigma
    {"\xCF\x83", "sigma"},        // sigma
    {"\xCF\x84", "tau"},          // tau
    {"\xCF\x86", "phi"},          // phi
    {"\xCF\x87", "chi"},          // chi
    {"\xCF\x88", "psi"},          // psi
    {"\xCF\x89", "omega"},        // omega
    {"\xE2\x86\x90", "<-"},       // ←
    {"\xE2\x86\x92", "->"},       // →
    {"\xE2\x86\x94", "<->"},      // ↔
    {"\xE2\x88\x88", "in"},       // ∈
    {"\xE2\x88\x89", "notin"},    // ∉
    {"\xE2\x8A\x82", "subset"},   // ⊂
    {"\xE2\x8A\x86", "subseteq"}, // ⊆
    {"\xE2\x8A\x83", "supset"},   // ⊃
    {"\xE2\x8A\x87", "supseteq"}, // ⊇
    {"\xE2\x88\xAA", "cup"},      // ∪
    {"\xE2\x88\xA9", "cap"},      // ∩
    {"\xE2\x88\x91", "sum"},      // ∑
    {"\xE2\x88\x8F", "prod"},     // ∏
    {"\xE2\x88\xAB", "int"},      // ∫
    {"\xE2\x88\x87", "nabla"},    // ∇
    {"\xE2\x88\x80", "forall"},   // ∀
    {"\xE2\x88\x83", "exists"},   // ∃
    {"\xE2\x88\x9D", "prop"},     // ∝
    {"\xE2\x88\xBC", "~"},        // ∼
    {"\xE2\x82\x80", "_0"},       // _0
    {"\xE2\x82\x81", "_1"},       // _1
    {"\xE2\x82\x82", "_2"},       // _2
    {"\xE2\x82\x83", "_3"},       // _3
    {"\xE2\x82\x84", "_4"},       // _4
    {"\xE2\x82\x85", "_5"},       // _5
    {"\xE2\x82\x86", "_6"},       // _6
    {"\xE2\x82\x87", "_7"},       // _7
    {"\xE2\x82\x88", "_8"},       // _8
    {"\xE2\x82\x89", "_9"},       // _9
    {"\xE2\x82\x9C", "_t"},       // _t
    {"\xE2\x81\xB0", "^0"},       // ^0
    {"\xC2\xB9", "^1"},           // ^1
    {"\xC2\xB2", "^2"},           // ^2
    {"\xC2\xB3", "^3"},           // ^3
    {"\xE2\x81\xBB", "^-"},       // ^-
});

[[nodiscard]] bool looks_like_tex_markup(std::string_view text) noexcept {
  return text.find('\\') != std::string_view::npos || text.find('$') != std::string_view::npos;
}

[[nodiscard]] std::size_t tex_marker_count(std::string_view text) noexcept {
  std::size_t count = 0;
  for (const char c : text) {
    if (c == '\\' || c == '$') {
      ++count;
    }
  }
  return count;
}

void replace_all(std::string& text, std::string_view from, std::string_view to) {
  if (from.empty()) {
    return;
  }
  std::size_t pos = 0;
  while ((pos = text.find(from, pos)) != std::string::npos) {
    text.replace(pos, from.size(), to);
    pos += to.size();
  }
}

void apply_core_tex_symbol_replacements(std::string& text) {
  constexpr std::string_view kLeqUtf8 = "\xE2\x89\xA4";
  constexpr std::string_view kGeqUtf8 = "\xE2\x89\xA5";
  constexpr std::string_view kTimesUtf8 = "\xC3\x97";
  constexpr std::string_view kPlusMinusUtf8 = "\xC2\xB1";
  replace_all(text, "\\leq", kLeqUtf8);
  replace_all(text, "\\le", kLeqUtf8);
  replace_all(text, "\\geq", kGeqUtf8);
  replace_all(text, "\\ge", kGeqUtf8);
  replace_all(text, "\\times", kTimesUtf8);
  replace_all(text, "\\pm", kPlusMinusUtf8);
}

[[nodiscard]] std::size_t utf8_char_length_local(unsigned char lead) noexcept {
  if ((lead & 0x80U) == 0U) return 1;
  if ((lead & 0xE0U) == 0xC0U) return 2;
  if ((lead & 0xF0U) == 0xE0U) return 3;
  if ((lead & 0xF8U) == 0xF0U) return 4;
  return 1;
}

[[nodiscard]] bool is_ascii_alpha(char c) noexcept {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

[[nodiscard]] bool is_ascii_space(char c) noexcept {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

[[nodiscard]] std::string_view unicode_script_char(char marker, char ascii) noexcept {
  struct ScriptPair {
    char ascii;
    std::string_view utf8;
  };
  constexpr auto kSubscript = std::to_array<ScriptPair>({
      {'0', "\xE2\x82\x80"}, {'1', "\xE2\x82\x81"}, {'2', "\xE2\x82\x82"},
      {'3', "\xE2\x82\x83"}, {'4', "\xE2\x82\x84"}, {'5', "\xE2\x82\x85"},
      {'6', "\xE2\x82\x86"}, {'7', "\xE2\x82\x87"}, {'8', "\xE2\x82\x88"},
      {'9', "\xE2\x82\x89"}, {'+', "\xE2\x82\x8A"}, {'-', "\xE2\x82\x8B"},
      {'=', "\xE2\x82\x8C"}, {'(', "\xE2\x82\x8D"}, {')', "\xE2\x82\x8E"},
      {'a', "\xE2\x82\x90"}, {'e', "\xE2\x82\x91"}, {'h', "\xE2\x82\x95"},
      {'i', "\xE1\xB5\xA2"}, {'j', "\xE2\xB1\xBC"}, {'k', "\xE2\x82\x96"},
      {'l', "\xE2\x82\x97"}, {'m', "\xE2\x82\x98"}, {'n', "\xE2\x82\x99"},
      {'o', "\xE2\x82\x92"}, {'p', "\xE2\x82\x9A"}, {'r', "\xE1\xB5\xA3"},
      {'s', "\xE2\x82\x9B"}, {'t', "\xE2\x82\x9C"}, {'u', "\xE1\xB5\xA4"},
      {'v', "\xE1\xB5\xA5"}, {'x', "\xE2\x82\x93"},
  });
  constexpr auto kSuperscript = std::to_array<ScriptPair>({
      {'0', "\xE2\x81\xB0"}, {'1', "\xC2\xB9"}, {'2', "\xC2\xB2"},
      {'3', "\xC2\xB3"}, {'4', "\xE2\x81\xB4"}, {'5', "\xE2\x81\xB5"},
      {'6', "\xE2\x81\xB6"}, {'7', "\xE2\x81\xB7"}, {'8', "\xE2\x81\xB8"},
      {'9', "\xE2\x81\xB9"}, {'+', "\xE2\x81\xBA"}, {'-', "\xE2\x81\xBB"},
      {'=', "\xE2\x81\xBC"}, {'(', "\xE2\x81\xBD"}, {')', "\xE2\x81\xBE"},
      {'i', "\xE2\x81\xB1"}, {'n', "\xE2\x81\xBF"},
  });
  if (marker == '_') {
    const auto found = std::ranges::find_if(kSubscript, [ascii](const ScriptPair& pair) { return pair.ascii == ascii; });
    return found == kSubscript.end() ? std::string_view{} : found->utf8;
  }
  const auto found = std::ranges::find_if(kSuperscript, [ascii](const ScriptPair& pair) { return pair.ascii == ascii; });
  return found == kSuperscript.end() ? std::string_view{} : found->utf8;
}

[[nodiscard]] std::string collapse_inline_nested_scripts(std::string_view token) {
  std::string out;
  out.reserve(token.size() + 8);
  for (std::size_t i = 0; i < token.size();) {
    const char marker = token[i];
    if ((marker == '_' || marker == '^') && i + 1 < token.size()) {
      std::size_t j = i + 1;
      if (token[j] == '{') {
        std::size_t depth = 1;
        ++j;
        std::size_t start = j;
        while (j < token.size() && depth > 0) {
          if (token[j] == '{') {
            ++depth;
          } else if (token[j] == '}') {
            --depth;
          }
          ++j;
        }
        if (depth == 0) {
          const std::string inner = collapse_inline_nested_scripts(token.substr(start, (j - 1) - start));
          if (inner.size() == 1U) {
            if (const auto mapped = unicode_script_char(marker, inner.front()); !mapped.empty()) {
              out += mapped;
              i = j;
              continue;
            }
          }
          out.push_back(marker);
          out.push_back('{');
          out += inner;
          out.push_back('}');
          i = j;
          continue;
        }
      } else if (token[j] < 0x80U) {
        if (const auto mapped = unicode_script_char(marker, token[j]); !mapped.empty()) {
          out += mapped;
          i = j + 1;
          continue;
        }
      }
    }

    const auto len = std::min<std::size_t>(utf8_char_length_local(static_cast<unsigned char>(token[i])), token.size() - i);
    out.append(token.substr(i, len));
    i += len;
  }
  return out;
}

[[nodiscard]] bool should_accent_codepoint(std::string_view cp) noexcept {
  if (cp.empty()) {
    return false;
  }
  if (cp.size() == 1) {
    const char c = cp.front();
    if (is_ascii_space(c)) {
      return false;
    }
    constexpr std::string_view kPunct = ",.;:!?()[]{}<>|/\\+-*=^_";
    return kPunct.find(c) == std::string_view::npos;
  }
  return true;
}

[[nodiscard]] std::string apply_combining_accent(std::string_view text, std::string_view combining_mark) {
  if (combining_mark.empty()) {
    return std::string(text);
  }
  std::string out;
  out.reserve(text.size() + 8);
  for (std::size_t i = 0; i < text.size();) {
    const auto len = std::min<std::size_t>(
        utf8_char_length_local(static_cast<unsigned char>(text[i])),
        text.size() - i);
    const std::string_view cp = text.substr(i, len);
    out.append(cp);
    if (should_accent_codepoint(cp)) {
      out.append(combining_mark);
    }
    i += len;
  }
  return out;
}

[[nodiscard]] std::string_view accent_mark_for_command(std::string_view cmd) noexcept {
  if (cmd == "bar" || cmd == "overline") {
    return "\xCC\x84"; // COMBINING MACRON
  }
  if (cmd == "hat" || cmd == "widehat") {
    return "\xCC\x82"; // COMBINING CIRCUMFLEX ACCENT
  }
  if (cmd == "tilde" || cmd == "widetilde") {
    return "\xCC\x83"; // COMBINING TILDE
  }
  if (cmd == "dot") {
    return "\xCC\x87"; // COMBINING DOT ABOVE
  }
  if (cmd == "ddot") {
    return "\xCC\x88"; // COMBINING DIAERESIS
  }
  if (cmd == "vec") {
    return "\xE2\x83\x97"; // COMBINING RIGHT ARROW ABOVE
  }
  return {};
}

[[nodiscard]] std::string_view lookup_tex_command(std::string_view cmd) noexcept {
  constexpr auto kTexCommandMap = std::to_array<std::pair<std::string_view, std::string_view>>({
      {"alpha", "\xCE\xB1"},   {"beta", "\xCE\xB2"},    {"gamma", "\xCE\xB3"},   {"delta", "\xCE\xB4"},
      {"epsilon", "\xCE\xB5"}, {"zeta", "\xCE\xB6"},    {"eta", "\xCE\xB7"},     {"theta", "\xCE\xB8"},
      {"varepsilon", "\xCE\xB5"}, {"vartheta", "\xCE\xB8"},
      {"kappa", "\xCE\xBA"},   {"lambda", "\xCE\xBB"},  {"mu", "\xCE\xBC"},      {"nu", "\xCE\xBD"},
      {"xi", "\xCE\xBE"},      {"pi", "\xCF\x80"},      {"rho", "\xCF\x81"},     {"sigma", "\xCF\x83"},
      {"varpi", "\xCF\x80"},   {"varrho", "\xCF\x81"},  {"varsigma", "\xCF\x82"},
      {"tau", "\xCF\x84"},     {"phi", "\xCF\x86"},     {"chi", "\xCF\x87"},     {"psi", "\xCF\x88"},
      {"varphi", "\xCF\x86"},
      {"omega", "\xCF\x89"},   {"Gamma", "\xCE\x93"},   {"Delta", "\xCE\x94"},   {"Lambda", "\xCE\x9B"},
      {"Sigma", "\xCE\xA3"},   {"Pi", "\xCE\xA0"},      {"Phi", "\xCE\xA6"},     {"Psi", "\xCE\xA8"},
      {"Omega", "\xCE\xA9"},
      {"leq", "\xE2\x89\xA4"}, {"le", "\xE2\x89\xA4"},  {"geq", "\xE2\x89\xA5"}, {"ge", "\xE2\x89\xA5"},
      {"neq", "\xE2\x89\xA0"}, {"times", "\xC3\x97"},   {"cdot", "\xE2\x8B\x85"}, {"pm", "\xC2\xB1"},
      {"infty", "\xE2\x88\x9E"}, {"partial", "\xE2\x88\x82"},
      {"leftarrow", "\xE2\x86\x90"}, {"to", "\xE2\x86\x92"}, {"rightarrow", "\xE2\x86\x92"},
      {"leftrightarrow", "\xE2\x86\x94"},
      {"in", "\xE2\x88\x88"}, {"notin", "\xE2\x88\x89"},
      {"subset", "\xE2\x8A\x82"}, {"subseteq", "\xE2\x8A\x86"},
      {"supset", "\xE2\x8A\x83"}, {"supseteq", "\xE2\x8A\x87"},
      {"cup", "\xE2\x88\xAA"}, {"cap", "\xE2\x88\xA9"},
      {"sum", "\xE2\x88\x91"}, {"prod", "\xE2\x88\x8F"}, {"int", "\xE2\x88\xAB"},
      {"nabla", "\xE2\x88\x87"}, {"forall", "\xE2\x88\x80"}, {"exists", "\xE2\x88\x83"},
      {"approx", "\xE2\x89\x88"}, {"sim", "\xE2\x88\xBC"}, {"propto", "\xE2\x88\x9D"},
      {"ldots", "..."}, {"cdots", "\xE2\x8B\xAF"},
      {"sin", "sin"}, {"cos", "cos"}, {"tan", "tan"},
      {"sinh", "sinh"}, {"cosh", "cosh"}, {"tanh", "tanh"},
      {"log", "log"}, {"ln", "ln"}, {"exp", "exp"},
      {"min", "min"}, {"max", "max"}, {"argmin", "argmin"}, {"argmax", "argmax"},
  });
  const auto found = std::ranges::find_if(kTexCommandMap, [cmd](const auto& item) { return item.first == cmd; });
  if (found == kTexCommandMap.end()) {
    return {};
  }
  return found->second;
}

void skip_tex_spaces(std::string_view input, std::size_t& pos) {
  while (pos < input.size() && (input[pos] == ' ' || input[pos] == '\t' || input[pos] == '\n' || input[pos] == '\r')) {
    ++pos;
  }
}

constexpr char kStretchLeftMarker = '\x1D';
constexpr char kStretchRightMarker = '\x1E';

[[nodiscard]] std::string parse_left_right_delimiter(std::string_view input, std::size_t& pos) {
  skip_tex_spaces(input, pos);
  if (pos >= input.size()) {
    return {};
  }

  if (input[pos] == '.') {
    ++pos;
    return {};
  }

  if (input[pos] == '\\') {
    ++pos;
    std::size_t start = pos;
    while (pos < input.size() && is_ascii_alpha(input[pos])) {
      ++pos;
    }
    const std::string_view cmd = (start < pos) ? input.substr(start, pos - start) : std::string_view{};
    if (cmd.empty()) {
      if (pos < input.size()) {
        return std::string(1, input[pos++]);
      }
      return {};
    }

    if (cmd == "lbrace" || cmd == "rbrace") return "{}";
    if (cmd == "lbrack" || cmd == "rbrack") return "[]";
    if (cmd == "langle" || cmd == "rangle") return "<>";
    if (cmd == "lvert" || cmd == "rvert" || cmd == "vert" || cmd == "mid") return "|";
    if (cmd == "lVert" || cmd == "rVert" || cmd == "Vert" || cmd == "parallel") return "||";
    if (const auto mapped = lookup_tex_command(cmd); !mapped.empty()) {
      return std::string(mapped);
    }
    return std::string(cmd);
  }

  const auto len = std::min<std::size_t>(
      utf8_char_length_local(static_cast<unsigned char>(input[pos])),
      input.size() - pos);
  std::string token(input.substr(pos, len));
  pos += len;
  return token;
}

[[nodiscard]] std::string encode_stretch_delimiter(bool left, std::string token) {
  if (token.empty()) {
    return {};
  }
  if (token == "{}") {
    token = left ? "{" : "}";
  } else if (token == "[]") {
    token = left ? "[" : "]";
  } else if (token == "<>") {
    token = left ? "<" : ">";
  }
  std::string out;
  out.reserve(token.size() + 1);
  out.push_back(left ? kStretchLeftMarker : kStretchRightMarker);
  out += token;
  return out;
}

[[nodiscard]] std::string parse_tex_subset_expr(std::string_view input, std::size_t& pos, bool stop_on_brace);

[[nodiscard]] std::string parse_tex_subset_group(std::string_view input, std::size_t& pos) {
  skip_tex_spaces(input, pos);
  if (pos >= input.size()) {
    return {};
  }
  if (input[pos] == '{') {
    ++pos;
    auto inner = parse_tex_subset_expr(input, pos, true);
    if (pos < input.size() && input[pos] == '}') {
      ++pos;
    }
    return inner;
  }
  if (input[pos] == '\\') {
    ++pos;
    std::size_t start = pos;
    while (pos < input.size() && is_ascii_alpha(input[pos])) {
      ++pos;
    }
    const std::string_view cmd = (start < pos) ? input.substr(start, pos - start) : std::string_view{};
    if (!cmd.empty()) {
      if (const auto mapped = lookup_tex_command(cmd); !mapped.empty()) {
        return std::string(mapped);
      }
      if (cmd == "left") {
        return encode_stretch_delimiter(true, parse_left_right_delimiter(input, pos));
      }
      if (cmd == "right") {
        return encode_stretch_delimiter(false, parse_left_right_delimiter(input, pos));
      }
      return std::string(cmd);
    }
    if (pos < input.size()) {
      return std::string(1, input[pos++]);
    }
    return {};
  }
  const auto len = std::min<std::size_t>(utf8_char_length_local(static_cast<unsigned char>(input[pos])), input.size() - pos);
  const std::string token(input.substr(pos, len));
  pos += len;
  return token;
}

[[nodiscard]] std::string parse_tex_subset_command(std::string_view input, std::size_t& pos) {
  if (pos >= input.size() || input[pos] != '\\') {
    return {};
  }
  ++pos;
  std::size_t start = pos;
  while (pos < input.size() && is_ascii_alpha(input[pos])) {
    ++pos;
  }
  const std::string_view cmd = (start < pos) ? input.substr(start, pos - start) : std::string_view{};
  if (cmd.empty()) {
    if (pos < input.size()) {
      const char escaped = input[pos++];
      if (escaped == ',' || escaped == ';' || escaped == ':' || escaped == '!') {
        return " ";
      }
      if (escaped == '_' || escaped == '^' || escaped == '{' || escaped == '}' || escaped == '$' || escaped == '%') {
        return std::string(1, escaped);
      }
      return std::string(1, escaped);
    }
    return {};
  }

  if (cmd == "frac") {
    const std::string numer = parse_tex_subset_group(input, pos);
    const std::string denom = parse_tex_subset_group(input, pos);
    if (numer.empty() || denom.empty()) {
      return "frac";
    }
    // Emit as script-friendly sequence so renderers can place numerator/denominator.
    return "^{" + numer + "}/_{" + denom + "}";
  }
  if (cmd == "sqrt") {
    const std::string radicand = parse_tex_subset_group(input, pos);
    if (radicand.empty()) {
      return "\xE2\x88\x9A";
    }
    return "\xE2\x88\x9A(" + radicand + ")";
  }
  if (cmd == "text") {
    return parse_tex_subset_group(input, pos);
  }
  if (const auto mark = accent_mark_for_command(cmd); !mark.empty()) {
    return apply_combining_accent(parse_tex_subset_group(input, pos), mark);
  }
  if (cmd == "underline") {
    return parse_tex_subset_group(input, pos);
  }
  if (cmd == "textbf" || cmd == "textit" || cmd == "texttt" || cmd == "mathsf" || cmd == "mathtt") {
    return parse_tex_subset_group(input, pos);
  }
  if (cmd == "mathrm" || cmd == "mathbf" || cmd == "mathit" || cmd == "operatorname" || cmd == "textrm") {
    return parse_tex_subset_group(input, pos);
  }
  if (cmd == "quad" || cmd == "qquad") {
    return " ";
  }
  if (cmd == "left") {
    return encode_stretch_delimiter(true, parse_left_right_delimiter(input, pos));
  }
  if (cmd == "right") {
    return encode_stretch_delimiter(false, parse_left_right_delimiter(input, pos));
  }
  if (const auto mapped = lookup_tex_command(cmd); !mapped.empty()) {
    return std::string(mapped);
  }
  return std::string(cmd);
}

[[nodiscard]] std::string parse_tex_subset_expr(std::string_view input, std::size_t& pos, bool stop_on_brace) {
  std::string out;
  out.reserve(input.size());
  while (pos < input.size()) {
    const char c = input[pos];
    if (stop_on_brace && c == '}') {
      break;
    }
    if (c == '$') {
      ++pos;
      continue;
    }
    if (c == '\\') {
      out += parse_tex_subset_command(input, pos);
      continue;
    }
    if (c == '{') {
      ++pos;
      out += parse_tex_subset_expr(input, pos, true);
      if (pos < input.size() && input[pos] == '}') {
        ++pos;
      }
      continue;
    }
    if (c == '_' || c == '^') {
      const char marker = c;
      ++pos;
      std::string token = parse_tex_subset_group(input, pos);
      if (token.empty()) {
        continue;
      }
      token = collapse_inline_nested_scripts(token);
      out.push_back(marker);
      if (token.size() == 1) {
        out += token;
      } else {
        out.push_back('{');
        out += token;
        out.push_back('}');
      }
      continue;
    }
    const auto len = std::min<std::size_t>(utf8_char_length_local(static_cast<unsigned char>(c)), input.size() - pos);
    out.append(input.substr(pos, len));
    pos += len;
  }
  return out;
}

[[nodiscard]] std::string normalize_tex_subset(std::string_view text) {
  if (!looks_like_tex_markup(text)) {
    return std::string(text);
  }
  std::size_t pos = 0;
  auto parsed = parse_tex_subset_expr(text, pos, false);
  apply_core_tex_symbol_replacements(parsed);
  return parsed;
}

} // namespace

std::string normalize_display_text(std::string_view text) {
  if (text.empty()) {
    return {};
  }

  std::string fallback(text);
  apply_core_tex_symbol_replacements(fallback);
  if (!looks_like_tex_markup(text)) {
    return fallback;
  }

  const std::string parsed = normalize_tex_subset(text);
  if (!parsed.empty() && tex_marker_count(parsed) <= tex_marker_count(fallback)) {
    return parsed;
  }
  return fallback;
}

std::string escape_pdf_text(std::string_view text) {
  const auto append_escaped_ascii = [](std::string& dst, std::string_view ascii) {
    for (const char c : ascii) {
      if (c == '\\' || c == '(' || c == ')') {
        dst.push_back('\\');
      }
      dst.push_back(c);
    }
  };

  const std::string normalized = normalize_display_text(text);
  std::string out;
  out.reserve(normalized.size() + 24);
  for (std::size_t i = 0; i < normalized.size();) {
    bool matched_fallback = false;
    for (const auto& fallback : kUtf8AsciiMathFallbacks) {
      if (i + fallback.utf8.size() <= normalized.size() &&
          normalized.compare(i, fallback.utf8.size(), fallback.utf8) == 0) {
        append_escaped_ascii(out, fallback.ascii);
        i += fallback.utf8.size();
        matched_fallback = true;
        break;
      }
    }
    if (matched_fallback) {
      continue;
    }

    const char c = normalized[i++];
    if (static_cast<unsigned char>(c) >= 0x80) {
      out.push_back('?');
      continue;
    }
    append_escaped_ascii(out, std::string_view(&c, 1));
  }
  return out;
}

} // namespace pluss::pl::plotism::spatex
