#include "pluss/tils/view/window/html_layout.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pluss/tils/view/html_window.h"
#include "pluss/tils/view/window/text_layout.h"

namespace pluss::view::window {

namespace {

struct ResolvedStyle {
  int font_size{12};
  int line_height{0};
  int font_weight{400};
  int margin{0};
  int padding{0};
  int gap{0};
  int border{0};
  int explicit_width{0};
  int explicit_height{0};
  int min_width{0};
  int min_height{0};
  double explicit_width_ratio{0.0};
  double explicit_height_ratio{0.0};
  bool explicit_width_set{false};
  bool explicit_height_set{false};
  double min_width_ratio{0.0};
  double min_height_ratio{0.0};
  int border_radius{0};
  int grid_min_column_width{0};
  std::string display{"block"};
  std::string flex_wrap{"nowrap"};
  std::string justify_content{"flex-start"};
  std::string text_align{"left"};
  std::string white_space{"normal"};
  std::string text_transform{"none"};
  Color text_color{Color::rgba(32, 36, 40, 255)};
  Color border_color{Color::rgba(208, 215, 222, 255)};
  Color background_color{Color::rgba(255, 255, 255, 255)};
  Color shadow_color{Color::rgba(22, 30, 42, 38)};
  int shadow_offset_x{0};
  int shadow_offset_y{0};
  int shadow_blur{0};
  int sticky_left_px{0};
  int sticky_top_px{0};
  bool sticky_left_set{false};
  bool sticky_top_set{false};
  bool margin_left_auto{false};
  std::string font_family{};
  std::string position{"static"};
  bool has_background{false};
  bool has_shadow{false};
  bool has_linear_gradient{false};
  bool gradient_horizontal{false};
  Color gradient_start{Color::rgba(255, 255, 255, 255)};
  Color gradient_end{Color::rgba(245, 246, 248, 255)};
};

struct TableCellData {
  const HtmlNode* node{nullptr};
  std::string text{};
  bool is_header{false};
};

struct TableRowData {
  const HtmlNode* node{nullptr};
  std::vector<const HtmlNode*> lineage{};
  std::vector<TableCellData> cells{};
};

struct TableColumnPlan {
  std::vector<std::size_t> source_indices{};
  std::vector<int> widths{};
  bool truncated{false};
  int ellipsis_width{0};
  int hidden_count{0};
};

struct SimpleSelector {
  std::string tag{};
  std::string id{};
  std::vector<std::string> classes{};
  bool universal{false};
};

[[nodiscard]] int clamp_nonneg(const int value) {
  return std::max(0, value);
}

[[nodiscard]] RectI intersect_rect(const RectI lhs, const RectI rhs) {
  const int x0 = std::max(lhs.x, rhs.x);
  const int y0 = std::max(lhs.y, rhs.y);
  const int x1 = std::min(lhs.x + lhs.width, rhs.x + rhs.width);
  const int y1 = std::min(lhs.y + lhs.height, rhs.y + rhs.height);
  return RectI{x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

void reset_render_stats(HtmlRenderStats& stats) {
  stats.document_viewport_rect = RectI{};
  stats.table_viewport_rect = RectI{};
  stats.document_max_scroll_x = 0;
  stats.document_max_scroll_y = 0;
  stats.table_max_scroll_x = 0;
  stats.table_max_scroll_y = 0;
  stats.table_body_row_height_px = 0;
  stats.document_horizontal_track_rect = RectI{};
  stats.document_horizontal_thumb_rect = RectI{};
  stats.document_vertical_track_rect = RectI{};
  stats.document_vertical_thumb_rect = RectI{};
  stats.table_scroll_regions.clear();
  stats.has_table = false;
}

[[nodiscard]] int clamp_font(const int value) {
  return std::clamp(value, 8, 48);
}

[[nodiscard]] int default_line_height(const int font_size) {
  return font_size + std::max(4, font_size / 3);
}

[[nodiscard]] int resolve_line_height_value(const int line_height, const int font_size) {
  if (line_height <= 0) {
    return default_line_height(font_size);
  }
  return std::max(font_size, line_height);
}

[[nodiscard]] std::string to_lower(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const char ch : value) {
    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  return out;
}

[[nodiscard]] std::string trim_copy(std::string_view value) {
  std::size_t begin = 0;
  while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
    ++begin;
  }
  std::size_t end = value.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
    --end;
  }
  return std::string(value.substr(begin, end - begin));
}

[[nodiscard]] int parse_css_int(std::string_view value) {
  std::string token;
  token.reserve(value.size());
  for (const char ch : value) {
    if ((ch >= '0' && ch <= '9') || ch == '-') {
      token.push_back(ch);
      continue;
    }
    if (!token.empty()) {
      break;
    }
  }
  if (token.empty() || token == "-") {
    return 0;
  }
  int parsed = 0;
  const auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), parsed);
  if (ec != std::errc{} || ptr != token.data() + token.size()) {
    return 0;
  }
  return parsed;
}

[[nodiscard]] std::optional<double> parse_css_number(std::string_view value) {
  const std::string token = trim_copy(value);
  if (token.empty()) {
    return std::nullopt;
  }

  char* end = nullptr;
  const double parsed = std::strtod(token.c_str(), &end);
  if (end == token.c_str()) {
    return std::nullopt;
  }
  return parsed;
}

[[nodiscard]] int parse_css_length_px(std::string_view value,
                                      const int base_font_size_px,
                                      const int parent_reference_px = 0) {
  const std::string token = to_lower(trim_copy(value));
  if (token.empty()) {
    return 0;
  }
  if (token == "auto") {
    return 0;
  }

  std::size_t i = 0;
  while (i < token.size() && (std::isdigit(static_cast<unsigned char>(token[i])) != 0 || token[i] == '.' ||
                              token[i] == '+' || token[i] == '-')) {
    ++i;
  }
  if (i == 0) {
    return 0;
  }

  const auto number = parse_css_number(std::string_view(token.data(), i));
  if (!number.has_value()) {
    return 0;
  }

  const std::string unit = trim_copy(std::string_view(token.data() + i, token.size() - i));
  double px = *number;
  if (unit.empty() || unit == "px") {
    // no-op
  } else if (unit == "rem") {
    px *= 16.0;
  } else if (unit == "em") {
    px *= static_cast<double>(base_font_size_px);
  } else if (unit == "%") {
    if (parent_reference_px <= 0) {
      return 0;
    }
    px = static_cast<double>(parent_reference_px) * (px / 100.0);
  } else if (unit == "vh" || unit == "vw") {
    // Keep this conservative in software-mode layout.
    return 0;
  } else {
    return 0;
  }

  return static_cast<int>(std::lround(px));
}

[[nodiscard]] std::optional<double> parse_css_percent_ratio(std::string_view value) {
  const std::string token = to_lower(trim_copy(value));
  if (token.empty() || token.back() != '%') {
    return std::nullopt;
  }
  const auto number = parse_css_number(std::string_view(token.data(), token.size() - 1));
  if (!number.has_value()) {
    return std::nullopt;
  }
  return std::clamp(*number / 100.0, 0.0, 4.0);
}

[[nodiscard]] int parse_css_line_height_px(std::string_view value, const int font_size_px) {
  const std::string token = to_lower(trim_copy(value));
  if (token.empty() || token == "normal") {
    return 0;
  }
  if (token.find("px") != std::string::npos || token.find("rem") != std::string::npos ||
      token.find("em") != std::string::npos) {
    return std::max(0, parse_css_length_px(token, font_size_px));
  }

  const auto numeric = parse_css_number(token);
  if (!numeric.has_value() || *numeric <= 0.0) {
    return 0;
  }
  if (token.find('%') != std::string::npos) {
    return static_cast<int>(std::lround(static_cast<double>(font_size_px) * (*numeric / 100.0)));
  }
  // Unitless line-height multiplier.
  if (*numeric < 6.0) {
    return static_cast<int>(std::lround(static_cast<double>(font_size_px) * (*numeric)));
  }
  return static_cast<int>(std::lround(*numeric));
}

[[nodiscard]] int hex_value(const char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
  if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
  return -1;
}

[[nodiscard]] std::optional<Color> parse_hex_color(std::string_view value) {
  if (value.empty() || value.front() != '#') {
    return std::nullopt;
  }

  const auto hex = value.substr(1);
  if (hex.size() == 3) {
    const int r = hex_value(hex[0]);
    const int g = hex_value(hex[1]);
    const int b = hex_value(hex[2]);
    if (r < 0 || g < 0 || b < 0) {
      return std::nullopt;
    }
    return Color::rgba(static_cast<std::uint8_t>(r * 17),
                       static_cast<std::uint8_t>(g * 17),
                       static_cast<std::uint8_t>(b * 17),
                       255);
  }

  if (hex.size() == 6) {
    const int r_hi = hex_value(hex[0]);
    const int r_lo = hex_value(hex[1]);
    const int g_hi = hex_value(hex[2]);
    const int g_lo = hex_value(hex[3]);
    const int b_hi = hex_value(hex[4]);
    const int b_lo = hex_value(hex[5]);
    if (r_hi < 0 || r_lo < 0 || g_hi < 0 || g_lo < 0 || b_hi < 0 || b_lo < 0) {
      return std::nullopt;
    }
    return Color::rgba(static_cast<std::uint8_t>((r_hi * 16) + r_lo),
                       static_cast<std::uint8_t>((g_hi * 16) + g_lo),
                       static_cast<std::uint8_t>((b_hi * 16) + b_lo),
                       255);
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<Color> named_color(std::string_view value) {
  static constexpr std::array<std::pair<std::string_view, Color>, 12> kMap{{
      {"white", Color::rgba(255, 255, 255, 255)},
      {"black", Color::rgba(0, 0, 0, 255)},
      {"gray", Color::rgba(128, 128, 128, 255)},
      {"grey", Color::rgba(128, 128, 128, 255)},
      {"red", Color::rgba(220, 72, 68, 255)},
      {"green", Color::rgba(39, 161, 98, 255)},
      {"blue", Color::rgba(35, 111, 218, 255)},
      {"lightgray", Color::rgba(230, 233, 236, 255)},
      {"lightgrey", Color::rgba(230, 233, 236, 255)},
      {"silver", Color::rgba(208, 215, 222, 255)},
      {"navy", Color::rgba(12, 42, 84, 255)},
      {"transparent", Color::rgba(0, 0, 0, 0)},
  }};
  for (const auto& [name, color] : kMap) {
    if (name == value) return color;
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::string> split_comma_args(std::string_view text) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  int depth = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char ch = text[i];
    if (ch == '(') {
      ++depth;
    } else if (ch == ')' && depth > 0) {
      --depth;
    } else if (ch == ',' && depth == 0) {
      parts.push_back(trim_copy(text.substr(start, i - start)));
      start = i + 1;
    }
  }
  parts.push_back(trim_copy(text.substr(start)));
  return parts;
}

[[nodiscard]] std::vector<std::string> split_space_tokens(std::string_view text) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  int depth = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char ch = text[i];
    if (ch == '(') {
      ++depth;
      continue;
    }
    if (ch == ')' && depth > 0) {
      --depth;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch)) != 0 && depth == 0) {
      if (i > start) {
        const std::string token = trim_copy(text.substr(start, i - start));
        if (!token.empty()) {
          parts.push_back(token);
        }
      }
      start = i + 1;
    }
  }
  if (start < text.size()) {
    const std::string token = trim_copy(text.substr(start));
    if (!token.empty()) {
      parts.push_back(token);
    }
  }
  return parts;
}

[[nodiscard]] std::uint8_t clamp_channel(const int value) {
  return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

[[nodiscard]] std::optional<int> parse_rgb_channel(std::string_view token) {
  const std::string t = trim_copy(token);
  if (t.empty()) {
    return std::nullopt;
  }
  if (!t.empty() && t.back() == '%') {
    const auto n = parse_css_number(std::string_view(t.data(), t.size() - 1));
    if (!n.has_value()) return std::nullopt;
    return static_cast<int>(std::lround(std::clamp(*n, 0.0, 100.0) * 2.55));
  }
  const auto n = parse_css_number(t);
  if (!n.has_value()) return std::nullopt;
  return static_cast<int>(std::lround(*n));
}

[[nodiscard]] std::optional<std::uint8_t> parse_alpha_channel(std::string_view token) {
  const std::string t = trim_copy(token);
  if (t.empty()) {
    return std::nullopt;
  }
  if (!t.empty() && t.back() == '%') {
    const auto n = parse_css_number(std::string_view(t.data(), t.size() - 1));
    if (!n.has_value()) return std::nullopt;
    return static_cast<std::uint8_t>(std::lround(std::clamp(*n, 0.0, 100.0) * 2.55));
  }
  const auto n = parse_css_number(t);
  if (!n.has_value()) return std::nullopt;
  const double v = *n;
  if (v <= 1.0) {
    return static_cast<std::uint8_t>(std::lround(std::clamp(v, 0.0, 1.0) * 255.0));
  }
  return static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(v)), 0, 255));
}

[[nodiscard]] std::optional<Color> parse_rgb_function(std::string_view lower) {
  const bool is_rgba = lower.rfind("rgba(", 0) == 0;
  const bool is_rgb = lower.rfind("rgb(", 0) == 0;
  if (!is_rgba && !is_rgb) {
    return std::nullopt;
  }
  const std::size_t lparen = lower.find('(');
  const std::size_t rparen = lower.rfind(')');
  if (lparen == std::string::npos || rparen == std::string::npos || rparen <= lparen + 1) {
    return std::nullopt;
  }

  const auto args = split_comma_args(std::string_view(lower.data() + lparen + 1, rparen - lparen - 1));
  if ((!is_rgba && args.size() < 3) || (is_rgba && args.size() < 4)) {
    return std::nullopt;
  }
  const auto r = parse_rgb_channel(args[0]);
  const auto g = parse_rgb_channel(args[1]);
  const auto b = parse_rgb_channel(args[2]);
  if (!r.has_value() || !g.has_value() || !b.has_value()) {
    return std::nullopt;
  }
  std::uint8_t a = 255;
  if (is_rgba) {
    const auto alpha = parse_alpha_channel(args[3]);
    if (!alpha.has_value()) {
      return std::nullopt;
    }
    a = *alpha;
  }
  return Color::rgba(clamp_channel(*r), clamp_channel(*g), clamp_channel(*b), a);
}

[[nodiscard]] std::optional<Color> parse_first_color_token(std::string_view value) {
  const std::string lower = to_lower(value);

  const std::size_t hash_pos = lower.find('#');
  if (hash_pos != std::string::npos) {
    std::size_t end = hash_pos + 1;
    while (end < lower.size() && std::isxdigit(static_cast<unsigned char>(lower[end])) != 0) {
      ++end;
    }
    if (const auto c = parse_hex_color(std::string_view(lower.data() + hash_pos, end - hash_pos)); c.has_value()) {
      return c;
    }
  }

  const std::size_t rgb_pos = lower.find("rgb(");
  const std::size_t rgba_pos = lower.find("rgba(");
  const std::size_t fn_pos = (rgb_pos == std::string::npos)
                                 ? rgba_pos
                                 : ((rgba_pos == std::string::npos) ? rgb_pos : std::min(rgb_pos, rgba_pos));
  if (fn_pos != std::string::npos) {
    const std::size_t close = lower.find(')', fn_pos);
    if (close != std::string::npos) {
      if (const auto c = parse_rgb_function(std::string_view(lower.data() + fn_pos, close - fn_pos + 1)); c.has_value()) {
        return c;
      }
    }
  }

  for (const auto token : split_comma_args(lower)) {
    const std::string candidate = trim_copy(token);
    if (candidate.empty()) {
      continue;
    }
    if (const auto c = named_color(candidate); c.has_value()) {
      return c;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<Color> parse_color(std::string_view value) {
  const std::string lower = to_lower(trim_copy(value));
  if (lower.empty()) {
    return std::nullopt;
  }
  if (const auto rgb = parse_rgb_function(lower); rgb.has_value()) {
    return rgb;
  }
  if (const auto hex = parse_hex_color(lower); hex.has_value()) {
    return hex;
  }
  if (const auto named = named_color(lower); named.has_value()) {
    return named;
  }
  return parse_first_color_token(lower);
}

struct BoxShadowSpec {
  int offset_x{0};
  int offset_y{0};
  int blur{0};
  Color color{Color::rgba(22, 30, 42, 38)};
};

struct LinearGradientSpec {
  Color start{Color::rgba(255, 255, 255, 255)};
  Color end{Color::rgba(245, 246, 248, 255)};
  bool horizontal{false};
};

[[nodiscard]] std::optional<BoxShadowSpec> parse_box_shadow(std::string_view value, const int font_size) {
  std::string lower = to_lower(trim_copy(value));
  if (lower.empty() || lower == "none") {
    return std::nullopt;
  }

  // Keep the first shadow in a comma-separated chain.
  int depth = 0;
  for (std::size_t i = 0; i < lower.size(); ++i) {
    if (lower[i] == '(') {
      ++depth;
      continue;
    }
    if (lower[i] == ')' && depth > 0) {
      --depth;
      continue;
    }
    if (lower[i] == ',' && depth == 0) {
      lower.resize(i);
      break;
    }
  }

  if (lower.find("inset") != std::string::npos) {
    return std::nullopt;
  }

  std::vector<int> lengths;
  const auto tokens = split_space_tokens(lower);
  for (const auto& token : tokens) {
    if (token.empty()) {
      continue;
    }
    const char first = token.front();
    const bool looks_numeric = std::isdigit(static_cast<unsigned char>(first)) != 0 || first == '-' || first == '+' ||
                               first == '.';
    if (!looks_numeric) {
      continue;
    }
    lengths.push_back(parse_css_length_px(token, font_size));
    if (lengths.size() >= 4) {
      break;
    }
  }
  if (lengths.size() < 2) {
    return std::nullopt;
  }

  const auto color = parse_color(lower).value_or(Color::rgba(22, 30, 42, 38));
  return BoxShadowSpec{
      .offset_x = lengths[0],
      .offset_y = lengths[1],
      .blur = (lengths.size() >= 3) ? std::max(0, lengths[2]) : 0,
      .color = color,
  };
}

[[nodiscard]] std::optional<LinearGradientSpec> parse_linear_gradient(std::string_view value) {
  const std::string lower = to_lower(trim_copy(value));
  const std::size_t fn = lower.find("linear-gradient(");
  if (fn == std::string::npos) {
    return std::nullopt;
  }
  const std::size_t open = lower.find('(', fn);
  const std::size_t close = lower.rfind(')');
  if (open == std::string::npos || close == std::string::npos || close <= open + 1) {
    return std::nullopt;
  }
  const auto args = split_comma_args(std::string_view(lower.data() + open + 1, close - open - 1));
  if (args.size() < 2) {
    return std::nullopt;
  }

  std::size_t color_start = 0;
  bool horizontal = false;
  if (args.size() >= 3) {
    const std::string dir = trim_copy(args[0]);
    if (dir.find("to right") != std::string::npos || dir.find("90deg") != std::string::npos ||
        dir.find("270deg") != std::string::npos) {
      horizontal = true;
      color_start = 1;
    } else if (dir.find("to left") != std::string::npos) {
      horizontal = true;
      color_start = 1;
    } else if (dir.find("to bottom") != std::string::npos || dir.find("180deg") != std::string::npos ||
               dir.find("0deg") != std::string::npos) {
      color_start = 1;
    } else if (parse_color(dir).has_value()) {
      color_start = 0;
    } else {
      color_start = 1;
    }
  }

  if (color_start >= args.size()) {
    return std::nullopt;
  }
  const auto start = parse_color(args[color_start]);
  const auto end = parse_color(args.back());
  if (!start.has_value() || !end.has_value()) {
    return std::nullopt;
  }

  return LinearGradientSpec{
      .start = *start,
      .end = *end,
      .horizontal = horizontal,
  };
}

[[nodiscard]] std::string apply_text_transform(std::string text, std::string_view transform) {
  const std::string mode = to_lower(trim_copy(transform));
  if (mode.empty() || mode == "none") {
    return text;
  }
  if (mode == "uppercase") {
    std::transform(text.begin(),
                   text.end(),
                   text.begin(),
                   [](const unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    return text;
  }
  if (mode == "lowercase") {
    std::transform(text.begin(),
                   text.end(),
                   text.begin(),
                   [](const unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return text;
  }
  if (mode == "capitalize") {
    bool new_word = true;
    for (char& ch : text) {
      const bool alpha = std::isalpha(static_cast<unsigned char>(ch)) != 0;
      if (new_word && alpha) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
      }
      new_word = std::isspace(static_cast<unsigned char>(ch)) != 0;
    }
  }
  return text;
}

[[nodiscard]] std::string collect_node_text(const HtmlNode& node) {
  if (node.kind == HtmlNodeKind::Text) {
    return node.text;
  }

  std::string out;
  for (const auto& child : node.children) {
    const std::string fragment = collect_node_text(child);
    if (fragment.empty()) continue;
    if (!out.empty()) out.push_back(' ');
    out += fragment;
  }
  return out;
}

[[nodiscard]] std::vector<std::string> split_class_tokens(std::string_view classes) {
  std::vector<std::string> out;
  std::size_t i = 0;
  while (i < classes.size()) {
    while (i < classes.size() && std::isspace(static_cast<unsigned char>(classes[i])) != 0) {
      ++i;
    }
    if (i >= classes.size()) {
      break;
    }
    std::size_t j = i;
    while (j < classes.size() && std::isspace(static_cast<unsigned char>(classes[j])) == 0) {
      ++j;
    }
    out.push_back(to_lower(classes.substr(i, j - i)));
    i = j;
  }
  return out;
}

[[nodiscard]] bool node_has_class(const HtmlNode* node, std::string_view class_name) {
  if (node == nullptr || class_name.empty()) {
    return false;
  }
  const HtmlAttribute* class_attr = find_attribute(*node, "class");
  if (class_attr == nullptr) {
    return false;
  }
  const auto classes = split_class_tokens(class_attr->value);
  const std::string needle = to_lower(class_name);
  return std::find(classes.begin(), classes.end(), needle) != classes.end();
}

[[nodiscard]] const std::string* attribute_value(const HtmlNode* node, std::string_view name) {
  if (node == nullptr || name.empty()) {
    return nullptr;
  }
  if (const HtmlAttribute* attr = find_attribute(*node, name); attr != nullptr) {
    return &attr->value;
  }
  return nullptr;
}

[[nodiscard]] std::string node_interaction_target(const HtmlNode& node, std::string_view fallback) {
  if (const std::string* id = attribute_value(&node, "id"); id != nullptr && !id->empty()) {
    return *id;
  }
  if (const std::string* name = attribute_value(&node, "name"); name != nullptr && !name->empty()) {
    return *name;
  }
  if (const std::string* cls = attribute_value(&node, "class"); cls != nullptr && !cls->empty()) {
    return *cls;
  }
  if (!fallback.empty()) {
    return std::string(fallback);
  }
  return node.tag;
}

[[nodiscard]] std::string node_instance_target(const HtmlNode& node, std::string_view fallback) {
  if (const std::string* id = attribute_value(&node, "id"); id != nullptr && !id->empty()) {
    return *id;
  }
  if (const std::string* name = attribute_value(&node, "name"); name != nullptr && !name->empty()) {
    return *name;
  }

  std::string target = fallback.empty() ? node.tag : std::string(fallback);
  target += '@';
  target += std::to_string(reinterpret_cast<std::uintptr_t>(&node));
  return target;
}

[[nodiscard]] std::optional<double> parse_numeric_text(std::string_view value) {
  const std::string token = trim_copy(value);
  if (token.empty()) {
    return std::nullopt;
  }
  char* end = nullptr;
  const double parsed = std::strtod(token.c_str(), &end);
  if (end == token.c_str()) {
    return std::nullopt;
  }
  while (end != nullptr && *end != '\0' && std::isspace(static_cast<unsigned char>(*end)) != 0) {
    ++end;
  }
  if (end != nullptr && *end != '\0') {
    return std::nullopt;
  }
  return parsed;
}

[[nodiscard]] std::string truncate_to_width(std::string text, int font_size, int max_width);

[[nodiscard]] std::vector<std::string> wrap_lines(std::string_view text,
                                                   const int font_size,
                                                   const int max_width) {
  std::vector<std::string> lines;
  if (max_width <= 0) {
    return lines;
  }

  std::string normalized;
  normalized.reserve(text.size());
  bool in_space = false;
  for (const char ch : text) {
    if (ch == '\n' || ch == '\r') {
      if (!normalized.empty() && normalized.back() != '\n') {
        normalized.push_back('\n');
      }
      in_space = false;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
      if (!in_space) {
        normalized.push_back(' ');
      }
      in_space = true;
      continue;
    }
    in_space = false;
    normalized.push_back(ch);
  }

  std::size_t start = 0;
  while (start < normalized.size()) {
    std::size_t end = normalized.find('\n', start);
    if (end == std::string::npos) {
      end = normalized.size();
    }

    std::string segment(normalized.substr(start, end - start));
    while (!segment.empty() && segment.front() == ' ') segment.erase(segment.begin());
    while (!segment.empty() && segment.back() == ' ') segment.pop_back();

    if (segment.empty()) {
      lines.push_back({});
      start = end + 1;
      continue;
    }

    std::string current;
    std::size_t word_pos = 0;
    while (word_pos < segment.size()) {
      while (word_pos < segment.size() && segment[word_pos] == ' ') ++word_pos;
      if (word_pos >= segment.size()) break;
      std::size_t word_end = word_pos;
      while (word_end < segment.size() && segment[word_end] != ' ') ++word_end;

      const std::string word(segment.substr(word_pos, word_end - word_pos));
      const std::string candidate = current.empty() ? word : (current + " " + word);
      if (layout_math_text(candidate, font_size).width <= max_width) {
        current = candidate;
      } else if (current.empty()) {
        lines.push_back(truncate_to_width(word, font_size, max_width));
        current.clear();
      } else {
        lines.push_back(current);
        current = word;
      }
      word_pos = word_end;
    }

    if (!current.empty()) {
      lines.push_back(std::move(current));
    }
    start = end + 1;
  }

  if (lines.empty()) {
    lines.push_back({});
  }
  return lines;
}

[[nodiscard]] std::string truncate_to_width(std::string text, const int font_size, const int max_width) {
  if (max_width <= 0) {
    return {};
  }
  if (layout_math_text(text, font_size).width <= max_width) {
    return text;
  }

  const std::string ellipsis = "...";
  while (!text.empty()) {
    text.pop_back();
    const std::string candidate = text + ellipsis;
    if (layout_math_text(candidate, font_size).width <= max_width) {
      return candidate;
    }
  }
  return ellipsis;
}

[[nodiscard]] std::vector<std::string> limit_lines(std::vector<std::string> lines,
                                                    const int max_lines,
                                                    const int font_size,
                                                    const int max_width) {
  if (max_lines <= 0 || static_cast<int>(lines.size()) <= max_lines) {
    return lines;
  }

  lines.resize(static_cast<std::size_t>(max_lines));
  if (!lines.empty()) {
    lines.back() = truncate_to_width(lines.back() + "...", font_size, max_width);
  }
  return lines;
}

[[nodiscard]] std::vector<std::string> layout_text_lines(const std::string& text,
                                                         const ResolvedStyle& style,
                                                         const int max_width) {
  const std::string transformed = apply_text_transform(text, style.text_transform);
  if (style.white_space == "nowrap") {
    return {truncate_to_width(transformed, style.font_size, max_width)};
  }
  return wrap_lines(transformed, style.font_size, max_width);
}

[[nodiscard]] std::string first_css_token(std::string_view value) {
  const std::string trimmed = trim_copy(value);
  std::size_t end = 0;
  while (end < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[end])) == 0) {
    ++end;
  }
  return trimmed.substr(0, end);
}

[[nodiscard]] int parse_font_weight(std::string_view value, const int fallback) {
  const std::string token = to_lower(trim_copy(value));
  if (token == "normal") return 400;
  if (token == "bold") return 700;
  if (token == "bolder") return std::max(700, fallback);
  if (token == "lighter") return std::min(300, fallback);
  const int parsed = parse_css_int(token);
  if (parsed <= 0) return fallback;
  return std::clamp(parsed, 100, 900);
}

[[nodiscard]] std::string resolve_css_variables(std::string_view value,
                                                const std::unordered_map<std::string, std::string>& variables) {
  std::string resolved = trim_copy(value);
  for (int guard = 0; guard < 6; ++guard) {
    const std::size_t var_pos = resolved.find("var(");
    if (var_pos == std::string::npos) {
      break;
    }
    const std::size_t close = resolved.find(')', var_pos + 4);
    if (close == std::string::npos) {
      break;
    }

    const std::string inside = trim_copy(std::string_view(resolved.data() + var_pos + 4, close - var_pos - 4));
    std::string variable_name = inside;
    std::string fallback{};
    const std::size_t comma = inside.find(',');
    if (comma != std::string::npos) {
      variable_name = trim_copy(std::string_view(inside.data(), comma));
      fallback = trim_copy(std::string_view(inside.data() + comma + 1, inside.size() - comma - 1));
    }

    std::string replacement = fallback;
    if (!variable_name.empty()) {
      const std::string lowered = to_lower(variable_name);
      if (const auto it = variables.find(lowered); it != variables.end()) {
        replacement = it->second;
      }
    }
    resolved.replace(var_pos, (close - var_pos) + 1, replacement);
  }
  return resolved;
}

void apply_style_property(ResolvedStyle& style,
                          std::string_view key,
                          std::string_view value,
                          const std::unordered_map<std::string, std::string>& css_variables) {
  const std::string property = to_lower(trim_copy(key));
  const std::string resolved = resolve_css_variables(value, css_variables);
  const std::string val = to_lower(trim_copy(resolved));
  const std::string first_token = first_css_token(val);

  if (property == "font-size") {
    const int parsed = parse_css_length_px(first_token, style.font_size);
    if (parsed > 0) style.font_size = clamp_font(parsed);
  } else if (property == "font-family") {
    style.font_family = trim_copy(resolved);
  } else if (property == "line-height") {
    const int parsed = parse_css_line_height_px(first_token, style.font_size);
    if (parsed > 0) style.line_height = parsed;
  } else if (property == "font-weight") {
    style.font_weight = parse_font_weight(val, style.font_weight);
  } else if (property == "margin") {
    style.margin = clamp_nonneg(parse_css_length_px(first_token, style.font_size));
  } else if (property == "margin-left") {
    style.margin_left_auto = val == "auto";
  } else if (property == "padding") {
    style.padding = clamp_nonneg(parse_css_length_px(first_token, style.font_size));
  } else if (property == "gap" || property == "column-gap" || property == "row-gap") {
    style.gap = clamp_nonneg(parse_css_length_px(first_token, style.font_size));
  } else if (property == "border-width") {
    style.border = clamp_nonneg(parse_css_length_px(first_token, style.font_size));
  } else if (property == "border-radius") {
    style.border_radius = clamp_nonneg(parse_css_length_px(first_token, style.font_size));
  } else if (property == "width") {
    style.explicit_width_set = true;
    if (const auto ratio = parse_css_percent_ratio(first_token); ratio.has_value()) {
      style.explicit_width_ratio = *ratio;
      style.explicit_width = 0;
    } else {
      style.explicit_width = clamp_nonneg(parse_css_length_px(first_token, style.font_size));
      style.explicit_width_ratio = 0.0;
    }
  } else if (property == "height") {
    style.explicit_height_set = true;
    if (const auto ratio = parse_css_percent_ratio(first_token); ratio.has_value()) {
      style.explicit_height_ratio = *ratio;
      style.explicit_height = 0;
    } else {
      style.explicit_height = clamp_nonneg(parse_css_length_px(first_token, style.font_size));
      style.explicit_height_ratio = 0.0;
    }
  } else if (property == "left") {
    style.sticky_left_px = parse_css_length_px(first_token, style.font_size);
    style.sticky_left_set = true;
  } else if (property == "top") {
    style.sticky_top_px = parse_css_length_px(first_token, style.font_size);
    style.sticky_top_set = true;
  } else if (property == "min-width") {
    if (const auto ratio = parse_css_percent_ratio(first_token); ratio.has_value()) {
      style.min_width_ratio = *ratio;
      style.min_width = 0;
    } else {
      style.min_width = clamp_nonneg(parse_css_length_px(first_token, style.font_size));
      style.min_width_ratio = 0.0;
    }
  } else if (property == "min-height") {
    if (const auto ratio = parse_css_percent_ratio(first_token); ratio.has_value()) {
      style.min_height_ratio = *ratio;
      style.min_height = 0;
    } else {
      style.min_height = clamp_nonneg(parse_css_length_px(first_token, style.font_size));
      style.min_height_ratio = 0.0;
    }
  } else if (property == "display") {
    style.display = val;
  } else if (property == "position") {
    style.position = val;
  } else if (property == "flex-wrap") {
    style.flex_wrap = val;
  } else if (property == "justify-content") {
    style.justify_content = val;
  } else if (property == "text-align") {
    style.text_align = val;
  } else if (property == "white-space") {
    style.white_space = val;
  } else if (property == "grid-template-columns") {
    const std::size_t minmax_pos = val.find("minmax(");
    if (minmax_pos != std::string::npos) {
      const std::size_t comma_pos = val.find(',', minmax_pos + 7);
      if (comma_pos != std::string::npos) {
        const std::string width_token =
            trim_copy(std::string_view(val.data() + minmax_pos + 7, comma_pos - (minmax_pos + 7)));
        style.grid_min_column_width = clamp_nonneg(parse_css_length_px(width_token, style.font_size));
      }
    }
  } else if (property == "color") {
    if (const auto c = parse_color(val); c.has_value()) style.text_color = *c;
  } else if (property == "border-color") {
    if (const auto c = parse_color(val); c.has_value()) style.border_color = *c;
  } else if (property == "background" || property == "background-color") {
    if (const auto gradient = parse_linear_gradient(val); gradient.has_value()) {
      style.has_linear_gradient = true;
      style.gradient_start = gradient->start;
      style.gradient_end = gradient->end;
      style.gradient_horizontal = gradient->horizontal;
      style.has_background = true;
      style.background_color = gradient->start;
    } else if (const auto c = parse_color(val); c.has_value()) {
      style.has_linear_gradient = false;
      style.background_color = *c;
      style.has_background = c->a > 0;
    }
  } else if (property == "border") {
    // Minimal shorthand: "1px solid #d0d7de"
    std::size_t p = 0;
    while (p < val.size() && std::isspace(static_cast<unsigned char>(val[p])) != 0) ++p;
    std::size_t q = p;
    while (q < val.size() && std::isspace(static_cast<unsigned char>(val[q])) == 0) ++q;
    style.border = clamp_nonneg(parse_css_length_px(val.substr(p, q - p), style.font_size));
    if (const auto c = parse_color(val); c.has_value()) {
      style.border_color = *c;
    }
  } else if (property == "box-shadow") {
    if (const auto shadow = parse_box_shadow(val, style.font_size); shadow.has_value()) {
      style.has_shadow = true;
      style.shadow_offset_x = shadow->offset_x;
      style.shadow_offset_y = shadow->offset_y;
      style.shadow_blur = shadow->blur;
      style.shadow_color = shadow->color;
    } else {
      style.has_shadow = false;
      style.shadow_offset_x = 0;
      style.shadow_offset_y = 0;
      style.shadow_blur = 0;
    }
  } else if (property == "font") {
    // Best effort for shorthand font: "... 14px/20px ..."
    const std::size_t slash = val.find('/');
    if (slash != std::string::npos) {
      const std::string lhs = trim_copy(std::string_view(val.data(), slash));
      const std::size_t space = lhs.rfind(' ');
      const std::string font_size_token = (space == std::string::npos) ? lhs : lhs.substr(space + 1);
      const std::string line_height_token = trim_copy(std::string_view(val.data() + slash + 1));
      const int font = parse_css_length_px(font_size_token, style.font_size);
      const int lh = parse_css_line_height_px(first_css_token(line_height_token), style.font_size);
      if (font > 0) style.font_size = clamp_font(font);
      if (lh > 0) style.line_height = lh;
    } else {
      const std::size_t space = val.rfind(' ');
      const std::string token = (space == std::string::npos) ? val : val.substr(space + 1);
      const int font = parse_css_length_px(token, style.font_size);
      if (font > 0) style.font_size = clamp_font(font);
    }
    if (val.find("bold") != std::string::npos) {
      style.font_weight = std::max(style.font_weight, 700);
    }
    // Family is usually the final token chunk after size/line-height.
    std::size_t family_pos = std::string::npos;
    if (slash != std::string::npos) {
      family_pos = val.find(' ', slash + 1);
    } else {
      const std::size_t px_pos = val.find("px");
      if (px_pos != std::string::npos) {
        family_pos = val.find(' ', px_pos + 2);
      }
    }
    if (family_pos != std::string::npos && family_pos + 1 < resolved.size()) {
      style.font_family = trim_copy(std::string_view(resolved.data() + family_pos + 1,
                                                     resolved.size() - family_pos - 1));
    }
  } else if (property == "text-transform") {
    style.text_transform = val;
  } else if (property == "right" || property == "bottom" || property == "overflow" || property == "overflow-x" ||
             property == "overflow-y" || property == "cursor" || property == "z-index" ||
             property == "backdrop-filter" || property == "letter-spacing" || property == "user-select" ||
             property == "font-variant-numeric" || property == "align-items") {
    // Parsed/ignored for now.
  } else if (property.size() > 2 && property[0] == '-' && property[1] == '-') {
    // CSS custom property; handled through document-level variable store.
  }
}

void apply_inline_style_attribute(ResolvedStyle& style,
                                  const HtmlNode& node,
                                  const std::unordered_map<std::string, std::string>& css_variables) {
  const HtmlAttribute* attr = find_attribute(node, "style");
  if (attr == nullptr || attr->value.empty()) {
    return;
  }

  std::size_t start = 0;
  const std::string& raw = attr->value;
  while (start < raw.size()) {
    std::size_t end = raw.find(';', start);
    if (end == std::string::npos) {
      end = raw.size();
    }
    const std::string decl(raw.substr(start, end - start));
    start = end + 1;
    if (decl.empty()) {
      continue;
    }
    const std::size_t colon = decl.find(':');
    if (colon == std::string::npos) {
      continue;
    }
    apply_style_property(style,
                         std::string_view(decl.data(), colon),
                         std::string_view(decl.data() + colon + 1, decl.size() - colon - 1),
                         css_variables);
  }
}

void apply_inline_style_attribute(ResolvedStyle& style, const HtmlNode& node) {
  static const std::unordered_map<std::string, std::string> kEmpty;
  apply_inline_style_attribute(style, node, kEmpty);
}

void apply_style_property(ResolvedStyle& style, std::string_view key, std::string_view value) {
  static const std::unordered_map<std::string, std::string> kEmpty;
  apply_style_property(style, key, value, kEmpty);
}

[[nodiscard]] std::vector<SimpleSelector> parse_selector_parts(std::string_view selector) {
  std::vector<SimpleSelector> parts;
  std::size_t i = 0;
  while (i < selector.size()) {
    while (i < selector.size() && std::isspace(static_cast<unsigned char>(selector[i])) != 0) ++i;
    if (i >= selector.size()) break;

    std::size_t j = i;
    while (j < selector.size() && std::isspace(static_cast<unsigned char>(selector[j])) == 0) ++j;
    const std::string token = to_lower(selector.substr(i, j - i));
    i = j;
    if (token.empty()) continue;
    if (token == ">" || token == "+" || token == "~") continue;
    if (token.find(':') != std::string::npos) {
      return {};
    }

    SimpleSelector part{};
    std::size_t p = 0;
    while (p < token.size()) {
      if (token[p] == '*') {
        part.universal = true;
        ++p;
        continue;
      }
      if (token[p] == '#') {
        ++p;
        const std::size_t begin = p;
        while (p < token.size() && token[p] != '.' && token[p] != '#' && token[p] != ':' && token[p] != '[') ++p;
        part.id = token.substr(begin, p - begin);
        continue;
      }
      if (token[p] == '.') {
        ++p;
        const std::size_t begin = p;
        while (p < token.size() && token[p] != '.' && token[p] != '#' && token[p] != ':' && token[p] != '[') ++p;
        const std::string klass = token.substr(begin, p - begin);
        if (!klass.empty()) {
          part.classes.push_back(klass);
        }
        continue;
      }
      if (token[p] == ':') {
        break;
      }

      const std::size_t begin = p;
      while (p < token.size() && token[p] != '.' && token[p] != '#' && token[p] != ':' && token[p] != '[') ++p;
      part.tag = token.substr(begin, p - begin);
    }
    parts.push_back(std::move(part));
  }
  return parts;
}

[[nodiscard]] bool node_matches_simple_selector(const HtmlNode& node, const SimpleSelector& selector) {
  if (!selector.universal && selector.tag.empty() && selector.id.empty() && selector.classes.empty()) {
    return false;
  }
  if (!selector.universal && !selector.tag.empty() && to_lower(node.tag) != selector.tag) {
    return false;
  }

  if (!selector.id.empty()) {
    const HtmlAttribute* id_attr = find_attribute(node, "id");
    if (id_attr == nullptr || to_lower(id_attr->value) != selector.id) {
      return false;
    }
  }

  if (!selector.classes.empty()) {
    const HtmlAttribute* class_attr = find_attribute(node, "class");
    if (class_attr == nullptr) {
      return false;
    }
    const auto classes = split_class_tokens(class_attr->value);
    for (const auto& required : selector.classes) {
      if (std::find(classes.begin(), classes.end(), required) == classes.end()) {
        return false;
      }
    }
  }
  return true;
}

[[nodiscard]] bool selector_matches_node(std::string_view selector,
                                         const std::vector<const HtmlNode*>& lineage,
                                         const HtmlNode& node) {
  const auto parts = parse_selector_parts(selector);
  if (parts.empty()) {
    return false;
  }
  if (!node_matches_simple_selector(node, parts.back())) {
    return false;
  }

  int lineage_index = static_cast<int>(lineage.size()) - 1;
  for (int part = static_cast<int>(parts.size()) - 2; part >= 0; --part) {
    bool found = false;
    while (lineage_index >= 0) {
      if (node_matches_simple_selector(*lineage[static_cast<std::size_t>(lineage_index)], parts[static_cast<std::size_t>(part)])) {
        found = true;
        --lineage_index;
        break;
      }
      --lineage_index;
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ResolvedStyle resolve_style(const HtmlDocument& document,
                                          const HtmlNode& node,
                                          const std::vector<const HtmlNode*>& lineage,
                                          const ResolvedStyle* parent_style,
                                          const int viewport_width) {
  ResolvedStyle style{};
  if (parent_style != nullptr) {
    style.font_size = parent_style->font_size;
    style.line_height = parent_style->line_height;
    style.font_weight = parent_style->font_weight;
    style.font_family = parent_style->font_family;
    style.text_color = parent_style->text_color;
    style.text_align = parent_style->text_align;
    style.text_transform = parent_style->text_transform;
  }

  style.margin = 0;
  style.padding = 0;
  style.gap = 0;
  style.border = 0;
  style.explicit_width = 0;
  style.explicit_height = 0;
  style.min_width = 0;
  style.min_height = 0;
  style.explicit_width_ratio = 0.0;
  style.explicit_height_ratio = 0.0;
  style.min_width_ratio = 0.0;
  style.min_height_ratio = 0.0;
  style.border_radius = 0;
  style.grid_min_column_width = 0;
  style.has_background = false;
  style.has_shadow = false;
  style.shadow_offset_x = 0;
  style.shadow_offset_y = 0;
  style.shadow_blur = 0;
  style.shadow_color = Color::rgba(22, 30, 42, 38);
  style.sticky_left_px = 0;
  style.sticky_top_px = 0;
  style.sticky_left_set = false;
  style.sticky_top_set = false;
  style.margin_left_auto = false;
  style.font_family.clear();
  style.position = "static";
  style.has_linear_gradient = false;
  style.gradient_horizontal = false;
  style.gradient_start = Color::rgba(255, 255, 255, 255);
  style.gradient_end = Color::rgba(245, 246, 248, 255);
  style.display = (node.kind == HtmlNodeKind::Text) ? "inline" : "block";
  style.flex_wrap = "nowrap";
  style.justify_content = "flex-start";
  style.white_space = "normal";
  if (style.text_transform.empty()) style.text_transform = "none";
  style.border_color = Color::rgba(208, 215, 222, 255);
  style.background_color = Color::rgba(255, 255, 255, 255);

  if (node.style.font_size_px > 0) style.font_size = clamp_font(node.style.font_size_px);
  if (node.style.line_height_px > 0) style.line_height = clamp_nonneg(node.style.line_height_px);
  if (node.style.margin_px > 0) style.margin = clamp_nonneg(node.style.margin_px);
  if (node.style.padding_px > 0) style.padding = clamp_nonneg(node.style.padding_px);
  if (node.style.border_px > 0) style.border = clamp_nonneg(node.style.border_px);
  if (node.style.border_radius_px > 0) style.border_radius = clamp_nonneg(node.style.border_radius_px);
  if (node.style.width_px > 0) style.explicit_width = clamp_nonneg(node.style.width_px);
  if (node.style.height_px > 0) style.explicit_height = clamp_nonneg(node.style.height_px);
  if (!node.style.display.empty()) style.display = to_lower(node.style.display);
  if (!node.style.text_align.empty()) style.text_align = to_lower(node.style.text_align);
  if (!node.style.color.empty()) {
    if (const auto c = parse_color(node.style.color); c.has_value()) style.text_color = *c;
  }
  if (!node.style.border_color.empty()) {
    if (const auto c = parse_color(node.style.border_color); c.has_value()) style.border_color = *c;
  }
  if (!node.style.background.empty()) {
    if (const auto gradient = parse_linear_gradient(node.style.background); gradient.has_value()) {
      style.has_linear_gradient = true;
      style.gradient_start = gradient->start;
      style.gradient_end = gradient->end;
      style.gradient_horizontal = gradient->horizontal;
      style.background_color = gradient->start;
      style.has_background = true;
    } else if (const auto c = parse_color(node.style.background); c.has_value()) {
      style.background_color = *c;
      style.has_background = c->a > 0;
    }
  }

  for (const auto& rule : document.style_rules) {
    if (rule.media_min_width_px > 0 && viewport_width < rule.media_min_width_px) {
      continue;
    }
    if (rule.media_max_width_px > 0 && viewport_width > rule.media_max_width_px) {
      continue;
    }
    if (!selector_matches_node(rule.selector, lineage, node)) {
      continue;
    }
    for (const auto& decl : rule.declarations) {
      apply_style_property(style, decl.name, decl.value, document.css_variables);
    }
  }

  apply_inline_style_attribute(style, node, document.css_variables);

  if (!style.explicit_width_set) {
    const HtmlAttribute* width_attr = find_attribute(node, "width");
    if (width_attr != nullptr) {
      const int width_px = clamp_nonneg(parse_css_length_px(width_attr->value, style.font_size));
      if (width_px > 0) {
        style.explicit_width = width_px;
      }
    }
  }
  if (!style.explicit_height_set) {
    const HtmlAttribute* height_attr = find_attribute(node, "height");
    if (height_attr != nullptr) {
      const int height_px = clamp_nonneg(parse_css_length_px(height_attr->value, style.font_size));
      if (height_px > 0) {
        style.explicit_height = height_px;
      }
    }
  }

  style.font_size = clamp_font(style.font_size);
  style.line_height = resolve_line_height_value(style.line_height, style.font_size);
  style.font_weight = std::clamp(style.font_weight, 100, 900);
  style.border_radius = std::max(0, style.border_radius);
  if (style.text_align.empty()) style.text_align = "left";
  if (style.display.empty()) style.display = "block";
  if (style.position.empty()) style.position = "static";
  if (style.flex_wrap.empty()) style.flex_wrap = "nowrap";
  if (style.justify_content.empty()) style.justify_content = "flex-start";
  if (style.white_space.empty()) style.white_space = "normal";
  if (style.text_transform.empty()) style.text_transform = "none";
  style.gap = std::max(0, style.gap);
  style.min_width = std::max(0, style.min_width);
  style.min_height = std::max(0, style.min_height);
  return style;
}

void collect_table_rows_recursive(const HtmlNode& node,
                                  std::vector<const HtmlNode*>& lineage,
                                  std::vector<TableRowData>& rows) {
  if (node.tag == "tr") {
    TableRowData row{};
    row.node = &node;
    row.lineage = lineage;
    for (const auto& child : node.children) {
      if (child.tag == "th" || child.tag == "td") {
        row.cells.push_back(TableCellData{
            .node = &child,
            .text = collect_node_text(child),
            .is_header = (child.tag == "th"),
        });
      }
    }
    if (!row.cells.empty()) {
      rows.push_back(std::move(row));
    }
    return;
  }

  lineage.push_back(&node);
  for (const auto& child : node.children) {
    collect_table_rows_recursive(child, lineage, rows);
  }
  lineage.pop_back();
}

[[nodiscard]] std::vector<TableRowData> extract_table_rows(const HtmlNode& table_node,
                                                           const std::vector<const HtmlNode*>& lineage) {
  std::vector<TableRowData> rows;
  std::vector<const HtmlNode*> table_lineage = lineage;
  collect_table_rows_recursive(table_node, table_lineage, rows);
  return rows;
}

[[nodiscard]] int resolve_length_px(const int absolute_px, const double ratio, const int reference_px) {
  if (absolute_px > 0) {
    return absolute_px;
  }
  if (ratio > 0.0 && reference_px > 0) {
    return std::max(0, static_cast<int>(std::lround(static_cast<double>(reference_px) * ratio)));
  }
  return 0;
}

[[nodiscard]] std::vector<int> extract_table_column_widths(const HtmlDocument& document,
                                                            const HtmlNode& table_node,
                                                            const std::vector<const HtmlNode*>& lineage,
                                                            const ResolvedStyle& table_style,
                                                            const int viewport_width,
                                                            const int reference_width) {
  std::vector<int> widths;
  std::vector<const HtmlNode*> column_lineage = lineage;
  column_lineage.push_back(&table_node);

  const auto visit = [&](const auto& self, const HtmlNode& node) -> void {
    if (node.tag == "col") {
      const ResolvedStyle column_style =
          resolve_style(document, node, column_lineage, &table_style, viewport_width);
      const int width = resolve_length_px(column_style.explicit_width,
                                          column_style.explicit_width_ratio,
                                          reference_width);
      const int minimum = resolve_length_px(column_style.min_width,
                                            column_style.min_width_ratio,
                                            reference_width);
      widths.push_back(std::max(width, minimum));
      return;
    }

    column_lineage.push_back(&node);
    for (const auto& child : node.children) {
      self(self, child);
    }
    column_lineage.pop_back();
  };

  for (const auto& child : table_node.children) {
    visit(visit, child);
  }
  return widths;
}

[[nodiscard]] int estimate_node_preferred_width(const HtmlDocument& document,
                                                const HtmlNode& node,
                                                std::vector<const HtmlNode*>& lineage,
                                                const ResolvedStyle* parent_style,
                                                const int cap_width,
                                                const int viewport_width) {
  const ResolvedStyle style = resolve_style(document, node, lineage, parent_style, viewport_width);

  int preferred = resolve_length_px(style.explicit_width, style.explicit_width_ratio, cap_width);
  if (preferred <= 0) {
    preferred = resolve_length_px(style.min_width, style.min_width_ratio, cap_width);
  }

  if (preferred <= 0 && node.kind == HtmlNodeKind::Text) {
    const std::string transformed = apply_text_transform(node.text, style.text_transform);
    const int natural_width = layout_math_text(transformed, style.font_size).width +
                              (style.padding * 2) + (style.border * 2) + 8;
    preferred = style.white_space == "nowrap" ? natural_width : std::min(natural_width, cap_width);
  }

  if (preferred <= 0 && (node.tag == "button" || node.tag == "input")) {
    std::string label = collect_node_text(node);
    if (label.empty()) {
      label = (node.tag == "input") ? "Search visible rows" : "Button";
    }
    label = apply_text_transform(label, style.text_transform);
    preferred = layout_math_text(truncate_to_width(label, style.font_size, 420), style.font_size).width +
                (style.padding * 2) + (style.border * 2) + 22;
  }

  if (preferred <= 0 && node.tag == "table") {
    // render_table/render_virtual_table both size a table to fill whatever
    // width they're given whenever explicit_width/min_width aren't set
    // (resolved_outer_w falls back to `width`, not to a shrink-to-fit
    // content measurement) -- so the table's *preferred* width, for the
    // parent's layout purposes, is that same cap, not a measurement of its
    // rows. Without this, a table with many/wide columns (or, for a
    // virtual table, literally no DOM rows to recurse into at all) reports
    // a preferred width equal to its full unclipped natural content size
    // (or the generic 140px fallback below, for the childless virtual
    // case) -- both far wider or narrower than what's actually painted --
    // and the document ends up drawing its own extra, spuriously-triggered
    // horizontal scrollbar to "reach" content that was never really there.
    preferred = cap_width;
  }

  if (preferred <= 0 && !node.children.empty()) {
    lineage.push_back(&node);
    if (style.display == "flex") {
      const int gap = std::max(0, style.gap);
      int sum = 0;
      bool has_child = false;
      for (const auto& child : node.children) {
        if (child.kind == HtmlNodeKind::Text && trim_copy(child.text).empty()) {
          continue;
        }
        const int child_w =
            estimate_node_preferred_width(document, child, lineage, &style, cap_width, viewport_width);
        if (has_child) {
          sum += gap;
        }
        sum += child_w;
        has_child = true;
      }
      preferred = style.flex_wrap == "nowrap" ? sum : std::min(sum, cap_width);
    } else {
      int max_child = 0;
      for (std::size_t i = 0; i < node.children.size(); ++i) {
        max_child = std::max(max_child,
                             estimate_node_preferred_width(
                                 document, node.children[i], lineage, &style, cap_width, viewport_width));
      }
      preferred = max_child;
    }
    lineage.pop_back();
  }

  if (preferred <= 0) {
    preferred = 140;
  }

  preferred += style.margin * 2;
  return std::clamp(preferred, 80, 32768);
}

[[nodiscard]] int measure_node_height(const HtmlDocument& document,
                                      const HtmlNode& node,
                                      int width,
                                      std::vector<const HtmlNode*>& lineage,
                                      const ResolvedStyle* parent_style,
                                      const int viewport_width);

[[nodiscard]] TableColumnPlan build_table_column_plan(const std::vector<TableRowData>& rows,
                                                      const ResolvedStyle& table_style,
                                                      const int inner_width,
                                                      const std::vector<int>& declared_widths = {}) {
  TableColumnPlan plan{};
  if (rows.empty() || inner_width <= 0) {
    return plan;
  }

  std::size_t col_count = 0;
  for (const auto& row : rows) {
    col_count = std::max(col_count, row.cells.size());
  }
  if (col_count == 0) {
    return plan;
  }

  std::vector<int> desired(col_count, 112);
  std::vector<bool> sticky_left(col_count, false);
  constexpr std::size_t kSampleRows = 48;
  const int probe_cap_px = std::clamp(inner_width * 2, 540, 1600);
  const std::size_t sample = std::min(rows.size(), kSampleRows);
  for (std::size_t c = 0; c < col_count; ++c) {
    for (std::size_t r = 0; r < rows.size(); ++r) {
      if (c < rows[r].cells.size() && node_has_class(rows[r].cells[c].node, "sticky-left")) {
        sticky_left[c] = true;
        break;
      }
    }
    const int declared_width = c < declared_widths.size() ? declared_widths[c] : 0;
    if (declared_width > 0) {
      desired[c] = declared_width;
      continue;
    }

    int max_w = sticky_left[c] ? 72 : 112;
    const std::size_t probe_rows = sticky_left[c] ? rows.size() : sample;
    for (std::size_t r = 0; r < probe_rows; ++r) {
      if (c >= rows[r].cells.size()) continue;
      const auto& cell = rows[r].cells[c];
      const std::string probe = sticky_left[c]
                                    ? cell.text
                                    : truncate_to_width(cell.text, table_style.font_size, probe_cap_px);
      const int text_w = layout_math_text(probe, table_style.font_size).width;
      const int pad = cell.is_header ? 36 : 28;
      const int capped_w = sticky_left[c]
                               ? text_w + pad
                               : std::min(probe_cap_px + pad, text_w + pad);
      max_w = std::max(max_w, capped_w);
    }
    desired[c] = max_w;
  }

  const int min_col_w = 104;
  const int max_col_w = std::max(260, inner_width * 2);
  for (std::size_t c = 0; c < col_count; ++c) {
    const int width = sticky_left[c]
                          ? std::clamp(desired[c], 72, max_col_w)
                          : std::clamp(desired[c], min_col_w, max_col_w);
    plan.source_indices.push_back(c);
    plan.widths.push_back(width);
  }

  if (plan.widths.empty()) {
    plan.source_indices.push_back(0);
    plan.widths.push_back(std::max(56, inner_width > 0 ? inner_width : 160));
  }

  int used = 0;
  for (const int w : plan.widths) {
    used += w;
  }
  if (used < inner_width && !plan.widths.empty()) {
    int extra = inner_width - used;
    std::vector<std::size_t> expandable;
    for (std::size_t c = 0; c < plan.widths.size(); ++c) {
      if (!sticky_left[c]) {
        expandable.push_back(c);
      }
    }
    if (expandable.empty()) {
      expandable.push_back(0);
    }
    std::size_t cursor = 0;
    while (extra > 0) {
      plan.widths[expandable[cursor % expandable.size()]] += 1;
      --extra;
      ++cursor;
    }
  }

  return plan;
}

[[nodiscard]] int measure_table_row_height(const HtmlDocument& document,
                                           const TableRowData& row,
                                           const TableColumnPlan& column_plan,
                                           const ResolvedStyle& table_style,
                                           const int viewport_width) {
  int row_height = std::max(24, table_style.line_height + 8);
  std::vector<const HtmlNode*> cell_lineage = row.lineage;
  if (row.node != nullptr) {
    cell_lineage.push_back(row.node);
  }

  for (std::size_t visible = 0; visible < column_plan.source_indices.size(); ++visible) {
    const std::size_t source_col = column_plan.source_indices[visible];
    if (source_col >= row.cells.size()) {
      continue;
    }
    const auto& cell = row.cells[source_col];
    const int cell_width = column_plan.widths[visible];
    ResolvedStyle cell_style = table_style;
    if (cell.node != nullptr) {
      cell_style = resolve_style(document, *cell.node, cell_lineage, &table_style, viewport_width);
    }
    const int line_height = resolve_line_height_value(cell_style.line_height, cell_style.font_size);
    const int cell_padding = std::max(2, cell_style.padding);
    const int inner_width = std::max(20, cell_width - ((cell_padding + cell_style.border) * 2) - 4);
    const int max_lines = cell.is_header ? 2 : 5;
    const auto wrapped = limit_lines(layout_text_lines(cell.text, cell_style, inner_width),
                                     max_lines,
                                     cell_style.font_size,
                                     inner_width);
    const int lines = std::max<int>(1, wrapped.size());
    const int content_h = lines * line_height;
    const int box_h = content_h + (cell_padding * 2) + (cell_style.border * 2) + 4;
    row_height = std::max(row_height, box_h);
  }

  return row_height;
}

[[nodiscard]] int measure_table_height(const HtmlDocument& document,
                                       const HtmlNode& node,
                                       const int width,
                                       const ResolvedStyle& style,
                                       const std::vector<const HtmlNode*>& lineage,
                                       const int viewport_width) {
  const int margin = style.margin;
  const int padding = style.padding;
  const int border = style.border;
  const int explicit_width = resolve_length_px(style.explicit_width, style.explicit_width_ratio, width);
  const int min_width = resolve_length_px(style.min_width, style.min_width_ratio, width);
  const int resolved_outer_width = std::max(min_width, explicit_width > 0 ? explicit_width : width);
  const int inner_width = std::max(40, resolved_outer_width - (margin + padding + border) * 2);

  // A fixed-height scrolling table does not need every cell measured during
  // the parent's height pass. The render pass computes its scroll geometry;
  // measuring all rows here duplicated that work on every scroll frame.
  if (style.explicit_height > 0) {
    return (margin * 2) + (padding * 2) + (border * 2) +
           std::max(style.explicit_height, style.min_height);
  }

  const auto rows = extract_table_rows(node, lineage);
  if (rows.empty()) {
    // A virtual table's rows never live in the DOM -- they're painted
    // straight from the model at render time (see render_virtual_table) --
    // so `rows` is always empty here for it. Without this check, this
    // function reports a ~40px height while render_virtual_table actually
    // paints a >=400px box, and every downstream measurement that trusts
    // this return value (sibling cursor_y, the document's own scroll
    // extents) goes stale relative to what's on screen.
    const HtmlAttribute* virtual_attr = find_attribute(node, "data-virtual-table");
    const bool is_virtual_table = virtual_attr != nullptr && virtual_attr->value == "true";
    const int empty_h = is_virtual_table
                             ? std::max({400, style.explicit_height, style.min_height})
                             : std::max(style.explicit_height, 40);
    return (margin * 2) + (padding * 2) + (border * 2) + empty_h;
  }

  std::size_t col_count = 0;
  for (const auto& row : rows) {
    col_count = std::max(col_count, row.cells.size());
  }
  if (col_count == 0) {
    return (margin * 2) + (padding * 2) + (border * 2) + 40;
  }

  const auto declared_widths =
      extract_table_column_widths(document, node, lineage, style, viewport_width, inner_width);
  const TableColumnPlan plan = build_table_column_plan(rows, style, inner_width, declared_widths);
  int total_rows_height = 0;
  for (const auto& row : rows) {
    total_rows_height += measure_table_row_height(document, row, plan, style, viewport_width);
  }

  int viewport_content_height = total_rows_height;
  if (style.explicit_height > 0) {
    viewport_content_height = style.explicit_height;
  }
  viewport_content_height = std::max(viewport_content_height, style.min_height);
  return (margin * 2) + (padding * 2) + (border * 2) + viewport_content_height;
}

[[nodiscard]] int measure_node_height(const HtmlDocument& document,
                                      const HtmlNode& node,
                                      int width,
                                      std::vector<const HtmlNode*>& lineage,
                                      const ResolvedStyle* parent_style,
                                      const int viewport_width) {
  if (width <= 0) {
    return 0;
  }

  const ResolvedStyle style = resolve_style(document, node, lineage, parent_style, viewport_width);
  if (style.display == "none") {
    return 0;
  }

  const int margin = style.margin;
  const int padding = style.padding;
  const int border = style.border;
  const int explicit_width = resolve_length_px(style.explicit_width, style.explicit_width_ratio, width);
  const int min_width = resolve_length_px(style.min_width, style.min_width_ratio, width);
  const int resolved_outer_width = std::max(min_width, explicit_width > 0 ? explicit_width : width);
  const int content_width = std::max(8, resolved_outer_width - (margin + padding + border) * 2);

  if (node.kind == HtmlNodeKind::Text) {
    const auto lines = layout_text_lines(node.text, style, content_width);
    const int line_h = resolve_line_height_value(style.line_height, style.font_size);
    return (margin * 2) + (padding * 2) + (border * 2) + (line_h * std::max<int>(1, lines.size()));
  }

  if (node.tag == "table") {
    return measure_table_height(document, node, width, style, lineage, viewport_width);
  }

  if (node.kind == HtmlNodeKind::Widget || node.tag == "canvas" || node.tag == "svg" || node.tag == "line-plot" ||
      node.tag == "chart") {
    const int box_h = std::max(style.min_height, style.explicit_height > 0 ? style.explicit_height : 240);
    return (margin * 2) + (padding * 2) + (border * 2) + box_h;
  }

  if (node.tag == "button") {
    const int natural_h = style.line_height + (padding * 2) + 8;
    const int box_h =
        std::max(std::max(30, style.min_height), style.explicit_height > 0 ? style.explicit_height : natural_h);
    return (margin * 2) + (border * 2) + box_h;
  }

  if (node.tag == "input") {
    const int box_h = std::max(std::max(30, style.min_height), style.explicit_height > 0 ? style.explicit_height : 34);
    return (margin * 2) + (border * 2) + box_h;
  }

  int content_height = 0;
  if (node.children.empty()) {
    content_height = std::max(style.min_height, style.explicit_height);
  } else {
    if (style.display == "grid") {
      const int gap = std::max(0, style.gap);
      const int min_col = std::max(120, style.grid_min_column_width > 0 ? style.grid_min_column_width : 180);
      const int cols = std::max(1, (content_width + gap) / std::max(1, min_col + gap));
      const int cell_w = std::max(70, (content_width - gap * (cols - 1)) / cols);

      lineage.push_back(&node);
      std::vector<int> row_heights;
      row_heights.reserve((node.children.size() / static_cast<std::size_t>(cols)) + 1);
      for (std::size_t i = 0; i < node.children.size(); ++i) {
        const int child_h =
            measure_node_height(document, node.children[i], cell_w, lineage, &style, viewport_width);
        const std::size_t row = i / static_cast<std::size_t>(cols);
        if (row >= row_heights.size()) {
          row_heights.push_back(child_h);
        } else {
          row_heights[row] = std::max(row_heights[row], child_h);
        }
      }
      lineage.pop_back();
      for (std::size_t i = 0; i < row_heights.size(); ++i) {
        content_height += row_heights[i];
        if (i + 1 < row_heights.size()) {
          content_height += gap;
        }
      }
    } else if (style.display == "flex") {
      const int gap = std::max(0, style.gap);
      const bool wrap = style.flex_wrap != "nowrap";
      std::vector<const HtmlNode*> flex_children;
      flex_children.reserve(node.children.size());
      for (const auto& child : node.children) {
        if (child.kind != HtmlNodeKind::Text || !trim_copy(child.text).empty()) {
          flex_children.push_back(&child);
        }
      }

      lineage.push_back(&node);
      int row_h = 0;
      int row_x = 0;
      int consumed_h = 0;
      const int fallback_w =
          std::max(96, content_width / std::max(1, static_cast<int>(flex_children.size())));

      for (const HtmlNode* child_ptr : flex_children) {
        const auto& child = *child_ptr;
        const ResolvedStyle child_style = resolve_style(document, child, lineage, &style, viewport_width);
        int child_w = resolve_length_px(child_style.explicit_width, child_style.explicit_width_ratio, content_width);
        if (child_w <= 0) child_w = resolve_length_px(child_style.min_width, child_style.min_width_ratio, content_width);
        if (child_w <= 0) {
          child_w = estimate_node_preferred_width(document, child, lineage, &style, content_width, viewport_width);
        }
        if (child_w <= 0) child_w = fallback_w;
        child_w = std::max(80, child_w);
        if (wrap) {
          child_w = std::min(child_w, std::max(80, content_width));
        }

        if (wrap && row_x > 0 && row_x + child_w > content_width) {
          consumed_h += row_h + gap;
          row_h = 0;
          row_x = 0;
        }

        const int child_h = measure_node_height(document, child, child_w, lineage, &style, viewport_width);
        row_h = std::max(row_h, child_h);
        row_x += child_w + gap;
      }
      lineage.pop_back();
      content_height = consumed_h + row_h;
    } else {
      lineage.push_back(&node);
      for (const auto& child : node.children) {
        content_height += measure_node_height(document, child, content_width, lineage, &style, viewport_width);
      }
      lineage.pop_back();
    }

    if (style.explicit_height > 0) {
      content_height = std::max(content_height, style.explicit_height);
    }
    content_height = std::max(content_height, style.min_height);
  }

  if (content_height == 0 && (node.tag == "hr")) {
    content_height = 2;
  }

  return (margin * 2) + (padding * 2) + (border * 2) + content_height;
}

void draw_text_lines(Scene& scene,
                     const std::vector<std::string>& lines,
                     const int font_size,
                     const int font_weight,
                     const std::string_view font_family,
                     const Color color,
                     const int x,
                     int y,
                     const int line_height,
                     const std::string_view text_align,
                     const int content_width) {
  for (const auto& line : lines) {
    const TextLayout layout = layout_math_text(line, font_size);
    int text_x = x;
    if (text_align == "center") {
      text_x = x + std::max(0, (content_width - layout.width) / 2);
    } else if (text_align == "right") {
      text_x = x + std::max(0, content_width - layout.width);
    }
    scene.add(TextCommand{PointI{text_x, y + layout.ascent}, line, font_size,
                          color, font_weight, std::string(font_family)});
    y += line_height;
  }
}

void draw_rect_fill(Scene& scene, const RectI rect, const Color color, const int radius) {
  if (rect.empty()) {
    return;
  }
  if (radius > 0) {
    scene.add(FillRoundedRectCommand{rect, radius, color});
  } else {
    scene.add(FillRectCommand{rect, color});
  }
}

void mask_rounded_viewport_corners(Scene& scene,
                                   const RectI rect,
                                   const int radius,
                                   const Color surround,
                                   const bool top,
                                   const bool bottom) {
  const int r = std::max(0, std::min(radius, std::min(rect.width, rect.height) / 2));
  if (rect.empty() || r <= 0 || (!top && !bottom)) {
    return;
  }

  constexpr double kHalfPi = 1.57079632679489661923;
  constexpr int kSegments = 8;
  const int right = rect.x + rect.width;
  const int lower = rect.y + rect.height;
  const auto corner = [&](const PointI outer,
                          const PointI center,
                          const double start_angle,
                          const double angle_delta) {
    std::vector<PointI> points;
    points.reserve(kSegments + 2);
    points.push_back(outer);
    for (int step = 0; step <= kSegments; ++step) {
      const double progress = static_cast<double>(step) / static_cast<double>(kSegments);
      const double angle = start_angle + angle_delta * progress;
      points.push_back(PointI{
          center.x + static_cast<int>(std::lround(std::cos(angle) * r)),
          center.y + static_cast<int>(std::lround(std::sin(angle) * r)),
      });
    }
    scene.add(FillPolygonCommand{std::move(points), surround});
  };

  if (top) {
    corner(PointI{rect.x, rect.y},
           PointI{rect.x + r, rect.y + r},
           -kHalfPi,
           -kHalfPi);
    corner(PointI{right, rect.y},
           PointI{right - r, rect.y + r},
           -kHalfPi,
           kHalfPi);
  }
  if (bottom) {
    corner(PointI{rect.x, lower},
           PointI{rect.x + r, lower - r},
           kHalfPi,
           kHalfPi);
    corner(PointI{right, lower},
           PointI{right - r, lower - r},
           kHalfPi,
           -kHalfPi);
  }
}

void draw_style_fill(Scene& scene, const RectI rect, const ResolvedStyle& style) {
  if (rect.empty() || !style.has_background) {
    return;
  }
  if (style.has_linear_gradient) {
    scene.add(FillLinearGradientCommand{
        .rect = rect,
        .start_color = style.gradient_start,
        .end_color = style.gradient_end,
        .horizontal = style.gradient_horizontal,
        .radius = style.border_radius,
    });
    return;
  }
  draw_rect_fill(scene, rect, style.background_color, style.border_radius);
}

void draw_rect_border(Scene& scene, const RectI rect, const int border, const Color color, const int radius) {
  if (border <= 0 || rect.empty()) {
    return;
  }
  if (radius > 0) {
    scene.add(StrokeRoundedRectCommand{rect, radius, border, color});
  } else {
    scene.add(StrokeRectCommand{rect, border, color});
  }
}

void draw_box_shadow(Scene& scene, const RectI rect, const ResolvedStyle& style) {
  if (!style.has_shadow || rect.empty()) {
    return;
  }

  const int blur = std::clamp(style.shadow_blur, 0, 64);
  const int layers = std::max(1, std::min(4, blur / 8 + 1));
  for (int layer = 0; layer < layers; ++layer) {
    const int expand = layer * std::max(1, blur / std::max(1, layers * 2));
    const RectI shadow_rect{
        rect.x + style.shadow_offset_x - expand,
        rect.y + style.shadow_offset_y - expand,
        rect.width + (expand * 2),
        rect.height + (expand * 2),
    };
    const int radius = std::max(0, style.border_radius + expand);
    const int alpha = std::max(6, static_cast<int>(style.shadow_color.a) / (layer + 2));
    draw_rect_fill(scene,
                   shadow_rect,
                   Color::rgba(style.shadow_color.r, style.shadow_color.g, style.shadow_color.b,
                               static_cast<std::uint8_t>(std::clamp(alpha, 0, 255))),
                   radius);
  }
}

struct SvgPointF {
  double x{0.0};
  double y{0.0};
};

struct SvgPathSubpath {
  std::vector<SvgPointF> points{};
  bool closed{false};
};

struct SvgViewport {
  double min_x{0.0};
  double min_y{0.0};
  double width{1.0};
  double height{1.0};
  double scale{1.0};
  double origin_x{0.0};
  double origin_y{0.0};
};

struct SvgPaint {
  bool fill_enabled{true};
  Color fill_color{Color::rgba(18, 24, 31, 255)};
  bool stroke_enabled{false};
  Color stroke_color{Color::rgba(18, 24, 31, 255)};
  int stroke_width{1};
  int font_size{12};
  std::string text_anchor{"start"};
};

[[nodiscard]] std::optional<std::string> svg_style_property(const HtmlNode& node, std::string_view name) {
  const HtmlAttribute* style_attr = find_attribute(node, "style");
  if (style_attr == nullptr || style_attr->value.empty()) {
    return std::nullopt;
  }

  const std::string needle = to_lower(trim_copy(name));
  const std::string style_text = style_attr->value;
  std::size_t start = 0;
  while (start < style_text.size()) {
    std::size_t end = style_text.find(';', start);
    if (end == std::string::npos) {
      end = style_text.size();
    }
    const std::string decl = trim_copy(std::string_view(style_text.data() + start, end - start));
    if (!decl.empty()) {
      const std::size_t colon = decl.find(':');
      if (colon != std::string::npos) {
        const std::string key = to_lower(trim_copy(std::string_view(decl.data(), colon)));
        if (key == needle) {
          return trim_copy(std::string_view(decl.data() + colon + 1, decl.size() - colon - 1));
        }
      }
    }
    start = end + 1;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string> svg_attr_or_style(const HtmlNode& node, std::string_view name) {
  if (const HtmlAttribute* attr = find_attribute(node, name); attr != nullptr) {
    return trim_copy(attr->value);
  }
  return svg_style_property(node, name);
}

[[nodiscard]] std::optional<double> svg_number(std::string_view text) {
  const std::string token = trim_copy(text);
  if (token.empty()) {
    return std::nullopt;
  }
  return parse_css_number(token);
}

[[nodiscard]] std::optional<double> svg_number_attr(const HtmlNode& node, std::string_view name) {
  const auto token = svg_attr_or_style(node, name);
  if (!token.has_value() || token->empty()) {
    return std::nullopt;
  }
  return svg_number(*token);
}

[[nodiscard]] std::vector<double> svg_number_list(std::string_view text) {
  std::string normalized;
  normalized.reserve(text.size());
  for (const char ch : text) {
    normalized.push_back(ch == ',' ? ' ' : ch);
  }
  const auto tokens = split_space_tokens(normalized);
  std::vector<double> out;
  out.reserve(tokens.size());
  for (const auto& token : tokens) {
    if (const auto n = parse_css_number(token); n.has_value()) {
      out.push_back(*n);
    }
  }
  return out;
}

[[nodiscard]] Color color_with_alpha(Color color, const double factor) {
  const double clamped = std::clamp(factor, 0.0, 1.0);
  color.a = static_cast<std::uint8_t>(
      std::clamp(static_cast<int>(std::lround(static_cast<double>(color.a) * clamped)), 0, 255));
  return color;
}

[[nodiscard]] SvgPaint resolve_svg_paint(const HtmlNode& node, const SvgPaint& parent) {
  SvgPaint paint = parent;

  if (const auto fill = svg_attr_or_style(node, "fill"); fill.has_value()) {
    const std::string token = to_lower(trim_copy(*fill));
    if (token == "none") {
      paint.fill_enabled = false;
    } else if (const auto parsed = parse_color(token); parsed.has_value()) {
      paint.fill_enabled = true;
      paint.fill_color = *parsed;
    }
  }

  if (const auto stroke = svg_attr_or_style(node, "stroke"); stroke.has_value()) {
    const std::string token = to_lower(trim_copy(*stroke));
    if (token == "none") {
      paint.stroke_enabled = false;
    } else if (const auto parsed = parse_color(token); parsed.has_value()) {
      paint.stroke_enabled = true;
      paint.stroke_color = *parsed;
    }
  }

  if (const auto stroke_w = svg_number_attr(node, "stroke-width"); stroke_w.has_value()) {
    paint.stroke_width = std::max(1, static_cast<int>(std::lround(*stroke_w)));
  }
  if (const auto font_size = svg_number_attr(node, "font-size"); font_size.has_value()) {
    paint.font_size = std::clamp(static_cast<int>(std::lround(*font_size)), 6, 72);
  }
  if (const auto anchor = svg_attr_or_style(node, "text-anchor"); anchor.has_value() && !anchor->empty()) {
    paint.text_anchor = to_lower(trim_copy(*anchor));
  }

  const double global_opacity = std::clamp(svg_number_attr(node, "opacity").value_or(1.0), 0.0, 1.0);
  const double fill_opacity = std::clamp(svg_number_attr(node, "fill-opacity").value_or(1.0), 0.0, 1.0);
  const double stroke_opacity = std::clamp(svg_number_attr(node, "stroke-opacity").value_or(1.0), 0.0, 1.0);

  paint.fill_color = color_with_alpha(paint.fill_color, global_opacity * fill_opacity);
  paint.stroke_color = color_with_alpha(paint.stroke_color, global_opacity * stroke_opacity);
  if (paint.fill_color.a == 0) {
    paint.fill_enabled = false;
  }
  if (paint.stroke_color.a == 0) {
    paint.stroke_enabled = false;
  }
  return paint;
}

[[nodiscard]] SvgViewport resolve_svg_viewport(const HtmlNode& svg_node,
                                               const RectI panel_rect,
                                               const double requested_scale) {
  SvgViewport viewport{};
  viewport.width = std::max(1.0, static_cast<double>(panel_rect.width));
  viewport.height = std::max(1.0, static_cast<double>(panel_rect.height));

  if (const auto view_box = svg_attr_or_style(svg_node, "viewbox"); view_box.has_value()) {
    const auto values = svg_number_list(*view_box);
    if (values.size() >= 4 && values[2] > 0.0 && values[3] > 0.0) {
      viewport.min_x = values[0];
      viewport.min_y = values[1];
      viewport.width = values[2];
      viewport.height = values[3];
    }
  }

  if (viewport.width <= 0.0 || viewport.height <= 0.0) {
    viewport.width = std::max(1.0, svg_number_attr(svg_node, "width").value_or(static_cast<double>(panel_rect.width)));
    viewport.height =
        std::max(1.0, svg_number_attr(svg_node, "height").value_or(static_cast<double>(panel_rect.height)));
  }

  const double sx = static_cast<double>(std::max(1, panel_rect.width)) / viewport.width;
  const double sy = static_cast<double>(std::max(1, panel_rect.height)) / viewport.height;
  viewport.scale = std::max(0.001, std::min(sx, sy)) *
                   std::clamp(requested_scale, 0.35, 3.0);
  viewport.origin_x =
      static_cast<double>(panel_rect.x) + (static_cast<double>(panel_rect.width) - viewport.width * viewport.scale) * 0.5;
  viewport.origin_y =
      static_cast<double>(panel_rect.y) + (static_cast<double>(panel_rect.height) - viewport.height * viewport.scale) * 0.5;
  return viewport;
}

[[nodiscard]] PointI map_svg_point(const SvgViewport& viewport, const SvgPointF point) {
  return PointI{
      static_cast<int>(std::lround(viewport.origin_x + (point.x - viewport.min_x) * viewport.scale)),
      static_cast<int>(std::lround(viewport.origin_y + (point.y - viewport.min_y) * viewport.scale)),
  };
}

[[nodiscard]] std::vector<SvgPointF> parse_svg_points(std::string_view points) {
  const auto values = svg_number_list(points);
  std::vector<SvgPointF> out;
  out.reserve(values.size() / 2);
  for (std::size_t i = 1; i < values.size(); i += 2) {
    out.push_back(SvgPointF{values[i - 1], values[i]});
  }
  return out;
}

[[nodiscard]] bool parse_path_number(std::string_view text, std::size_t& i, double& out) {
  while (i < text.size()) {
    const unsigned char ch = static_cast<unsigned char>(text[i]);
    if (std::isspace(ch) != 0 || ch == ',') {
      ++i;
      continue;
    }
    break;
  }
  if (i >= text.size()) {
    return false;
  }

  const std::size_t start = i;
  bool seen_digit = false;
  if (text[i] == '+' || text[i] == '-') {
    ++i;
  }
  while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
    seen_digit = true;
    ++i;
  }
  if (i < text.size() && text[i] == '.') {
    ++i;
    while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
      seen_digit = true;
      ++i;
    }
  }
  if (i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
    const std::size_t exp_pos = i++;
    if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
      ++i;
    }
    bool exp_digits = false;
    while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
      exp_digits = true;
      ++i;
    }
    if (!exp_digits) {
      i = exp_pos;
    }
  }
  if (!seen_digit || i <= start) {
    return false;
  }
  const auto parsed = parse_css_number(std::string_view(text.data() + start, i - start));
  if (!parsed.has_value()) {
    return false;
  }
  out = *parsed;
  return true;
}

[[nodiscard]] std::vector<SvgPathSubpath> parse_svg_path(std::string_view data) {
  std::vector<SvgPathSubpath> paths;
  SvgPathSubpath current{};
  double cur_x = 0.0;
  double cur_y = 0.0;
  double start_x = 0.0;
  double start_y = 0.0;
  char command = '\0';
  std::size_t i = 0;

  auto flush_path = [&]() {
    if (!current.points.empty()) {
      paths.push_back(current);
      current = SvgPathSubpath{};
    }
  };

  while (i < data.size()) {
    while (i < data.size() && (std::isspace(static_cast<unsigned char>(data[i])) != 0 || data[i] == ',')) {
      ++i;
    }
    if (i >= data.size()) {
      break;
    }

    if (std::isalpha(static_cast<unsigned char>(data[i])) != 0) {
      command = data[i++];
      if (command == 'z' || command == 'Z') {
        current.closed = true;
        cur_x = start_x;
        cur_y = start_y;
        continue;
      }
    }
    if (command == '\0') {
      ++i;
      continue;
    }

    const bool relative = std::islower(static_cast<unsigned char>(command)) != 0;
    const char op = static_cast<char>(std::toupper(static_cast<unsigned char>(command)));

    if (op == 'M') {
      double x = 0.0;
      double y = 0.0;
      if (!parse_path_number(data, i, x) || !parse_path_number(data, i, y)) {
        continue;
      }
      flush_path();
      cur_x = relative ? (cur_x + x) : x;
      cur_y = relative ? (cur_y + y) : y;
      start_x = cur_x;
      start_y = cur_y;
      current.points.push_back(SvgPointF{cur_x, cur_y});

      while (true) {
        std::size_t probe = i;
        double px = 0.0;
        double py = 0.0;
        if (!parse_path_number(data, probe, px) || !parse_path_number(data, probe, py)) {
          break;
        }
        i = probe;
        cur_x = relative ? (cur_x + px) : px;
        cur_y = relative ? (cur_y + py) : py;
        current.points.push_back(SvgPointF{cur_x, cur_y});
      }
      continue;
    }

    if (op == 'L') {
      while (true) {
        std::size_t probe = i;
        double x = 0.0;
        double y = 0.0;
        if (!parse_path_number(data, probe, x) || !parse_path_number(data, probe, y)) {
          break;
        }
        i = probe;
        cur_x = relative ? (cur_x + x) : x;
        cur_y = relative ? (cur_y + y) : y;
        current.points.push_back(SvgPointF{cur_x, cur_y});
      }
      continue;
    }

    if (op == 'H') {
      while (true) {
        std::size_t probe = i;
        double x = 0.0;
        if (!parse_path_number(data, probe, x)) {
          break;
        }
        i = probe;
        cur_x = relative ? (cur_x + x) : x;
        current.points.push_back(SvgPointF{cur_x, cur_y});
      }
      continue;
    }

    if (op == 'V') {
      while (true) {
        std::size_t probe = i;
        double y = 0.0;
        if (!parse_path_number(data, probe, y)) {
          break;
        }
        i = probe;
        cur_y = relative ? (cur_y + y) : y;
        current.points.push_back(SvgPointF{cur_x, cur_y});
      }
      continue;
    }

    ++i;
  }

  flush_path();
  return paths;
}

void render_svg_children(Scene& scene, const HtmlNode& node, const SvgViewport& viewport, const SvgPaint& inherited) {
  for (const auto& child : node.children) {
    const std::string tag = to_lower(child.tag);
    const SvgPaint paint = resolve_svg_paint(child, inherited);

    if (tag == "g" || tag == "svg") {
      render_svg_children(scene, child, viewport, paint);
      continue;
    }

    if (tag == "rect") {
      const double x = svg_number_attr(child, "x").value_or(0.0);
      const double y = svg_number_attr(child, "y").value_or(0.0);
      const double w = std::max(0.0, svg_number_attr(child, "width").value_or(0.0));
      const double h = std::max(0.0, svg_number_attr(child, "height").value_or(0.0));
      if (w <= 0.0 || h <= 0.0) {
        continue;
      }
      const PointI p0 = map_svg_point(viewport, SvgPointF{x, y});
      const PointI p1 = map_svg_point(viewport, SvgPointF{x + w, y + h});
      const RectI rect{
          std::min(p0.x, p1.x),
          std::min(p0.y, p1.y),
          std::max(1, std::abs(p1.x - p0.x)),
          std::max(1, std::abs(p1.y - p0.y)),
      };
      if (paint.fill_enabled) {
        scene.add(FillRectCommand{rect, paint.fill_color});
      }
      if (paint.stroke_enabled) {
        const int stroke_px = std::max(1, static_cast<int>(std::lround(static_cast<double>(paint.stroke_width) * viewport.scale)));
        scene.add(StrokeRectCommand{rect, stroke_px, paint.stroke_color});
      }
      continue;
    }

    if (tag == "line") {
      const SvgPointF a{
          svg_number_attr(child, "x1").value_or(0.0),
          svg_number_attr(child, "y1").value_or(0.0),
      };
      const SvgPointF b{
          svg_number_attr(child, "x2").value_or(0.0),
          svg_number_attr(child, "y2").value_or(0.0),
      };
      const int stroke_px = std::max(1, static_cast<int>(std::lround(static_cast<double>(paint.stroke_width) * viewport.scale)));
      const Color stroke_color = paint.stroke_enabled ? paint.stroke_color : paint.fill_color;
      scene.add(LineCommand{map_svg_point(viewport, a), map_svg_point(viewport, b), stroke_px, stroke_color});
      continue;
    }

    if (tag == "polyline" || tag == "polygon") {
      const auto points_attr = svg_attr_or_style(child, "points");
      if (!points_attr.has_value()) {
        continue;
      }
      const auto points_f = parse_svg_points(*points_attr);
      if (points_f.size() < 2) {
        continue;
      }
      std::vector<PointI> points;
      points.reserve(points_f.size() + 1);
      for (const auto& point : points_f) {
        points.push_back(map_svg_point(viewport, point));
      }
      if (tag == "polygon" && paint.fill_enabled && points.size() >= 3) {
        scene.add(FillPolygonCommand{points, paint.fill_color});
      }
      if (paint.stroke_enabled) {
        std::vector<PointI> stroke_points = points;
        if (tag == "polygon") {
          stroke_points.push_back(stroke_points.front());
        }
        const int stroke_px = std::max(1, static_cast<int>(std::lround(static_cast<double>(paint.stroke_width) * viewport.scale)));
        scene.add(PolylineCommand{std::move(stroke_points), stroke_px, paint.stroke_color});
      }
      continue;
    }

    if (tag == "path") {
      const auto data_attr = svg_attr_or_style(child, "d");
      if (!data_attr.has_value() || data_attr->empty()) {
        continue;
      }
      const auto paths = parse_svg_path(*data_attr);
      const int stroke_px = std::max(1, static_cast<int>(std::lround(static_cast<double>(paint.stroke_width) * viewport.scale)));
      for (const auto& subpath : paths) {
        if (subpath.points.size() < 2) {
          continue;
        }
        std::vector<PointI> points;
        points.reserve(subpath.points.size() + (subpath.closed ? 1 : 0));
        for (const auto& point : subpath.points) {
          points.push_back(map_svg_point(viewport, point));
        }
        if (paint.fill_enabled && subpath.closed && points.size() >= 3) {
          scene.add(FillPolygonCommand{points, paint.fill_color});
        }
        if (paint.stroke_enabled) {
          if (subpath.closed) {
            points.push_back(points.front());
          }
          scene.add(PolylineCommand{std::move(points), stroke_px, paint.stroke_color});
        }
      }
      continue;
    }

    if (tag == "circle" || tag == "ellipse") {
      const double cx = svg_number_attr(child, "cx").value_or(0.0);
      const double cy = svg_number_attr(child, "cy").value_or(0.0);
      const double rx = (tag == "circle") ? svg_number_attr(child, "r").value_or(0.0)
                                          : svg_number_attr(child, "rx").value_or(0.0);
      const double ry = (tag == "circle") ? rx : svg_number_attr(child, "ry").value_or(0.0);
      if (rx <= 0.0 || ry <= 0.0) {
        continue;
      }
      const int samples = std::clamp(static_cast<int>(std::lround(std::max(rx, ry) * viewport.scale * 0.6)), 18, 96);
      std::vector<PointI> ring;
      ring.reserve(static_cast<std::size_t>(samples));
      for (int i = 0; i < samples; ++i) {
        const double t = (static_cast<double>(i) * 2.0 * 3.14159265358979323846) / static_cast<double>(samples);
        ring.push_back(map_svg_point(viewport, SvgPointF{cx + std::cos(t) * rx, cy + std::sin(t) * ry}));
      }
      if (paint.fill_enabled && ring.size() >= 3) {
        scene.add(FillPolygonCommand{ring, paint.fill_color});
      }
      if (paint.stroke_enabled && ring.size() >= 2) {
        std::vector<PointI> stroke_ring = ring;
        stroke_ring.push_back(stroke_ring.front());
        const int stroke_px = std::max(1, static_cast<int>(std::lround(static_cast<double>(paint.stroke_width) * viewport.scale)));
        scene.add(PolylineCommand{std::move(stroke_ring), stroke_px, paint.stroke_color});
      }
      continue;
    }

    if (tag == "text") {
      const std::string text = collect_node_text(child);
      if (text.empty()) {
        continue;
      }
      PointI point = map_svg_point(
          viewport,
          SvgPointF{svg_number_attr(child, "x").value_or(0.0), svg_number_attr(child, "y").value_or(0.0)});
      const int font_px = std::max(8, static_cast<int>(std::lround(static_cast<double>(paint.font_size) * viewport.scale)));
      const TextLayout layout = layout_math_text(text, font_px);
      if (paint.text_anchor == "middle") {
        point.x -= layout.width / 2;
      } else if (paint.text_anchor == "end") {
        point.x -= layout.width;
      }
      const Color color = paint.fill_enabled ? paint.fill_color : paint.stroke_color;
      scene.add(TextCommand{point, text, font_px, color, 400, "Helvetica, Arial, sans-serif"});
      continue;
    }

    render_svg_children(scene, child, viewport, paint);
  }
}

int render_node(Scene& scene,
                const HtmlDocument& document,
                const HtmlNode& node,
                int x,
                int y,
                int width,
                int bottom,
                std::vector<const HtmlNode*>& lineage,
                const HtmlRenderConfig& render_config,
                const ResolvedStyle* parent_style,
                const int viewport_width);

int render_virtual_table(
    Scene& scene,
    const HtmlNode& node,
    const int x,
    const int y,
    const int width,
    const int bottom,
    const ResolvedStyle& style,
    const HtmlRenderConfig& render_config,
    const ResolvedStyle* parent_style) {
  const auto* model = render_config.virtual_table_model;
  if (model == nullptr) {
    return 0;
  }

  const int margin = style.margin;
  const int padding = style.padding;
  const int border = style.border;
  const int content_height =
      std::max({400, style.explicit_height, style.min_height});
  const int total_h =
      margin * 2 + padding * 2 + border * 2 + content_height;
  if (y >= bottom) {
    return total_h;
  }

  const int explicit_width =
      resolve_length_px(style.explicit_width, style.explicit_width_ratio, width);
  const int min_width =
      resolve_length_px(style.min_width, style.min_width_ratio, width);
  const int resolved_outer_w =
      std::max(min_width, explicit_width > 0 ? explicit_width : width);
  const int outer_x = x + margin;
  const int outer_y = y + margin;
  const int outer_w = std::max(20, resolved_outer_w - margin * 2);
  const int outer_h = std::max(24, total_h - margin * 2);
  const RectI outer{outer_x, outer_y, outer_w, outer_h};
  draw_box_shadow(scene, outer, style);
  draw_style_fill(scene, outer, style);
  draw_rect_border(scene, outer, border, style.border_color,
                   style.border_radius);

  const int content_x = outer_x + border + padding;
  const int content_y = outer_y + border + padding;
  const int content_w = std::max(40, outer_w - (border + padding) * 2);
  const int viewport_h = std::max(24, outer_h - (border + padding) * 2);
  const RectI viewport_clip{content_x, content_y, content_w, viewport_h};
  const Color viewport_surround =
      parent_style != nullptr && parent_style->has_background
          ? parent_style->background_color
          : style.background_color;

  constexpr int header_h = 42;
  constexpr int row_h = 38;
  constexpr int row_header_w = 84;
  constexpr int min_text_column_w = 150;
  constexpr int min_numeric_column_w = 132;
  constexpr int max_column_w = 280;
  const std::int64_t model_row_count =
      std::max<std::int64_t>(0, model->row_count());
  const auto* row_order = render_config.virtual_row_order;
  const std::int64_t row_count =
      row_order != nullptr
          ? static_cast<std::int64_t>(row_order->size())
          : model_row_count;
  const std::int64_t column_count =
      std::max<std::int64_t>(0, model->column_count());
  const std::string table_target = node_instance_target(node, "table");

  std::vector<int> column_widths;
  std::int64_t data_width = 0;
  bool cached_widths = false;
  if (render_config.layout_cache != nullptr) {
    const auto cached =
        render_config.layout_cache->tables.find(table_target);
    if (cached != render_config.layout_cache->tables.end()) {
      const auto& entry = cached->second;
      cached_widths =
          entry.node_identity == reinterpret_cast<std::uintptr_t>(model) &&
          entry.content_width == content_w &&
          entry.column_count == static_cast<std::size_t>(column_count) &&
          entry.column_widths.size() ==
              static_cast<std::size_t>(column_count);
      if (cached_widths) {
        column_widths = entry.column_widths;
      }
    }
  }
  if (!cached_widths) {
    column_widths.reserve(static_cast<std::size_t>(
        std::min<std::int64_t>(column_count, 1000000)));
    for (std::int64_t column = 0; column < column_count; ++column) {
      const std::string name = model->column_name(column);
      const int preferred =
          layout_math_text(name, style.font_size).width + 24;
      const int minimum = model->column_numeric(column)
                              ? min_numeric_column_w
                              : min_text_column_w;
      column_widths.push_back(
          std::clamp(preferred, minimum, max_column_w));
    }
    if (render_config.layout_cache != nullptr) {
      render_config.layout_cache->tables[table_target] =
          HtmlTableGeometryCacheEntry{
              .node_identity = reinterpret_cast<std::uintptr_t>(model),
              .viewport_width = content_w,
              .content_width = content_w,
              .row_count = static_cast<std::size_t>(model_row_count),
              .column_count = static_cast<std::size_t>(column_count),
              .column_widths = column_widths,
          };
    }
  }
  for (const int cell_w : column_widths) {
    data_width = std::min<std::int64_t>(
        static_cast<std::int64_t>(std::numeric_limits<int>::max()),
        data_width + cell_w);
  }

  const int body_viewport_h = std::max(0, viewport_h - header_h);
  const int scrollable_view_w = std::max(16, content_w - row_header_w);
  const int max_scroll_x = static_cast<int>(std::max<std::int64_t>(
      0, data_width - static_cast<std::int64_t>(scrollable_view_w)));
  const std::int64_t body_height = std::min<std::int64_t>(
      static_cast<std::int64_t>(std::numeric_limits<int>::max()),
      row_count * static_cast<std::int64_t>(row_h));
  const int max_scroll_y = static_cast<int>(std::max<std::int64_t>(
      0, body_height - static_cast<std::int64_t>(body_viewport_h)));
  int requested_scroll_x = render_config.table_scroll_x_px;
  int requested_scroll_y = render_config.table_scroll_y_px;
  if (render_config.table_scroll_offsets != nullptr) {
    const auto offset =
        render_config.table_scroll_offsets->find(table_target);
    if (offset != render_config.table_scroll_offsets->end()) {
      requested_scroll_x = offset->second.x;
      requested_scroll_y = offset->second.y;
    }
  }
  const int scroll_x = std::clamp(requested_scroll_x, 0, max_scroll_x);
  const int scroll_y = std::clamp(requested_scroll_y, 0, max_scroll_y);

  RectI interactive_viewport = viewport_clip;
  if (render_config.render_stats != nullptr) {
    auto& stats = *render_config.render_stats;
    interactive_viewport =
        intersect_rect(viewport_clip, stats.document_viewport_rect);
    const int current_area =
        stats.table_viewport_rect.width * stats.table_viewport_rect.height;
    const int candidate_area =
        interactive_viewport.width * interactive_viewport.height;
    stats.has_table = true;
    if (candidate_area >= current_area) {
      stats.table_viewport_rect = interactive_viewport;
      stats.table_max_scroll_x = max_scroll_x;
      stats.table_max_scroll_y = max_scroll_y;
      stats.table_body_row_height_px = row_h;
    }
  }

  struct VisibleColumn {
    std::int64_t index{0};
    int x{0};
    int width{0};
  };
  std::vector<VisibleColumn> visible_columns;
  visible_columns.reserve(
      static_cast<std::size_t>(std::max(2, content_w / min_numeric_column_w + 2)));
  std::int64_t running_x = static_cast<std::int64_t>(content_x) +
                           row_header_w - scroll_x;
  const int scrollable_left = content_x + row_header_w;
  const int scrollable_right = content_x + content_w;
  for (std::int64_t column = 0; column < column_count; ++column) {
    const int cell_w = column_widths[static_cast<std::size_t>(column)];
    const std::int64_t x0 = running_x;
    const std::int64_t x1 = x0 + cell_w;
    if (x1 > scrollable_left && x0 < scrollable_right) {
      visible_columns.push_back(VisibleColumn{
          .index = column,
          .x = static_cast<int>(x0),
          .width = cell_w,
      });
    } else if (x0 >= scrollable_right) {
      break;
    }
    running_x = x1;
  }

  const Color grid = Color::rgba(216, 222, 228, 255);
  const Color header_fill = Color::rgba(220, 239, 238, 255);
  const Color sticky_fill = Color::rgba(237, 246, 247, 255);
  const Color sticky_header_fill = Color::rgba(204, 232, 230, 255);
  const int font_size = std::max(10, style.font_size);
  const int line_h = resolve_line_height_value(style.line_height, font_size);

  const auto draw_cell_text =
      [&](const std::string& source,
          const int cell_x,
          const int cell_y,
          const int cell_w,
          const bool header,
          const bool missing) {
        const int pad = 10;
        const int text_w = std::max(16, cell_w - pad * 2);
        const std::string text =
            truncate_to_width(source, font_size, text_w);
        const int text_y =
            cell_y + std::max(2, ((header ? header_h : row_h) - line_h) / 2);
        draw_text_lines(
            scene, {text}, font_size, header ? 600 : 400, style.font_family,
            missing ? Color::rgba(201, 54, 43, 255) : style.text_color,
            cell_x + pad, text_y, line_h, header ? "center" : "left", text_w);
      };

  if (render_config.clip_table_viewport) {
    scene.add(SetClipRectCommand{viewport_clip});
  }
  draw_rect_fill(scene,
                 RectI{content_x, content_y, content_w, header_h},
                 header_fill, std::min(style.border_radius, header_h / 2));
  if (style.border_radius > 0) {
    scene.add(FillRectCommand{
        RectI{content_x, content_y + style.border_radius, content_w,
              std::max(0, header_h - style.border_radius)},
        header_fill});
  }
  for (const auto& column : visible_columns) {
    scene.add(LineCommand{PointI{column.x, content_y},
                          PointI{column.x, content_y + viewport_h}, 1, grid});
    draw_cell_text(model->column_name(column.index), column.x, content_y,
                   column.width, true, false);
  }

  const int body_y = content_y + header_h;
  if (render_config.clip_table_viewport) {
    scene.add(ClearClipRectCommand{});
    scene.add(SetClipRectCommand{
        RectI{content_x, body_y, content_w, body_viewport_h}});
  }
  const std::int64_t first_row =
      row_h > 0 ? static_cast<std::int64_t>(scroll_y / row_h) : 0;
  int row_y = body_y - (scroll_y % row_h);
  const int body_bottom = body_y + body_viewport_h;
  // Stop before a row that would extend past the viewport rather than
  // drawing it and trusting the clip rect to cut it off cleanly -- see the
  // identical fix in render_table's row loop for why.
  for (std::int64_t display_row = first_row;
       display_row < row_count && row_y + row_h <= body_bottom;
       ++display_row, row_y += row_h) {
    if (row_y + row_h <= body_y) {
      continue;
    }
    const std::int64_t source_row =
        row_order != nullptr
            ? (*row_order)[static_cast<std::size_t>(display_row)]
            : display_row;
    const Color row_fill =
        (display_row % 2 == 0) ? Color::rgba(255, 255, 255, 255)
                               : Color::rgba(248, 250, 252, 255);
    scene.add(FillRectCommand{
        RectI{content_x, row_y, content_w, row_h}, row_fill});
    scene.add(LineCommand{PointI{content_x, row_y},
                          PointI{content_x + content_w, row_y}, 1, grid});
    for (const auto& column : visible_columns) {
      const VirtualTableCell value = model->cell(source_row, column.index);
      if (value.missing) {
        scene.add(FillRectCommand{
            RectI{column.x + 1, row_y + 1,
                  std::max(1, column.width - 2), std::max(1, row_h - 2)},
            Color::rgba(255, 240, 238, 255)});
      }
      scene.add(LineCommand{PointI{column.x, row_y},
                            PointI{column.x, row_y + row_h}, 1, grid});
      draw_cell_text(value.text, column.x, row_y, column.width, false,
                     value.missing);
    }
  }

  row_y = body_y - (scroll_y % row_h);
  for (std::int64_t display_row = first_row;
       display_row < row_count && row_y < body_bottom;
       ++display_row, row_y += row_h) {
    if (row_y + row_h <= body_y) {
      continue;
    }
    const std::int64_t source_row =
        row_order != nullptr
            ? (*row_order)[static_cast<std::size_t>(display_row)]
            : display_row;
    scene.add(FillRectCommand{
        RectI{content_x, row_y, row_header_w, row_h}, sticky_fill});
    scene.add(LineCommand{PointI{content_x, row_y},
                          PointI{content_x + row_header_w, row_y}, 1, grid});
    draw_cell_text(model->row_label(source_row), content_x, row_y,
                   row_header_w, false, false);
  }
  if (row_count == 0) {
    draw_text_lines(scene, {"No matching rows"}, font_size, 500,
                    style.font_family, Color::rgba(99, 111, 124, 255),
                    content_x + row_header_w + 12,
                    body_y + std::max(6, (body_viewport_h - line_h) / 2),
                    line_h, "left",
                    std::max(20, content_w - row_header_w - 24));
  }
  if (render_config.clip_table_viewport) {
    scene.add(ClearClipRectCommand{});
    scene.add(SetClipRectCommand{viewport_clip});
  }
  scene.add(FillRectCommand{
      RectI{content_x, content_y, row_header_w, header_h},
      sticky_header_fill});
  draw_cell_text(model->row_header(), content_x, content_y, row_header_w,
                 true, false);
  scene.add(LineCommand{
      PointI{content_x + row_header_w, content_y},
      PointI{content_x + row_header_w, content_y + viewport_h}, 2,
      Color::rgba(79, 137, 148, 255)});
  mask_rounded_viewport_corners(scene, viewport_clip, style.border_radius,
                                viewport_surround, true, true);
  if (render_config.clip_table_viewport) {
    scene.add(ClearClipRectCommand{});
  }

  RectI horizontal_track{};
  RectI horizontal_thumb{};
  RectI vertical_track{};
  RectI vertical_thumb{};
  if (max_scroll_x > 0) {
    const int track_h = 10;
    // Anchor to the content's actual bottom edge first, falling back to the
    // outer-box heuristic only when that leaves enough room -- the old
    // heuristic alone (padding / 2 + track_h) could be narrower than the
    // real border+padding gutter, which let the track sit on top of the
    // table's last visible row instead of below it.
    const int below_content = content_y + viewport_h + 2;
    const int near_outer_edge =
        outer_y + outer_h - std::max(14, padding / 2 + track_h);
    const int track_y = std::max(below_content, near_outer_edge);
    const int track_w = std::max(36, scrollable_view_w - 6);
    const int track_x = content_x + row_header_w + 3;
    horizontal_track = RectI{track_x, track_y, track_w, track_h};
    const double ratio =
        static_cast<double>(scrollable_view_w) /
        static_cast<double>(std::max<std::int64_t>(1, data_width));
    const int thumb_w =
        std::min(track_w, std::max(24, static_cast<int>(
            std::lround(static_cast<double>(track_w) * ratio))));
    const int thumb_x =
        track_x + static_cast<int>(std::lround(
                      static_cast<double>(track_w - thumb_w) * scroll_x /
                      std::max(1, max_scroll_x)));
    horizontal_thumb = RectI{thumb_x, track_y, thumb_w, track_h};
    if (render_config.draw_scrollbars) {
      draw_rect_fill(scene, horizontal_track,
                     Color::rgba(218, 224, 231, 238), 5);
      draw_rect_fill(scene, horizontal_thumb,
                     Color::rgba(94, 111, 130, 255), 5);
    }
  }
  if (max_scroll_y > 0) {
    const int track_w = 10;
    // Same reasoning as the horizontal track above: anchor to the content's
    // actual right edge first so the track can never land on top of the
    // rightmost visible column, only falling back to the outer-box
    // heuristic when that's already clear of content.
    const int right_of_content = content_x + content_w + 2;
    const int near_outer_edge =
        outer_x + outer_w - std::max(14, padding / 2 + track_w);
    const int track_x = std::max(right_of_content, near_outer_edge);
    const int track_h = std::max(36, body_viewport_h - 6);
    const int track_y = body_y + 3;
    vertical_track = RectI{track_x, track_y, track_w, track_h};
    const double ratio =
        static_cast<double>(body_viewport_h) /
        static_cast<double>(std::max<std::int64_t>(1, body_height));
    const int thumb_h =
        std::min(track_h, std::max(24, static_cast<int>(
            std::lround(static_cast<double>(track_h) * ratio))));
    const int thumb_y =
        track_y + static_cast<int>(std::lround(
                      static_cast<double>(track_h - thumb_h) * scroll_y /
                      std::max(1, max_scroll_y)));
    vertical_thumb = RectI{track_x, thumb_y, track_w, thumb_h};
    if (render_config.draw_scrollbars) {
      draw_rect_fill(scene, vertical_track,
                     Color::rgba(218, 224, 231, 238), 5);
      draw_rect_fill(scene, vertical_thumb,
                     Color::rgba(94, 111, 130, 255), 5);
    }
  }
  if (render_config.render_stats != nullptr) {
    render_config.render_stats->table_scroll_regions.push_back(
        HtmlScrollRegion{
            .target = table_target,
            .viewport_rect = interactive_viewport,
            .max_scroll_x = max_scroll_x,
            .max_scroll_y = max_scroll_y,
            .horizontal_track_rect = horizontal_track,
            .horizontal_thumb_rect = horizontal_thumb,
            .vertical_track_rect = vertical_track,
            .vertical_thumb_rect = vertical_thumb,
        });
  }
  return total_h;
}

int render_table(Scene& scene,
                 const HtmlDocument& document,
                 const HtmlNode& node,
                 int x,
                 int y,
                 int width,
                 int bottom,
                 std::vector<const HtmlNode*>& lineage,
                 const HtmlRenderConfig& render_config,
                 const ResolvedStyle* parent_style,
                 const int viewport_width) {
  const ResolvedStyle style = resolve_style(document, node, lineage, parent_style, viewport_width);
  if (render_config.virtual_table_model != nullptr) {
    return render_virtual_table(scene, node, x, y, width, bottom, style,
                                render_config, parent_style);
  }
  const int total_h = measure_table_height(document, node, width, style, lineage, viewport_width);
  if (total_h <= 0 || y >= bottom) {
    return 0;
  }

  const int margin = style.margin;
  const int padding = style.padding;
  const int border = style.border;
  const int explicit_width = resolve_length_px(style.explicit_width, style.explicit_width_ratio, width);
  const int min_width = resolve_length_px(style.min_width, style.min_width_ratio, width);
  const int resolved_outer_w = std::max(min_width, explicit_width > 0 ? explicit_width : width);

  const int outer_x = x + margin;
  const int outer_y = y + margin;
  const int outer_w = std::max(20, resolved_outer_w - margin * 2);
  const int outer_h = total_h - margin * 2;
  const RectI outer{outer_x, outer_y, outer_w, outer_h};
  draw_box_shadow(scene, outer, style);
  draw_style_fill(scene, outer, style);
  draw_rect_border(scene, outer, border, style.border_color, style.border_radius);

  auto rows = extract_table_rows(node, lineage);
  if (rows.empty()) {
    scene.add(TextCommand{PointI{outer_x + 8, outer_y + 24}, "Empty table", 12, style.text_color});
    return total_h;
  }
  const std::string table_target = node_instance_target(node, "table");

  const int content_x = outer_x + border + padding;
  const int content_y = outer_y + border + padding;
  const int content_w = std::max(40, outer_w - (border + padding) * 2);
  std::size_t column_count = 0;
  for (const auto& row : rows) {
    column_count = std::max(column_count, row.cells.size());
  }

  TableColumnPlan plan;
  std::vector<int> row_heights;
  bool geometry_cache_hit = false;
  if (render_config.layout_cache != nullptr) {
    const auto cached = render_config.layout_cache->tables.find(table_target);
    if (cached != render_config.layout_cache->tables.end()) {
      const auto& entry = cached->second;
      geometry_cache_hit =
          entry.node_identity == reinterpret_cast<std::uintptr_t>(&node) &&
          entry.viewport_width == viewport_width &&
          entry.content_width == content_w &&
          entry.row_count == rows.size() &&
          entry.column_count == column_count &&
          entry.row_heights.size() == rows.size();
      if (geometry_cache_hit) {
        plan.source_indices = entry.source_indices;
        plan.widths = entry.column_widths;
        row_heights = entry.row_heights;
      }
    }
  }
  if (!geometry_cache_hit) {
    const auto declared_widths =
        extract_table_column_widths(document, node, lineage, style, viewport_width, content_w);
    plan = build_table_column_plan(rows, style, content_w, declared_widths);
    row_heights.reserve(rows.size());
    for (const auto& row : rows) {
      row_heights.push_back(
          measure_table_row_height(document, row, plan, style, viewport_width));
    }
    if (render_config.layout_cache != nullptr) {
      render_config.layout_cache->tables[table_target] =
          HtmlTableGeometryCacheEntry{
              .node_identity = reinterpret_cast<std::uintptr_t>(&node),
              .viewport_width = viewport_width,
              .content_width = content_w,
              .row_count = rows.size(),
              .column_count = column_count,
              .source_indices = plan.source_indices,
              .column_widths = plan.widths,
              .row_heights = row_heights,
          };
    }
  }
  int row_total_w = 0;
  for (const int w : plan.widths) row_total_w += w;
  row_total_w = std::max(40, row_total_w);

  std::size_t sticky_prefix_count = 0;
  std::vector<std::size_t> sticky_sources;
  std::vector<int> sticky_widths;
  sticky_sources.reserve(plan.source_indices.size());
  sticky_widths.reserve(plan.widths.size());
  int sticky_total_w = 0;

  for (std::size_t c = 0; c < plan.source_indices.size(); ++c) {
    const std::size_t source_index = plan.source_indices[c];
    bool sticky = false;
    const std::size_t probe_rows = std::min<std::size_t>(rows.size(), 8);
    for (std::size_t r = 0; r < probe_rows; ++r) {
      if (source_index >= rows[r].cells.size()) {
        continue;
      }
      const auto* node_ptr = rows[r].cells[source_index].node;
      if (node_has_class(node_ptr, "sticky-left")) {
        sticky = true;
        break;
      }
      if (node_ptr != nullptr) {
        std::vector<const HtmlNode*> cell_lineage = rows[r].lineage;
        if (rows[r].node != nullptr) {
          cell_lineage.push_back(rows[r].node);
        }
        const ResolvedStyle cell_style = resolve_style(document, *node_ptr, cell_lineage, &style, viewport_width);
        if (cell_style.position == "sticky" && cell_style.sticky_left_set) {
          sticky = true;
          break;
        }
      }
    }
    if (!sticky || c != sticky_prefix_count) {
      break;
    }
    const int width_px = std::max(0, plan.widths[c]);
    sticky_sources.push_back(source_index);
    sticky_widths.push_back(width_px);
    sticky_total_w += width_px;
    ++sticky_prefix_count;
  }
  if (sticky_total_w >= content_w - 24) {
    sticky_prefix_count = 0;
    sticky_sources.clear();
    sticky_widths.clear();
    sticky_total_w = 0;
  }

  const int viewport_h = std::max(24, outer_h - (border + padding) * 2);
  const RectI viewport_clip{content_x, content_y, content_w, viewport_h};
  const Color viewport_surround =
      parent_style != nullptr && parent_style->has_background
          ? parent_style->background_color
          : style.background_color;
  int content_total_h = 0;
  for (const int row_h : row_heights) {
    content_total_h += row_h;
  }
  const bool has_header_row =
      !rows.empty() && std::any_of(rows.front().cells.begin(), rows.front().cells.end(), [](const TableCellData& cell) {
        return cell.is_header;
      });
  const std::size_t row_start = has_header_row ? 1U : 0U;
  std::vector<std::size_t> body_order;
  body_order.reserve(rows.size());
  for (std::size_t r = row_start; r < rows.size(); ++r) {
    body_order.push_back(r);
  }
  const bool filter_target_matches =
      render_config.table_filter_target.empty() || render_config.table_filter_target == table_target;
  const std::string filter_query = to_lower(trim_copy(render_config.table_filter_query));
  if (filter_target_matches && !filter_query.empty()) {
    std::vector<std::size_t> filtered;
    filtered.reserve(body_order.size());
    for (const std::size_t row_index : body_order) {
      if (row_index >= rows.size()) {
        continue;
      }
      std::string haystack;
      for (const auto& cell : rows[row_index].cells) {
        if (!haystack.empty()) {
          haystack.push_back(' ');
        }
        haystack += cell.text;
      }
      const std::string folded = to_lower(trim_copy(haystack));
      if (folded.find(filter_query) != std::string::npos) {
        filtered.push_back(row_index);
      }
    }
    body_order.swap(filtered);
  }
  const bool sort_target_matches =
      render_config.table_sort_target.empty() || render_config.table_sort_target == table_target;
  if (sort_target_matches && render_config.table_sort_column >= 0 &&
      static_cast<std::size_t>(render_config.table_sort_column) < plan.source_indices.size()) {
    const std::size_t source_col = plan.source_indices[static_cast<std::size_t>(render_config.table_sort_column)];
    std::stable_sort(body_order.begin(), body_order.end(), [&](const std::size_t lhs_r, const std::size_t rhs_r) {
      const std::string lhs = source_col < rows[lhs_r].cells.size() ? rows[lhs_r].cells[source_col].text : std::string{};
      const std::string rhs = source_col < rows[rhs_r].cells.size() ? rows[rhs_r].cells[source_col].text : std::string{};
      const auto lhs_num = parse_numeric_text(lhs);
      const auto rhs_num = parse_numeric_text(rhs);
      if (lhs_num.has_value() && rhs_num.has_value()) {
        if (*lhs_num == *rhs_num) {
          return lhs_r < rhs_r;
        }
        return render_config.table_sort_ascending ? (*lhs_num < *rhs_num) : (*lhs_num > *rhs_num);
      }
      const std::string lhs_norm = to_lower(trim_copy(lhs));
      const std::string rhs_norm = to_lower(trim_copy(rhs));
      if (lhs_norm == rhs_norm) {
        return lhs_r < rhs_r;
      }
      return render_config.table_sort_ascending ? (lhs_norm < rhs_norm) : (lhs_norm > rhs_norm);
    });
  }
  const int header_h = (has_header_row && !row_heights.empty()) ? row_heights.front() : 0;
  int body_total_h = 0;
  for (const std::size_t r : body_order) {
    if (r < row_heights.size()) {
      body_total_h += row_heights[r];
    }
  }
  const int body_viewport_h = std::max(0, viewport_h - header_h);
  const int scrollable_view_w = std::max(16, content_w - sticky_total_w);
  const int scrollable_content_w = std::max(0, row_total_w - sticky_total_w);
  const int max_scroll_x = std::max(0, scrollable_content_w - scrollable_view_w);
  const int max_scroll_y = std::max(0, body_total_h - body_viewport_h);
  int requested_scroll_x = render_config.table_scroll_x_px;
  int requested_scroll_y = render_config.table_scroll_y_px;
  if (render_config.table_scroll_offsets != nullptr) {
    const auto it = render_config.table_scroll_offsets->find(table_target);
    if (it != render_config.table_scroll_offsets->end()) {
      requested_scroll_x = it->second.x;
      requested_scroll_y = it->second.y;
    }
  }
  const int scroll_x = std::clamp(requested_scroll_x, 0, max_scroll_x);
  const int scroll_y = std::clamp(requested_scroll_y, 0, max_scroll_y);
  const int table_origin_x = content_x - scroll_x;
  const int scrollable_left = content_x + sticky_total_w;
  const int scrollable_right = content_x + content_w;
  RectI interactive_viewport = viewport_clip;
  if (render_config.render_stats != nullptr) {
    auto& stats = *render_config.render_stats;
    interactive_viewport = intersect_rect(viewport_clip, stats.document_viewport_rect);
    stats.has_table = true;
    const int current_area = stats.table_viewport_rect.width * stats.table_viewport_rect.height;
    const int candidate_area = interactive_viewport.width * interactive_viewport.height;
    if (candidate_area >= current_area) {
      stats.table_viewport_rect = interactive_viewport;
      stats.table_max_scroll_x = max_scroll_x;
      stats.table_max_scroll_y = max_scroll_y;
      stats.table_body_row_height_px = 0;
    }
    if (!body_order.empty()) {
      const std::size_t sample_count = std::min<std::size_t>(body_order.size(), 12U);
      int sample_sum = 0;
      for (std::size_t i = 0; i < sample_count; ++i) {
        sample_sum += row_heights[body_order[i]];
      }
      if (sample_sum > 0 && sample_count > 0) {
        if (candidate_area >= current_area) {
          stats.table_body_row_height_px = sample_sum / static_cast<int>(sample_count);
        }
      }
    }
  }

  std::vector<std::size_t> visible_scroll_columns;
  visible_scroll_columns.reserve(plan.source_indices.size());
  std::vector<int> column_x_start;
  column_x_start.reserve(plan.source_indices.size());
  {
    int probe_x = table_origin_x;
    for (std::size_t c = 0; c < plan.source_indices.size(); ++c) {
      const int cell_w = plan.widths[c];
      column_x_start.push_back(probe_x);
      if (c < sticky_prefix_count) {
        probe_x += cell_w;
        continue;
      }
      const int x0 = probe_x;
      const int x1 = probe_x + cell_w;
      if (x1 > scrollable_left && x0 < scrollable_right) {
        visible_scroll_columns.push_back(c);
      }
      probe_x += cell_w;
    }
  }

  auto render_row = [&](const std::size_t r, const int row_y) {
    const auto& row = rows[r];
    const bool header_row = std::any_of(row.cells.begin(), row.cells.end(), [](const TableCellData& cell) {
      return cell.is_header;
    });

    const int row_h = row_heights[r];

    const Color fill = header_row ? Color::rgba(235, 240, 246, 255)
                                  : ((r % 2 == 0) ? Color::rgba(255, 255, 255, 255)
                                                  : Color::rgba(248, 250, 252, 255));
    if (header_row && row_y == content_y) {
      const int header_radius = std::max(0, std::min(style.border_radius, row_h / 2));
      draw_rect_fill(scene, RectI{content_x, row_y, content_w, row_h}, fill, header_radius);
      if (header_radius > 0 && row_h > header_radius) {
        scene.add(FillRectCommand{RectI{content_x, row_y + header_radius, content_w, row_h - header_radius}, fill});
      }
    } else {
      scene.add(FillRectCommand{RectI{table_origin_x, row_y, row_total_w, row_h}, fill});
    }
    scene.add(LineCommand{PointI{table_origin_x, row_y},
                          PointI{table_origin_x + row_total_w, row_y},
                          1,
                          Color::rgba(216, 222, 228, 255)});

    for (const std::size_t c : visible_scroll_columns) {
      const std::size_t source_index = plan.source_indices[c];
      const int cell_w = plan.widths[c];
      const int cell_x = column_x_start[c];
      scene.add(LineCommand{PointI{cell_x, row_y},
                            PointI{cell_x, row_y + row_h},
                            1,
                            Color::rgba(216, 222, 228, 255)});

      const std::string value = (source_index < row.cells.size()) ? row.cells[source_index].text : std::string{};

      ResolvedStyle cell_style = style;
      if (source_index < row.cells.size() && row.cells[source_index].node != nullptr) {
        std::vector<const HtmlNode*> cell_lineage = row.lineage;
        if (row.node != nullptr) {
          cell_lineage.push_back(row.node);
        }
        cell_style = resolve_style(document, *row.cells[source_index].node, cell_lineage, &style, viewport_width);
      }

      const int cell_padding = std::max(2, cell_style.padding);
      const int text_max_w = std::max(20, cell_w - ((cell_padding + cell_style.border) * 2) - 2);
      const int max_lines = header_row ? 2 : 5;
      const auto wrapped = limit_lines(layout_text_lines(value, cell_style, text_max_w),
                                       max_lines,
                                       cell_style.font_size,
                                       text_max_w);
      if (cell_style.has_background) {
        draw_style_fill(scene, RectI{cell_x + 1, row_y + 1, std::max(1, cell_w - 2), std::max(1, row_h - 2)}, cell_style);
      }
      const int text_block_h =
          std::max(1, static_cast<int>(wrapped.size())) * cell_style.line_height;
      const int text_y = header_row
                             ? row_y + cell_padding
                             : row_y + std::max(cell_padding, (row_h - text_block_h) / 2);
      draw_text_lines(scene,
                      wrapped,
                      cell_style.font_size,
                      cell_style.font_weight,
                      cell_style.font_family,
                      cell_style.text_color,
                      cell_x + cell_padding,
                      text_y,
                      cell_style.line_height,
                      cell_style.text_align,
                      text_max_w);
      if (header_row && render_config.hit_regions != nullptr) {
        const int x0 = std::max(cell_x, scrollable_left);
        const int x1 = std::min(cell_x + cell_w, scrollable_right);
        if (x1 > x0) {
          render_config.hit_regions->push_back(HtmlHitRegion{
              .rect = RectI{x0, row_y, x1 - x0, row_h},
              .action = "table.sort.column",
              .target = table_target,
              .value = static_cast<int>(c),
          });
        }
      }
    }
    scene.add(LineCommand{PointI{table_origin_x + row_total_w, row_y},
                          PointI{table_origin_x + row_total_w, row_y + row_h},
                          1,
                          Color::rgba(216, 222, 228, 255)});

    if (sticky_prefix_count > 0) {
      int sticky_x = content_x;
      for (std::size_t s = 0; s < sticky_prefix_count; ++s) {
        const std::size_t source_index = sticky_sources[s];
        const int sticky_w = sticky_widths[s];
        scene.add(FillRectCommand{RectI{sticky_x, row_y, sticky_w, row_h}, fill});
        scene.add(LineCommand{PointI{sticky_x, row_y},
                              PointI{sticky_x, row_y + row_h},
                              1,
                              Color::rgba(216, 222, 228, 255)});

        const std::string value = (source_index < row.cells.size()) ? row.cells[source_index].text : std::string{};
        ResolvedStyle cell_style = style;
        if (source_index < row.cells.size() && row.cells[source_index].node != nullptr) {
          std::vector<const HtmlNode*> cell_lineage = row.lineage;
          if (row.node != nullptr) {
            cell_lineage.push_back(row.node);
          }
          cell_style = resolve_style(document, *row.cells[source_index].node, cell_lineage, &style, viewport_width);
        }
        const int cell_padding = std::max(2, cell_style.padding);
        const int text_max_w = std::max(20, sticky_w - ((cell_padding + cell_style.border) * 2) - 2);
        const int max_lines = header_row ? 2 : 5;
        const auto wrapped = limit_lines(layout_text_lines(value, cell_style, text_max_w),
                                         max_lines,
                                         cell_style.font_size,
                                         text_max_w);
        if (cell_style.has_background) {
          draw_style_fill(
              scene,
              RectI{sticky_x + 1, row_y + 1, std::max(1, sticky_w - 2), std::max(1, row_h - 2)},
              cell_style);
        }
        const int text_block_h =
            std::max(1, static_cast<int>(wrapped.size())) * cell_style.line_height;
        const int text_y = header_row
                               ? row_y + cell_padding
                               : row_y + std::max(cell_padding, (row_h - text_block_h) / 2);
        draw_text_lines(scene,
                        wrapped,
                        cell_style.font_size,
                        cell_style.font_weight,
                        cell_style.font_family,
                        cell_style.text_color,
                        sticky_x + cell_padding,
                        text_y,
                        cell_style.line_height,
                        cell_style.text_align,
                        text_max_w);
        scene.add(LineCommand{PointI{sticky_x, row_y},
                              PointI{sticky_x + sticky_w, row_y},
                              1,
                              Color::rgba(216, 222, 228, 255)});
        if (header_row && render_config.hit_regions != nullptr) {
          render_config.hit_regions->push_back(HtmlHitRegion{
              .rect = RectI{sticky_x, row_y, sticky_w, row_h},
              .action = "table.sort.column",
              .target = table_target,
              .value = static_cast<int>(s),
          });
        }
        sticky_x += sticky_w;
      }
      scene.add(LineCommand{PointI{content_x + sticky_total_w, row_y},
                            PointI{content_x + sticky_total_w, row_y + row_h},
                            2,
                            Color::rgba(79, 137, 148, 255)});
    }
  };

  if (has_header_row && !rows.empty()) {
    if (render_config.clip_table_viewport) {
      scene.add(SetClipRectCommand{RectI{content_x, content_y, content_w, header_h}});
    }
    render_row(0, content_y);
    mask_rounded_viewport_corners(
        scene, viewport_clip, style.border_radius, viewport_surround, true, false);
    if (render_config.clip_table_viewport) {
      scene.add(ClearClipRectCommand{});
    }
  }

  const int body_clip_y = content_y + header_h;
  const int body_clip_h = std::max(0, viewport_h - header_h);
  if (body_clip_h > 0) {
    if (render_config.clip_table_viewport) {
      scene.add(SetClipRectCommand{RectI{content_x, body_clip_y, content_w, body_clip_h}});
    }

    if (body_order.empty()) {
      const int line_h = resolve_line_height_value(style.line_height, style.font_size);
      draw_text_lines(scene,
                      {"No matching rows"},
                      style.font_size,
                      style.font_weight,
                      style.font_family,
                      Color::rgba(99, 111, 124, 255),
                      content_x + 12,
                      body_clip_y + std::max(6, (body_clip_h - line_h) / 2),
                      line_h,
                      "left",
                      std::max(20, content_w - 24));
    } else {
      std::size_t first_visible = 0;
      int consumed_h = 0;
      while (first_visible < body_order.size() &&
             consumed_h + row_heights[body_order[first_visible]] <= scroll_y) {
        consumed_h += row_heights[body_order[first_visible]];
        ++first_visible;
      }

      int cursor_y = body_clip_y - (scroll_y - consumed_h);
      const int body_clip_bottom = body_clip_y + body_clip_h;
      for (std::size_t idx = first_visible; idx < body_order.size(); ++idx) {
        const std::size_t r = body_order[idx];
        const int row_h = row_heights[r];
        // Stop before a row that would extend past the viewport, rather
        // than drawing it and trusting the clip rect to cut it off cleanly
        // -- text glyphs aren't clipped as tightly as fills/lines, so a
        // partial last row was bleeding its full text underneath (and
        // through) the scrollbar instead of being visually cut short.
        if (cursor_y + row_h > body_clip_bottom) {
          break;
        }
        if (cursor_y + row_h > body_clip_y) {
          render_row(r, cursor_y);
        }
        cursor_y += row_h;
      }
    }
    mask_rounded_viewport_corners(
        scene, viewport_clip, style.border_radius, viewport_surround, false, true);
    if (render_config.clip_table_viewport) {
      scene.add(ClearClipRectCommand{});
    }
  }

  const int table_end_y = content_y + header_h + body_total_h - scroll_y;
  if (table_end_y >= content_y && table_end_y <= content_y + viewport_h) {
    scene.add(LineCommand{PointI{table_origin_x, table_end_y},
                          PointI{table_origin_x + row_total_w, table_end_y},
                          1,
                          Color::rgba(216, 222, 228, 255)});
  }
  RectI horizontal_track{};
  RectI horizontal_thumb{};
  RectI vertical_track{};
  RectI vertical_thumb{};
  if (max_scroll_x > 0) {
    const int track_h = 10;
    // See render_virtual_table's identical fix: anchor to the content's
    // real bottom edge so the track can't land on top of the last row.
    const int below_content = content_y + viewport_h + 2;
    const int near_outer_edge = outer_y + outer_h - std::max(14, padding / 2 + track_h);
    const int track_y = std::max(below_content, near_outer_edge);
    const int track_w = std::max(36, scrollable_view_w - 6);
    const int track_x = content_x + sticky_total_w + 3;
    horizontal_track = RectI{track_x, track_y, track_w, track_h};
    if (render_config.draw_scrollbars) {
      draw_rect_fill(scene, horizontal_track, Color::rgba(218, 224, 231, 238), 5);
    }

    const double ratio = static_cast<double>(scrollable_view_w) / static_cast<double>(std::max(1, scrollable_content_w));
    const int thumb_w =
        std::min(track_w, std::max(32, static_cast<int>(std::lround(static_cast<double>(track_w) * ratio))));
    const int thumb_x = track_x + static_cast<int>(std::lround(
                                   (static_cast<double>(std::max(0, track_w - thumb_w)) *
                                    static_cast<double>(scroll_x)) /
                                   static_cast<double>(max_scroll_x)));
    horizontal_thumb = RectI{thumb_x, track_y, thumb_w, track_h};
    if (render_config.draw_scrollbars) {
      draw_rect_fill(scene, horizontal_thumb, Color::rgba(94, 111, 130, 255), 5);
    }
  }
  if (max_scroll_y > 0) {
    const int track_w = 10;
    // See render_virtual_table's identical fix: anchor to the content's
    // real right edge so the track can't land on top of the last column.
    const int right_of_content = content_x + content_w + 2;
    const int near_outer_edge = outer_x + outer_w - std::max(14, padding / 2 + track_w);
    const int track_x = std::max(right_of_content, near_outer_edge);
    const int track_h = std::max(36, body_viewport_h - 6);
    const int track_y = content_y + header_h + 3;
    vertical_track = RectI{track_x, track_y, track_w, track_h};
    if (render_config.draw_scrollbars) {
      draw_rect_fill(scene, vertical_track, Color::rgba(218, 224, 231, 238), 5);
    }

    const double ratio = static_cast<double>(body_viewport_h) / static_cast<double>(body_total_h);
    const int thumb_h =
        std::min(track_h, std::max(32, static_cast<int>(std::lround(static_cast<double>(track_h) * ratio))));
    const int thumb_y = track_y + static_cast<int>(std::lround(
                                   (static_cast<double>(std::max(0, track_h - thumb_h)) *
                                    static_cast<double>(scroll_y)) /
                                   static_cast<double>(max_scroll_y)));
    vertical_thumb = RectI{track_x, thumb_y, track_w, thumb_h};
    if (render_config.draw_scrollbars) {
      draw_rect_fill(scene, vertical_thumb, Color::rgba(94, 111, 130, 255), 5);
    }
  }
  if (render_config.render_stats != nullptr) {
    render_config.render_stats->table_scroll_regions.push_back(HtmlScrollRegion{
        .target = table_target,
        .viewport_rect = interactive_viewport,
        .max_scroll_x = max_scroll_x,
        .max_scroll_y = max_scroll_y,
        .horizontal_track_rect = horizontal_track,
        .horizontal_thumb_rect = horizontal_thumb,
        .vertical_track_rect = vertical_track,
        .vertical_thumb_rect = vertical_thumb,
    });
  }
  return total_h;
}

int render_node(Scene& scene,
                const HtmlDocument& document,
                const HtmlNode& node,
                int x,
                int y,
                int width,
                int bottom,
                std::vector<const HtmlNode*>& lineage,
                const HtmlRenderConfig& render_config,
                const ResolvedStyle* parent_style,
                const int viewport_width) {
  const ResolvedStyle style = resolve_style(document, node, lineage, parent_style, viewport_width);
  if (style.display == "none") {
    return 0;
  }

  const int total_h = measure_node_height(document, node, width, lineage, parent_style, viewport_width);
  if (total_h <= 0 || y >= bottom) {
    return 0;
  }

  if (node.tag == "table") {
    return render_table(scene, document, node, x, y, width, bottom, lineage, render_config, parent_style, viewport_width);
  }

  const int margin = style.margin;
  const int padding = style.padding;
  const int border = style.border;
  const int explicit_width = resolve_length_px(style.explicit_width, style.explicit_width_ratio, width);
  const int min_width = resolve_length_px(style.min_width, style.min_width_ratio, width);
  const int resolved_outer_w = std::max(min_width, explicit_width > 0 ? explicit_width : width);
  const int outer_x = x + margin;
  const int outer_y = y + margin;
  const int outer_w = std::max(20, resolved_outer_w - margin * 2);
  const int outer_h = total_h - margin * 2;
  const RectI outer_rect{outer_x, outer_y, outer_w, outer_h};
  const int content_x = outer_x + border + padding;
  const int content_y = outer_y + border + padding;
  const int content_w = std::max(8, outer_w - (border + padding) * 2);

  if (node.kind != HtmlNodeKind::Text) {
    draw_box_shadow(scene, outer_rect, style);
    draw_style_fill(scene, outer_rect, style);
    draw_rect_border(scene, outer_rect, border, style.border_color, style.border_radius);
  }

  if (node.kind == HtmlNodeKind::Text) {
    const auto lines = layout_text_lines(node.text, style, std::max(8, content_w));
    draw_text_lines(scene,
                    lines,
                    style.font_size,
                    style.font_weight,
                    style.font_family,
                    style.text_color,
                    content_x,
                    content_y,
                    style.line_height,
                    style.text_align,
                    content_w);
    return total_h;
  }

  if (node.tag == "button" && render_config.hit_regions != nullptr) {
    std::string target = node_interaction_target(node, trim_copy(collect_node_text(node)));
    int value = 0;
    if (const std::string* frame_index = attribute_value(&node, "data-frame-index");
        frame_index != nullptr) {
      value = parse_css_int(*frame_index);
    }
    render_config.hit_regions->push_back(HtmlHitRegion{
        .rect = outer_rect,
        .action = "button.click",
        .target = std::move(target),
        .value = value,
    });
  }

  if (node.kind == HtmlNodeKind::Widget || node.tag == "canvas" || node.tag == "svg" || node.tag == "line-plot" ||
      node.tag == "chart") {
    const RectI panel_rect{content_x, content_y, content_w, std::max(24, outer_h - (padding + border) * 2)};
    if (style.has_background) {
      draw_style_fill(scene, panel_rect, style);
    } else {
      scene.add(FillRectCommand{panel_rect, Color::rgba(255, 255, 255, 255)});
    }
    if (node.tag == "svg") {
      const SvgPaint root_paint = resolve_svg_paint(node, SvgPaint{});
      const SvgViewport viewport =
          resolve_svg_viewport(node, panel_rect, render_config.widget_scale);
      scene.add(SetClipRectCommand{panel_rect});
      render_svg_children(scene, node, viewport, root_paint);
      scene.add(ClearClipRectCommand{});
      return total_h;
    }
    const std::string label = node.tag.empty() ? std::string("widget") : node.tag;
    const std::string descriptor = "in-house widget: " + label;
    draw_text_lines(scene,
                    {descriptor},
                    std::max(12, style.font_size),
                    style.font_weight,
                    style.font_family,
                    style.text_color,
                    content_x + 10,
                    content_y + 10,
                    style.line_height,
                    "left",
                    std::max(40, content_w - 20));
    return total_h;
  }

  if (node.tag == "input") {
    const std::string input_target = node_interaction_target(node, "input");
    const bool focused = !render_config.active_input_target.empty() && render_config.active_input_target == input_target;
    if (render_config.hit_regions != nullptr) {
      render_config.hit_regions->push_back(HtmlHitRegion{
          .rect = outer_rect,
          .action = "input.focus",
          .target = input_target,
          .value = 0,
      });
    }
    const HtmlAttribute* value_attr = find_attribute(node, "value");
    const HtmlAttribute* placeholder_attr = find_attribute(node, "placeholder");
    const bool use_override = focused;
    std::string text{};
    if (use_override && !render_config.active_input_value.empty()) {
      text = render_config.active_input_value;
    } else if (value_attr != nullptr && !value_attr->value.empty()) {
      text = value_attr->value;
    } else if (placeholder_attr != nullptr && !placeholder_attr->value.empty()) {
      text = placeholder_attr->value;
    }
    if (text.empty()) {
      text = "Input";
    }
    const bool has_value = (use_override && !render_config.active_input_value.empty()) ||
                           (!use_override && value_attr != nullptr && !value_attr->value.empty());
    const Color text_color = has_value ? style.text_color : Color::rgba(132, 141, 151, 255);
    const std::string transformed = apply_text_transform(text, style.text_transform);
    const std::string clipped = truncate_to_width(transformed, style.font_size, std::max(20, content_w - 12));
    const TextLayout text_layout = layout_math_text(clipped, style.font_size);
    const int glyph_h = std::max(1, text_layout.ascent + text_layout.descent);
    const int text_top = outer_y + std::max(0, (outer_h - glyph_h) / 2);
    const int text_area_x = content_x + 6;
    const int text_area_w = std::max(20, content_w - 12);
    int text_x = text_area_x;
    if (style.text_align == "center") {
      text_x += std::max(0, (text_area_w - text_layout.width) / 2);
    } else if (style.text_align == "right") {
      text_x += std::max(0, text_area_w - text_layout.width);
    }
    draw_text_lines(scene,
                    {clipped},
                    style.font_size,
                    style.font_weight,
                    style.font_family,
                    text_color,
                    text_area_x,
                    text_top,
                    glyph_h,
                    style.text_align,
                    text_area_w);
    if (focused) {
      const int caret_h = std::max(12, glyph_h + 4);
      const int caret_y0 = outer_y + std::max(1, (outer_h - caret_h) / 2);
      const int caret_y1 = caret_y0 + caret_h;
      if (render_config.show_input_caret) {
        scene.add(TextCaretCommand{
            .text_anchor = PointI{has_value ? text_x : text_area_x,
                                  text_top + text_layout.ascent},
            .text = has_value ? clipped : std::string{},
            .pixel_size = style.font_size,
            .font_weight = style.font_weight,
            .font_family = style.font_family,
            .fallback_width = has_value ? text_layout.width : 0,
            .max_x = content_x + std::max(8, content_w - 8),
            .y0 = caret_y0,
            .y1 = caret_y1,
            .thickness = 1,
            .color = Color::rgba(71, 118, 184, 255),
        });
      }
      scene.add(StrokeRoundedRectCommand{
          outer_rect,
          std::max(0, style.border_radius),
          1,
          Color::rgba(104, 145, 208, 255),
      });
    }
    return total_h;
  }

  if (node.tag == "button") {
    const std::string text = collect_node_text(node).empty() ? std::string("Button") : collect_node_text(node);
    const std::string transformed = apply_text_transform(text, style.text_transform);
    const std::string clipped = truncate_to_width(transformed, style.font_size, std::max(20, content_w - 12));
    std::string align = style.text_align.empty() ? std::string("left") : style.text_align;
    if (align == "left") {
      align = "center";
    }
    const TextLayout text_layout = layout_math_text(clipped, style.font_size);
    const int glyph_h = std::max(1, text_layout.ascent + text_layout.descent);
    const int text_top = outer_y + std::max(0, (outer_h - glyph_h) / 2);
    draw_text_lines(scene,
                    {clipped},
                    style.font_size,
                    style.font_weight,
                    style.font_family,
                    style.text_color,
                    content_x + 6,
                    text_top,
                    glyph_h,
                    align,
                    std::max(20, content_w - 12));
    return total_h;
  }

  if (node.tag == "hr") {
    const int line_y = outer_y + outer_h / 2;
    scene.add(LineCommand{PointI{outer_x, line_y}, PointI{outer_x + outer_w, line_y}, 1, style.border_color});
    return total_h;
  }

  lineage.push_back(&node);
  if (style.display == "grid") {
    const int gap = std::max(0, style.gap);
    const int min_col = std::max(120, style.grid_min_column_width > 0 ? style.grid_min_column_width : 180);
    const int cols = std::max(1, (content_w + gap) / std::max(1, min_col + gap));
    const int cell_w = std::max(70, (content_w - gap * (cols - 1)) / cols);

    std::vector<int> row_heights;
    row_heights.reserve((node.children.size() / static_cast<std::size_t>(cols)) + 1);
    for (std::size_t i = 0; i < node.children.size(); ++i) {
      const int child_h = measure_node_height(document, node.children[i], cell_w, lineage, &style, viewport_width);
      const std::size_t row = i / static_cast<std::size_t>(cols);
      if (row >= row_heights.size()) {
        row_heights.push_back(child_h);
      } else {
        row_heights[row] = std::max(row_heights[row], child_h);
      }
    }

    std::vector<int> row_top(row_heights.size(), content_y);
    for (std::size_t row = 1; row < row_heights.size(); ++row) {
      row_top[row] = row_top[row - 1] + row_heights[row - 1] + gap;
    }

    for (std::size_t i = 0; i < node.children.size(); ++i) {
      const int col = static_cast<int>(i % static_cast<std::size_t>(cols));
      const std::size_t row = i / static_cast<std::size_t>(cols);
      const int cell_x = content_x + col * (cell_w + gap);
      const int cell_y = row_top[row];
      (void)render_node(scene,
                        document,
                        node.children[i],
                        cell_x,
                        cell_y,
                        cell_w,
                        bottom,
                        lineage,
                        render_config,
                        &style,
                        viewport_width);
    }
  } else if (style.display == "flex") {
    const int gap = std::max(0, style.gap);
    const bool wrap = style.flex_wrap != "nowrap";
    std::vector<const HtmlNode*> flex_children;
    flex_children.reserve(node.children.size());
    for (const auto& child : node.children) {
      if (child.kind != HtmlNodeKind::Text || !trim_copy(child.text).empty()) {
        flex_children.push_back(&child);
      }
    }
    const int fallback_w =
        std::max(96, content_w / std::max(1, static_cast<int>(flex_children.size())));

    std::vector<int> child_widths;
    std::vector<bool> child_auto_left;
    child_widths.reserve(flex_children.size());
    child_auto_left.reserve(flex_children.size());
    int total_child_width = 0;
    bool has_auto_left = false;
    for (const HtmlNode* child_ptr : flex_children) {
      const auto& child = *child_ptr;
      const ResolvedStyle child_style =
          resolve_style(document, child, lineage, &style, viewport_width);
      int child_w =
          resolve_length_px(child_style.explicit_width, child_style.explicit_width_ratio, content_w);
      if (child_w <= 0) {
        child_w =
            resolve_length_px(child_style.min_width, child_style.min_width_ratio, content_w);
      }
      if (child_w <= 0) {
        child_w = estimate_node_preferred_width(
            document, child, lineage, &style, content_w, viewport_width);
      }
      if (child_w <= 0) child_w = fallback_w;
      child_w = std::max(80, child_w);
      if (wrap) {
        child_w = std::min(child_w, std::max(80, content_w));
      }
      child_widths.push_back(child_w);
      child_auto_left.push_back(child_style.margin_left_auto);
      has_auto_left = has_auto_left || child_style.margin_left_auto;
      total_child_width += child_w;
    }

    int resolved_gap = gap;
    int leading_space = 0;
    const int base_gap_total =
        gap * std::max(0, static_cast<int>(flex_children.size()) - 1);
    int free_space = std::max(0, content_w - total_child_width - base_gap_total);
    if (!wrap && !has_auto_left) {
      if (style.justify_content == "flex-end" || style.justify_content == "end") {
        leading_space = free_space;
      } else if (style.justify_content == "center") {
        leading_space = free_space / 2;
      } else if (style.justify_content == "space-between" && flex_children.size() > 1) {
        resolved_gap =
            gap + free_space / (static_cast<int>(flex_children.size()) - 1);
        free_space = 0;
      }
    }

    int row_x = 0;
    int row_y = content_y;
    int row_h = 0;
    for (std::size_t index = 0; index < flex_children.size(); ++index) {
      const auto& child = *flex_children[index];
      const int child_w = child_widths[index];

      if (wrap && row_x > 0 && row_x + child_w > content_w) {
        row_y += row_h + resolved_gap;
        row_x = 0;
        row_h = 0;
      }
      if (wrap && child_auto_left[index]) {
        row_x = std::max(row_x, content_w - child_w);
      }
      if (!wrap && index == 0) {
        row_x += leading_space;
      }
      if (!wrap && child_auto_left[index]) {
        row_x += free_space;
        free_space = 0;
      }

      const int child_h = measure_node_height(document, child, child_w, lineage, &style, viewport_width);
      (void)render_node(scene,
                        document,
                        child,
                        content_x + row_x,
                        row_y,
                        child_w,
                        bottom,
                        lineage,
                        render_config,
                        &style,
                        viewport_width);
      row_h = std::max(row_h, child_h);
      row_x += child_w + resolved_gap;
      if (row_y + row_h >= bottom) {
        break;
      }
    }
  } else {
    int cursor_y = content_y;
    for (const auto& child : node.children) {
      const int child_h = render_node(
          scene, document, child, content_x, cursor_y, content_w, bottom, lineage, render_config, &style, viewport_width);
      cursor_y += child_h;
      if (cursor_y >= bottom) {
        break;
      }
    }
  }
  lineage.pop_back();
  return total_h;
}

} // namespace

Scene build_html_document_scene(const HtmlDocument& document, const SizeI viewport, const HtmlRenderConfig config) {
  if (config.render_stats != nullptr) {
    reset_render_stats(*config.render_stats);
  }
  const std::size_t first_hit_region =
      config.hit_regions != nullptr ? config.hit_regions->size() : 0U;

  const int width = std::max(320, viewport.width);
  const int height = std::max(240, viewport.height);

  Scene scene;
  scene.add(ClearCommand{Color::rgba(245, 248, 252, 255)});

  int body_top = config.root_padding_px;
  if (config.draw_top_bar) {
    const int top_h = std::max(36, config.top_bar_height_px);
    scene.add(FillRectCommand{RectI{0, 0, width, top_h}, Color::rgba(28, 35, 44, 255)});
    scene.add(TextCommand{
        PointI{18, 34},
        document.title.empty() ? std::string("PlussTilsView In-House HTML") : document.title,
        16,
        Color::rgba(241, 245, 250, 255),
    });
    body_top = top_h + config.root_padding_px;
  }

  const int body_left = std::max(10, config.root_padding_px);
  const int body_width = std::max(80, width - (body_left * 2));
  const int body_bottom = height - std::max(8, config.root_padding_px);
  const int body_height = std::max(24, body_bottom - body_top);

  int content_height = 0;
  int content_pref_w = body_width;
  std::vector<const HtmlNode*> measure_lineage;
  measure_lineage.reserve(64);
  for (const auto& child : document.root.children) {
    const int child_h = measure_node_height(document, child, body_width, measure_lineage, nullptr, width);
    content_height += std::max(0, child_h);
    const int child_w =
        estimate_node_preferred_width(document, child, measure_lineage, nullptr, body_width, width);
    content_pref_w = std::max(content_pref_w, child_w);
  }

  const int max_scroll_x =
      config.enable_document_scroll ? std::max(0, content_pref_w - body_width) : 0;
  const int max_scroll_y =
      config.enable_document_scroll ? std::max(0, content_height - body_height) : 0;
  const int doc_scroll_x = std::clamp(config.document_scroll_x_px, 0, max_scroll_x);
  const int doc_scroll_y = std::clamp(config.document_scroll_y_px, 0, max_scroll_y);
  const RectI body_clip{body_left, body_top, body_width, body_height};
  if (config.render_stats != nullptr) {
    auto& stats = *config.render_stats;
    stats.document_viewport_rect = body_clip;
    stats.document_max_scroll_x = max_scroll_x;
    stats.document_max_scroll_y = max_scroll_y;
  }

  if (config.clip_document_viewport) {
    scene.add(SetClipRectCommand{body_clip});
  }

  int cursor_y = body_top - doc_scroll_y;
  const int render_left = body_left - doc_scroll_x;
  bool rendered_any = false;
  std::vector<const HtmlNode*> lineage;
  lineage.reserve(64);
  for (const auto& child : document.root.children) {
    const int child_h = render_node(
        scene, document, child, render_left, cursor_y, body_width, body_bottom, lineage, config, nullptr, width);
    if (child_h > 0) {
      rendered_any = true;
      cursor_y += child_h;
    }
    if (cursor_y >= body_bottom) {
      break;
    }
  }

  if (config.clip_document_viewport) {
    scene.add(ClearClipRectCommand{});
  }

  if (config.hit_regions != nullptr) {
    auto& regions = *config.hit_regions;
    std::size_t index = std::min(first_hit_region, regions.size());
    while (index < regions.size()) {
      regions[index].rect = intersect_rect(regions[index].rect, body_clip);
      if (regions[index].rect.empty()) {
        regions.erase(regions.begin() + static_cast<std::ptrdiff_t>(index));
      } else {
        ++index;
      }
    }
  }

  if (!rendered_any) {
    scene.add(TextCommand{
        PointI{body_left, body_top + 20},
        "No renderable HTML nodes found in payload.",
        13,
        Color::rgba(79, 88, 99, 255),
    });
  }

  if (max_scroll_x > 0) {
    const int track_h = 10;
    const int track_y = body_bottom - 12;
    const int track_x = body_left + 3;
    const int track_w = std::max(40, body_width - (max_scroll_y > 0 ? 14 : 8));
    const RectI track{track_x, track_y, track_w, track_h};
    if (config.draw_scrollbars) {
      draw_rect_fill(scene, track, Color::rgba(218, 224, 231, 238), 5);
    }

    const double ratio = static_cast<double>(body_width) / static_cast<double>(std::max(1, content_pref_w));
    const int thumb_w =
        std::min(track_w, std::max(32, static_cast<int>(std::lround(static_cast<double>(track_w) * ratio))));
    const int thumb_x = track_x + static_cast<int>(std::lround(
                                   (static_cast<double>(std::max(0, track_w - thumb_w)) *
                                    static_cast<double>(doc_scroll_x)) /
                                   static_cast<double>(max_scroll_x)));
    const RectI thumb{thumb_x, track_y, thumb_w, track_h};
    if (config.draw_scrollbars) {
      draw_rect_fill(scene, thumb, Color::rgba(94, 111, 130, 255), 5);
    }
    if (config.render_stats != nullptr) {
      config.render_stats->document_horizontal_track_rect = track;
      config.render_stats->document_horizontal_thumb_rect = thumb;
    }
  }
  if (max_scroll_y > 0) {
    const int track_w = 10;
    const int track_x = width - std::max(14, config.root_padding_px / 2 + track_w);
    const int track_y = body_top + 3;
    const int track_h = std::max(36, body_height - (max_scroll_x > 0 ? 14 : 6));
    const RectI track{track_x, track_y, track_w, track_h};
    if (config.draw_scrollbars) {
      draw_rect_fill(scene, track, Color::rgba(218, 224, 231, 238), 5);
    }

    const double ratio = static_cast<double>(body_height) / static_cast<double>(std::max(1, content_height));
    const int thumb_h =
        std::min(track_h, std::max(32, static_cast<int>(std::lround(static_cast<double>(track_h) * ratio))));
    const int thumb_y = track_y + static_cast<int>(std::lround(
                                   (static_cast<double>(std::max(0, track_h - thumb_h)) *
                                    static_cast<double>(doc_scroll_y)) /
                                   static_cast<double>(max_scroll_y)));
    const RectI thumb{track_x, thumb_y, track_w, thumb_h};
    if (config.draw_scrollbars) {
      draw_rect_fill(scene, thumb, Color::rgba(94, 111, 130, 255), 5);
    }
    if (config.render_stats != nullptr) {
      config.render_stats->document_vertical_track_rect = track;
      config.render_stats->document_vertical_thumb_rect = thumb;
    }
  }

  return scene;
}

} // namespace pluss::view::window
