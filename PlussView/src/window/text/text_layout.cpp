#include "pluss/view/window/text_layout.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>

namespace pluss::view::window {

namespace {

struct LayoutBuilder {
  TextLayout layout{};
  int cursor_x{0};
};

[[nodiscard]] constexpr int clamp_size(const int pixel_size) noexcept {
  return std::max(6, pixel_size);
}

[[nodiscard]] int glyph_cell(const int pixel_size) noexcept {
  const int size = clamp_size(pixel_size);
  return std::max(1, (size + 6) / 7);
}

[[nodiscard]] int glyph_advance(const int pixel_size, const char ch) noexcept {
  const int cell = glyph_cell(pixel_size);
  if (ch == ' ') {
    return std::max(4, cell * 4);
  }
  if (ch == 'i' || ch == 'l' || ch == 'I' || ch == '|' || ch == '!' || ch == '\'' || ch == ':' || ch == ';' ||
      ch == ',') {
    return std::max(4, cell * 4);
  }
  if (ch == 'm' || ch == 'w' || ch == 'M' || ch == 'W' || ch == '@') {
    return std::max(8, cell * 8);
  }
  if (ch >= '0' && ch <= '9') {
    return std::max(6, cell * 6);
  }
  if (std::isupper(static_cast<unsigned char>(ch)) != 0) {
    return std::max(6, cell * 6);
  }
  return std::max(6, cell * 6);
}

[[nodiscard]] std::string command_replacement(const std::string_view name) {
  using Pair = std::pair<std::string_view, std::string_view>;
  static constexpr std::array<Pair, 20> map = {{
      {"alpha", "a"},
      {"beta", "b"},
      {"gamma", "g"},
      {"delta", "d"},
      {"epsilon", "e"},
      {"lambda", "l"},
      {"mu", "m"},
      {"sigma", "s"},
      {"omega", "w"},
      {"theta", "t"},
      {"pi", "p"},
      {"rho", "r"},
      {"phi", "f"},
      {"leq", "<="},
      {"geq", ">="},
      {"neq", "!="},
      {"times", "x"},
      {"cdot", "."},
      {"pm", "+/-"},
      {"to", "->"},
  }};

  for (const auto& [from, to] : map) {
    if (from == name) {
      return std::string(to);
    }
  }
  return {};
}

void append_text(std::string_view text, const int pixel_size, const int baseline_shift, LayoutBuilder& builder) {
  const int size = clamp_size(pixel_size);
  for (const char ch : text) {
    builder.layout.glyphs.push_back(TextGlyph{ch, builder.cursor_x, baseline_shift, size});
    builder.layout.normalized_text.push_back(ch);
    builder.cursor_x += glyph_advance(size, ch);
  }
}

[[nodiscard]] std::string consume_group(std::string_view source, std::size_t& pos) {
  if (pos >= source.size()) {
    return {};
  }

  if (source[pos] != '{') {
    const std::string out(1, source[pos]);
    ++pos;
    return out;
  }

  ++pos;
  std::string result;
  int depth = 1;
  while (pos < source.size() && depth > 0) {
    const char ch = source[pos++];
    if (ch == '{') {
      ++depth;
      result.push_back(ch);
      continue;
    }
    if (ch == '}') {
      --depth;
      if (depth == 0) {
        break;
      }
      result.push_back(ch);
      continue;
    }
    result.push_back(ch);
  }

  return result;
}

void merge_layout(const TextLayout& child, const int offset_x, const int offset_y, LayoutBuilder& builder) {
  for (const auto& glyph : child.glyphs) {
    builder.layout.glyphs.push_back(
        TextGlyph{glyph.ch, offset_x + glyph.x, offset_y + glyph.y, glyph.pixel_size});
  }
  for (const auto& line : child.lines) {
    builder.layout.lines.push_back(TextLineDecoration{offset_x + line.x0,
                                                      offset_y + line.y0,
                                                      offset_x + line.x1,
                                                      offset_y + line.y1,
                                                      line.thickness});
  }
  if (!child.normalized_text.empty()) {
    builder.layout.normalized_text += child.normalized_text;
  }
  builder.layout.warnings.insert(builder.layout.warnings.end(), child.warnings.begin(), child.warnings.end());
}

void parse_fragment(std::string_view source,
                    std::size_t& pos,
                    const int pixel_size,
                    const int baseline_shift,
                    const bool stop_at_brace,
                    LayoutBuilder& builder);

void parse_command(std::string_view source,
                   std::size_t& pos,
                   const int pixel_size,
                   const int baseline_shift,
                   LayoutBuilder& builder) {
  if (pos >= source.size()) {
    return;
  }

  if (!std::isalpha(static_cast<unsigned char>(source[pos]))) {
    append_text(std::string_view(&source[pos], 1), pixel_size, baseline_shift, builder);
    ++pos;
    return;
  }

  const std::size_t begin = pos;
  while (pos < source.size() && std::isalpha(static_cast<unsigned char>(source[pos]))) {
    ++pos;
  }
  const std::string_view command = source.substr(begin, pos - begin);

  if (command == "frac") {
    std::string numerator = consume_group(source, pos);
    std::string denominator = consume_group(source, pos);

    const int child_size = std::max(6, pixel_size * 72 / 100);
    TextLayout top = layout_math_text(numerator, child_size);
    TextLayout bottom = layout_math_text(denominator, child_size);

    const int pad = std::max(2, pixel_size / 5);
    const int box_width = std::max(top.width, bottom.width) + pad * 2;
    const int start_x = builder.cursor_x;

    merge_layout(top, start_x + (box_width - top.width) / 2, baseline_shift - (pixel_size + pixel_size / 6), builder);
    merge_layout(
        bottom, start_x + (box_width - bottom.width) / 2, baseline_shift + pixel_size / 2, builder);

    const int bar_y = baseline_shift;
    builder.layout.lines.push_back(TextLineDecoration{start_x, bar_y, start_x + box_width, bar_y, std::max(1, pixel_size / 10)});
    builder.layout.normalized_text += "(" + top.normalized_text + "/" + bottom.normalized_text + ")";
    builder.cursor_x += box_width + std::max(1, pixel_size / 5);
    return;
  }

  if (command == "left" || command == "right") {
    return;
  }

  std::string replacement = command_replacement(command);
  if (replacement.empty()) {
    replacement.assign(command.begin(), command.end());
    builder.layout.warnings.push_back("unsupported_math_command:" + std::string(command));
  }
  append_text(replacement, pixel_size, baseline_shift, builder);
}

void parse_fragment(std::string_view source,
                    std::size_t& pos,
                    const int pixel_size,
                    const int baseline_shift,
                    const bool stop_at_brace,
                    LayoutBuilder& builder) {
  while (pos < source.size()) {
    const char ch = source[pos];

    if (stop_at_brace && ch == '}') {
      ++pos;
      return;
    }

    if (ch == '$') {
      ++pos;
      continue;
    }

    if (ch == '\\') {
      ++pos;
      parse_command(source, pos, pixel_size, baseline_shift, builder);
      continue;
    }

    if (ch == '^' || ch == '_') {
      const bool superscript = ch == '^';
      ++pos;
      std::string group = consume_group(source, pos);
      TextLayout child = layout_math_text(group, std::max(6, pixel_size * 70 / 100));
      const int start_x = builder.cursor_x;
      const int dy = superscript ? -(pixel_size * 3 / 4) : (pixel_size * 2 / 3);
      merge_layout(child, start_x, baseline_shift + dy, builder);
      builder.cursor_x += child.width + std::max(1, pixel_size / 8);
      continue;
    }

    if (ch == '{') {
      ++pos;
      parse_fragment(source, pos, pixel_size, baseline_shift, true, builder);
      continue;
    }

    append_text(std::string_view(&source[pos], 1), pixel_size, baseline_shift, builder);
    ++pos;
  }
}

void compute_metrics(TextLayout& layout) {
  int min_top = 0;
  int max_bottom = 0;

  for (const auto& glyph : layout.glyphs) {
    const int top = glyph.y - glyph.pixel_size;
    const int bottom = glyph.y + glyph.pixel_size / 4;
    min_top = std::min(min_top, top);
    max_bottom = std::max(max_bottom, bottom);
    layout.width = std::max(layout.width, glyph.x + glyph_advance(glyph.pixel_size, glyph.ch));
  }

  for (const auto& line : layout.lines) {
    min_top = std::min(min_top, std::min(line.y0, line.y1) - line.thickness);
    max_bottom = std::max(max_bottom, std::max(line.y0, line.y1) + line.thickness);
    layout.width = std::max(layout.width, std::max(line.x0, line.x1));
  }

  layout.ascent = -min_top;
  layout.descent = std::max(0, max_bottom);
}

} // namespace

TextLayout layout_math_text(std::string_view text, int pixel_size) {
  LayoutBuilder builder{};
  std::size_t pos = 0;
  parse_fragment(text, pos, clamp_size(pixel_size), 0, false, builder);
  compute_metrics(builder.layout);
  return builder.layout;
}

} // namespace pluss::view::window
