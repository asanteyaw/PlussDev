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

[[nodiscard]] std::string_view lookup_tex_command(std::string_view cmd) noexcept {
  constexpr auto kTexCommandMap = std::to_array<std::pair<std::string_view, std::string_view>>({
      {"alpha", "\xCE\xB1"},   {"beta", "\xCE\xB2"},    {"gamma", "\xCE\xB3"},   {"delta", "\xCE\xB4"},
      {"epsilon", "\xCE\xB5"}, {"zeta", "\xCE\xB6"},    {"eta", "\xCE\xB7"},     {"theta", "\xCE\xB8"},
      {"kappa", "\xCE\xBA"},   {"lambda", "\xCE\xBB"},  {"mu", "\xCE\xBC"},      {"nu", "\xCE\xBD"},
      {"xi", "\xCE\xBE"},      {"pi", "\xCF\x80"},      {"rho", "\xCF\x81"},     {"sigma", "\xCF\x83"},
      {"tau", "\xCF\x84"},     {"phi", "\xCF\x86"},     {"chi", "\xCF\x87"},     {"psi", "\xCF\x88"},
      {"omega", "\xCF\x89"},   {"Gamma", "\xCE\x93"},   {"Delta", "\xCE\x94"},   {"Lambda", "\xCE\x9B"},
      {"Sigma", "\xCE\xA3"},   {"Phi", "\xCE\xA6"},     {"Psi", "\xCE\xA8"},     {"Omega", "\xCE\xA9"},
      {"leq", "\xE2\x89\xA4"}, {"le", "\xE2\x89\xA4"},  {"geq", "\xE2\x89\xA5"}, {"ge", "\xE2\x89\xA5"},
      {"neq", "\xE2\x89\xA0"}, {"times", "\xC3\x97"},   {"cdot", "\xE2\x8B\x85"}, {"pm", "\xC2\xB1"},
      {"infty", "\xE2\x88\x9E"}, {"partial", "\xE2\x88\x82"},
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
      if (cmd == "left" || cmd == "right") {
        return {};
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
  if (cmd == "mathrm" || cmd == "mathbf" || cmd == "mathit" || cmd == "operatorname" || cmd == "textrm") {
    return parse_tex_subset_group(input, pos);
  }
  if (cmd == "quad" || cmd == "qquad") {
    return " ";
  }
  if (cmd == "left" || cmd == "right") {
    return {};
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
