#include "pluss/view/window/html_mapping.h"

#include <string>

namespace pluss::view::window {

namespace {

[[nodiscard]] HtmlNode make_text_node(std::string text, const std::int32_t font_px) {
  HtmlNode node{};
  node.kind = HtmlNodeKind::Text;
  node.text = std::move(text);
  node.style.font_size_px = font_px;
  return node;
}

[[nodiscard]] HtmlNode make_widget_node(const HtmlWidgetKind kind,
                                        std::string tag,
                                        std::vector<HtmlAttribute> attributes) {
  HtmlNode node{};
  node.kind = HtmlNodeKind::Widget;
  node.widget = kind;
  node.tag = std::move(tag);
  node.attributes = std::move(attributes);
  return node;
}

} // namespace

HtmlDocument map_line_plot_payload_to_html_document(const HtmlLinePlotPayload& payload) {
  HtmlDocument doc{};
  doc.title = payload.title.empty() ? std::string("Line Plot") : payload.title;

  doc.root.children.push_back(make_text_node(doc.title, 20));
  doc.root.children.push_back(make_text_node("x-axis: " + payload.x_label + ", y-axis: " + payload.y_label, 12));
  doc.root.children.push_back(make_widget_node(
      HtmlWidgetKind::LinePlot,
      "line-plot",
      {
          HtmlAttribute{"x-count", std::to_string(payload.x.size())},
          HtmlAttribute{"y-count", std::to_string(payload.y.size())},
      }));
  return doc;
}

HtmlDocument map_table_payload_to_html_document(const HtmlTablePayload& payload) {
  HtmlDocument doc{};
  doc.title = payload.title.empty() ? std::string("Table") : payload.title;

  doc.root.children.push_back(make_text_node(doc.title, 20));
  doc.root.children.push_back(make_widget_node(
      HtmlWidgetKind::Table,
      "table",
      {
          HtmlAttribute{"column-count", std::to_string(payload.headers.size())},
          HtmlAttribute{"row-count", std::to_string(payload.rows.size())},
          HtmlAttribute{"row-offset", std::to_string(payload.row_offset)},
      }));
  return doc;
}

HtmlDocument map_line_plot_scene_to_html_document(const Scene& scene, std::string title) {
  HtmlLinePlotPayload payload{};
  payload.title = title.empty() ? std::string("Line Plot") : std::move(title);
  payload.x_label = "x";
  payload.y_label = "y";
  payload.y.resize(scene.commands().size());
  return map_line_plot_payload_to_html_document(payload);
}

HtmlDocument map_table_scene_to_html_document(const Scene& scene, std::string title) {
  HtmlTablePayload payload{};
  payload.title = title.empty() ? std::string("Table") : std::move(title);
  payload.rows.resize(scene.commands().size());
  return map_table_payload_to_html_document(payload);
}

} // namespace pluss::view::window
