#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pluss::view::window {

enum class HtmlNodeKind : std::uint8_t {
  Document = 0,
  Element = 1,
  Text = 2,
  Widget = 3,
};

enum class HtmlWidgetKind : std::uint8_t {
  None = 0,
  LinePlot = 1,
  Table = 2,
  Canvas = 3,
  Svg = 4,
  WebGl = 5,
};

struct HtmlAttribute {
  std::string name{};
  std::string value{};
};

struct HtmlStyle {
  std::int32_t font_size_px{0};
  std::int32_t margin_px{0};
  std::int32_t padding_px{0};
  std::int32_t border_px{0};
  std::int32_t border_radius_px{0};
  std::int32_t width_px{0};
  std::int32_t height_px{0};
  std::int32_t line_height_px{0};
  std::string color{};
  std::string border_color{};
  std::string background{};
  std::string display{};
  std::string text_align{};
};

struct HtmlCssRule {
  std::string selector{};
  std::vector<HtmlAttribute> declarations{};
  std::int32_t media_min_width_px{0};
  std::int32_t media_max_width_px{0};
};

struct HtmlScript {
  std::vector<HtmlAttribute> attributes{};
  std::string content{};
};

struct HtmlNode {
  HtmlNodeKind kind{HtmlNodeKind::Element};
  HtmlWidgetKind widget{HtmlWidgetKind::None};
  std::string tag{};
  std::string text{};
  HtmlStyle style{};
  std::vector<HtmlAttribute> attributes{};
  std::vector<HtmlNode> children{};
};

struct HtmlDocument {
  std::string title{"PlussView"};
  HtmlNode root{
      .kind = HtmlNodeKind::Document,
      .widget = HtmlWidgetKind::None,
      .tag = "document",
  };
  std::vector<HtmlCssRule> style_rules{};
  std::unordered_map<std::string, std::string> css_variables{};
  std::vector<HtmlScript> scripts{};
};

[[nodiscard]] HtmlDocument parse_html_document(std::string_view html, std::string title = "PlussView");
[[nodiscard]] std::string flatten_text_content(const HtmlDocument& document);
[[nodiscard]] const HtmlAttribute* find_attribute(const HtmlNode& node, std::string_view name) noexcept;

} // namespace pluss::view::window
