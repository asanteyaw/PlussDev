#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "pluss/tils/view/html_window.h"
#include "pluss/tils/view/table_order.h"
#include "pluss/tils/view/window.h"

namespace {

using pluss::view::window::HtmlRenderConfig;
using pluss::view::window::HtmlRenderStats;
using pluss::view::window::HtmlScrollModel;
using pluss::view::window::PointI;
using pluss::view::window::RectI;
using pluss::view::window::SizeI;

// A writable scratch file on any platform (the tests used a hardcoded /tmp, which Windows lacks).
[[nodiscard]] std::string scratch_path(const std::string& name) {
  return (std::filesystem::temp_directory_path() / name).string();
}

[[noreturn]] void fail(const std::string& message) {
  throw std::runtime_error(message);
}

void expect(const bool condition, const std::string& message) {
  if (!condition) {
    fail(message);
  }
}

[[nodiscard]] PointI center(const RectI rect) {
  return PointI{rect.x + rect.width / 2, rect.y + rect.height / 2};
}

[[nodiscard]] std::string stress_html() {
  std::string html = R"HTML(
<!doctype html>
<html>
<head>
  <title>PlussView Scroll Validation</title>
  <style>
    body { margin: 0; font-family: sans-serif; }
    .wide { width: 1500px; white-space: nowrap; padding: 8px; }
    table { height: 220px; border: 1px solid #cbd5e1; }
    thead th { position: sticky; top: 0; }
    th, td { padding: 8px 12px; white-space: nowrap; }
    p { margin: 8px; }
  </style>
</head>
<body>
  <div class="wide">A deliberately wide document region for horizontal document scrolling.</div>
  <table id="ledger">
    <thead><tr>
)HTML";
  for (int col = 0; col < 10; ++col) {
    html += col == 0 ? "<th class=\"sticky-left\">case</th>"
                     : "<th>Column " + std::to_string(col) + " with detail</th>";
  }
  html += "</tr></thead><tbody>";
  for (int row = 0; row < 48; ++row) {
    html += "<tr>";
    for (int col = 0; col < 10; ++col) {
      html += col == 0 ? "<td class=\"sticky-left\">case-" + std::to_string(row) + "</td>"
                       : "<td>r" + std::to_string(row) + " c" + std::to_string(col) + " value</td>";
    }
    html += "</tr>";
  }
  html += "</tbody></table>";
  for (int row = 0; row < 36; ++row) {
    html += "<p>Document paragraph " + std::to_string(row) +
            " keeps the page tall enough to validate vertical chaining.</p>";
  }
  html += "</body></html>";
  return html;
}

[[nodiscard]] std::string sticky_label_html() {
  return R"HTML(
<!doctype html>
<html>
<head>
  <style>
    body { margin: 0; font-family: sans-serif; }
    table { width: 980px; border: 1px solid #cbd5e1; }
    thead th { position: sticky; top: 0; }
    th, td { padding: 8px 12px; white-space: nowrap; }
  </style>
</head>
<body>
  <table>
    <thead>
      <tr>
        <th class="sticky-left">case</th>
        <th>count</th>
        <th>mean</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td class="sticky-left">customer_age_years_at_observation</td>
        <td>4500</td>
        <td>44.64</td>
      </tr>
      <tr>
        <td class="sticky-left">district_entrepreneurs_per_1000</td>
        <td>4500</td>
        <td>14662.86</td>
      </tr>
    </tbody>
  </table>
</body>
</html>
)HTML";
}

void render_scroll_frames(const pluss::view::window::HtmlDocument& document,
                          HtmlScrollModel& model,
                          const SizeI viewport,
                          HtmlRenderStats& stats) {
  using namespace pluss::view::window;

  ViewerRuntime runtime(RuntimeConfig{viewport, Color::rgba(245, 248, 252, 255)});
  HtmlRenderConfig config{};
  config.render_stats = &stats;
  std::vector<HtmlHitRegion> hit_regions;
  config.hit_regions = &hit_regions;
  auto scene = build_html_document_scene(document, viewport, config);
  const auto text_x = [](const auto& rendered_scene, const std::string_view needle) {
    for (const auto& command : rendered_scene.commands()) {
      if (const auto* text = std::get_if<TextCommand>(&command);
          text != nullptr && text->text.find(needle) != std::string::npos) {
        return text->anchor.x;
      }
    }
    return -1;
  };
  const int initial_case_x = text_x(scene, "case");
  expect(initial_case_x >= 0, "sticky case header should be present in the initial frame");
  const auto sticky_hit = std::find_if(hit_regions.begin(), hit_regions.end(), [](const auto& hit) {
    return hit.action == "table.sort.column" && hit.value == 0;
  });
  const auto regular_hit = std::find_if(hit_regions.begin(), hit_regions.end(), [](const auto& hit) {
    return hit.action == "table.sort.column" && hit.value == 1;
  });
  expect(sticky_hit != hit_regions.end() && regular_hit != hit_regions.end() &&
             sticky_hit->rect.width < regular_hit->rect.width,
         "the sticky case rail should be narrower than ordinary data columns");
  expect(runtime.submit(scene), "initial scroll validation frame should render");
  const std::vector<std::uint8_t> initial_pixels(
      runtime.canvas().bytes().begin(), runtime.canvas().bytes().end());
  expect(save_canvas(runtime.canvas(), scratch_path("plussview_scroll_initial.png")),
         "initial scroll validation frame should save");

  model.update_metrics(stats);
  expect(!stats.table_scroll_regions.empty(), "scroll validation table region should exist");
  const PointI table_point = center(stats.table_scroll_regions.front().viewport_rect);
  expect(model.scroll_wheel(-90.0, -120.0, table_point, true, false),
         "scroll validation model should move from the origin");

  const auto document_offset = model.document_offset_px();
  const auto table_offsets = model.table_offsets_px();
  config.document_scroll_x_px = document_offset.x;
  config.document_scroll_y_px = document_offset.y;
  config.table_scroll_offsets = &table_offsets;
  hit_regions.clear();
  scene = build_html_document_scene(document, viewport, config);
  expect(text_x(scene, "case") == initial_case_x,
         "the case column must remain fixed during horizontal table scrolling");
  expect(runtime.submit(scene), "scrolled validation frame should render");
  expect(!std::equal(initial_pixels.begin(),
                     initial_pixels.end(),
                     runtime.canvas().bytes().begin(),
                     runtime.canvas().bytes().end()),
         "scrolling should produce a visibly different rendered frame");
  expect(save_canvas(runtime.canvas(), scratch_path("plussview_scroll_scrolled.png")),
         "scrolled validation frame should save");

  const std::vector<std::uint8_t> table_scrolled_pixels(
      runtime.canvas().bytes().begin(), runtime.canvas().bytes().end());
  model.reset();
  model.update_metrics(stats);
  const PointI document_point{
      stats.document_viewport_rect.x + 4,
      stats.document_viewport_rect.y + 4,
  };
  expect(model.scroll_wheel(-100.0, -160.0, document_point, true, false),
         "document validation input should move both outer axes");
  const auto outer_offset = model.document_offset_px();
  config.document_scroll_x_px = outer_offset.x;
  config.document_scroll_y_px = outer_offset.y;
  const auto reset_table_offsets = model.table_offsets_px();
  config.table_scroll_offsets = &reset_table_offsets;
  std::vector<HtmlHitRegion> document_scrolled_hits;
  config.hit_regions = &document_scrolled_hits;
  scene = build_html_document_scene(document, viewport, config);
  for (const auto& region : stats.table_scroll_regions) {
    if (!region.viewport_rect.empty()) {
      expect(region.viewport_rect.y >= stats.document_viewport_rect.y,
             "off-screen table geometry must not capture input over the fixed title bar");
    }
  }
  expect(std::all_of(document_scrolled_hits.begin(),
                     document_scrolled_hits.end(),
                     [&](const HtmlHitRegion& region) {
                       return region.rect.y >= stats.document_viewport_rect.y;
                     }),
         "off-screen HTML controls must not capture clicks over the fixed title bar");
  expect(runtime.submit(scene), "document-scrolled validation frame should render");
  const std::size_t fixed_header_bytes =
      static_cast<std::size_t>(viewport.width) * 54U * 4U;
  expect(std::equal(initial_pixels.begin(),
                    initial_pixels.begin() + static_cast<std::ptrdiff_t>(fixed_header_bytes),
                    runtime.canvas().bytes().begin()),
         "nested off-screen clips must not paint over the fixed document title bar");
  expect(!std::equal(table_scrolled_pixels.begin(),
                     table_scrolled_pixels.end(),
                     runtime.canvas().bytes().begin(),
                     runtime.canvas().bytes().end()),
         "document scrolling should visibly differ from nested table scrolling");
  expect(save_canvas(runtime.canvas(), scratch_path("plussview_scroll_document.png")),
         "document-scrolled validation frame should save");
}

void test_sticky_case_column_expands_for_text_labels() {
  using namespace pluss::view::window;

  const auto document = parse_html_document(sticky_label_html());
  HtmlRenderStats stats;
  HtmlRenderConfig config{};
  config.render_stats = &stats;
  std::vector<HtmlHitRegion> hit_regions;
  config.hit_regions = &hit_regions;
  const auto scene = build_html_document_scene(document, SizeI{1100, 520}, config);

  const auto sticky_hit = std::find_if(hit_regions.begin(), hit_regions.end(), [](const auto& hit) {
    return hit.action == "table.sort.column" && hit.value == 0;
  });
  const auto regular_hit = std::find_if(hit_regions.begin(), hit_regions.end(), [](const auto& hit) {
    return hit.action == "table.sort.column" && hit.value == 1;
  });
  expect(sticky_hit != hit_regions.end(), "sticky case header hit region should exist");
  expect(regular_hit != hit_regions.end(), "regular data header hit region should exist");
  expect(sticky_hit->rect.width > 84,
         "sticky case column should grow beyond the old compact width when labels are textual");
  expect(sticky_hit->rect.width > regular_hit->rect.width,
         "sticky case column should expand to fit longer case labels when they exceed data-column headers");

  const auto long_label = std::find_if(scene.commands().begin(), scene.commands().end(), [](const auto& command) {
    const auto* text = std::get_if<TextCommand>(&command);
    return text != nullptr && text->text.find("customer_age_years_at_observation") != std::string::npos;
  });
  expect(long_label != scene.commands().end(),
         "long textual case labels should render without being collapsed to an ellipsis");
}

void test_automatic_table_identity() {
  using namespace pluss::view::window;

  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html>
<head>
  <style>
    body { margin: 0; }
    table { height: 180px; margin: 6px; }
    th, td { padding: 7px 12px; white-space: nowrap; }
  </style>
</head>
<body>
  <table>
    <tr><th>First table heading with width</th><th>Another wide heading</th><th>Final heading</th></tr>
    <tr><td>first 0</td><td>value</td><td>detail</td></tr>
    <tr><td>first 1</td><td>value</td><td>detail</td></tr>
    <tr><td>first 2</td><td>value</td><td>detail</td></tr>
    <tr><td>first 3</td><td>value</td><td>detail</td></tr>
    <tr><td>first 4</td><td>value</td><td>detail</td></tr>
    <tr><td>first 5</td><td>value</td><td>detail</td></tr>
    <tr><td>first 6</td><td>value</td><td>detail</td></tr>
  </table>
  <table>
    <tr><th>Second table heading with width</th><th>Another wide heading</th><th>Final heading</th></tr>
    <tr><td>second 0</td><td>value</td><td>detail</td></tr>
    <tr><td>second 1</td><td>value</td><td>detail</td></tr>
    <tr><td>second 2</td><td>value</td><td>detail</td></tr>
    <tr><td>second 3</td><td>value</td><td>detail</td></tr>
    <tr><td>second 4</td><td>value</td><td>detail</td></tr>
    <tr><td>second 5</td><td>value</td><td>detail</td></tr>
    <tr><td>second 6</td><td>value</td><td>detail</td></tr>
  </table>
</body>
</html>
)HTML");

  HtmlRenderStats stats{};
  HtmlRenderConfig config{};
  config.render_stats = &stats;
  (void)build_html_document_scene(document, SizeI{500, 620}, config);

  expect(stats.table_scroll_regions.size() == 2,
         "two visible anonymous tables should publish two scroll regions");
  expect(stats.table_scroll_regions[0].target != stats.table_scroll_regions[1].target,
         "anonymous tables should receive independent stable targets");

  HtmlRenderStats rerendered_stats{};
  config.render_stats = &rerendered_stats;
  (void)build_html_document_scene(document, SizeI{500, 620}, config);
  expect(rerendered_stats.table_scroll_regions.size() == 2,
         "anonymous table targets should survive a rerender");
  expect(stats.table_scroll_regions[0].target == rerendered_stats.table_scroll_regions[0].target &&
             stats.table_scroll_regions[1].target == rerendered_stats.table_scroll_regions[1].target,
         "anonymous table targets should remain stable for the document lifetime");
}

void test_layout_metrics_and_scroll_model() {
  using namespace pluss::view::window;

  const SizeI viewport{720, 480};
  const auto document = parse_html_document(stress_html());
  HtmlRenderStats stats{};
  HtmlRenderConfig config{};
  config.render_stats = &stats;
  const auto visible_scrollbars = build_html_document_scene(document, viewport, config);

  expect(stats.document_max_scroll_x > 0,
         "wide document content should produce horizontal document overflow");
  expect(stats.document_max_scroll_y > 0,
         "tall document content should produce vertical document overflow");
  expect(!stats.document_horizontal_track_rect.empty() &&
             !stats.document_horizontal_thumb_rect.empty(),
         "horizontal document scrollbar geometry should be published");
  expect(!stats.document_vertical_track_rect.empty() &&
             !stats.document_vertical_thumb_rect.empty(),
         "vertical document scrollbar geometry should be published");
  expect(stats.table_scroll_regions.size() == 1,
         "one table should publish one independent scroll region");

  const auto& table = stats.table_scroll_regions.front();
  expect(table.target == "ledger", "table scroll region should retain its HTML target");
  expect(table.max_scroll_x > 0, "wide table should support horizontal scrolling");
  expect(table.max_scroll_y > 0, "fixed-height table should support vertical scrolling");
  expect(!table.horizontal_track_rect.empty() && !table.horizontal_thumb_rect.empty(),
         "table horizontal scrollbar should be interactive");
  expect(!table.vertical_track_rect.empty() && !table.vertical_thumb_rect.empty(),
         "table vertical scrollbar should be interactive");

  HtmlScrollModel animated_model;
  animated_model.update_metrics(stats);
  const PointI animated_table_point = center(table.viewport_rect);
  expect(animated_model.scroll_wheel(0.0, -120.0, animated_table_point, true, false, true),
         "precise scrolling should accept a table delta");
  expect(animated_model.snapshot().tables.at("ledger").y == 120 &&
             !animated_model.has_pending_animation(),
         "precise platform deltas should apply directly without a second animation");

  animated_model.reset();
  animated_model.update_metrics(stats);
  expect(animated_model.scroll_wheel(0.0, -3.0, animated_table_point, false, false, true),
         "animated scrolling should accept a discrete mouse-wheel delta");
  expect(animated_model.snapshot().tables.at("ledger").y == 0 &&
             animated_model.has_pending_animation(),
         "discrete mouse-wheel scrolling should retain a target without jumping");
  expect(animated_model.advance_animation(0.016),
         "animated scrolling should advance on a frame tick");
  const int first_animated_y = animated_model.snapshot().tables.at("ledger").y;
  expect(first_animated_y > 0 && first_animated_y < 120,
         "the first animated frame should move partway toward the target");
  for (int frame = 0; frame < 120 && animated_model.has_pending_animation(); ++frame) {
    (void)animated_model.advance_animation(0.016);
  }
  expect(std::abs(animated_model.snapshot().tables.at("ledger").y - 120) <= 1 &&
             !animated_model.has_pending_animation(),
         "animated scrolling should converge to the requested offset and come to rest");

  config.draw_scrollbars = false;
  const auto hidden_scrollbars = build_html_document_scene(document, viewport, config);
  const auto scrollbar_paint_count = [](const auto& scene) {
    return std::count_if(scene.commands().begin(), scene.commands().end(), [](const auto& command) {
      const auto* rounded = std::get_if<pluss::view::window::FillRoundedRectCommand>(&command);
      if (rounded == nullptr) {
        return false;
      }
      const auto color = rounded->color;
      return (color.r == 218 && color.g == 224 && color.b == 231) ||
             (color.r == 94 && color.g == 111 && color.b == 130);
    });
  };
  expect(scrollbar_paint_count(visible_scrollbars) > 0,
         "default rendering should paint published scrollbar geometry");
  expect(scrollbar_paint_count(hidden_scrollbars) == 0,
         "overlay-free rendering should hide scrollbar tracks and thumbs");

  HtmlRenderStats table_only_stats{};
  HtmlRenderConfig table_only_config{};
  table_only_config.render_stats = &table_only_stats;
  table_only_config.enable_document_scroll = false;
  (void)build_html_document_scene(document, viewport, table_only_config);
  expect(table_only_stats.document_max_scroll_x == 0 &&
             table_only_stats.document_max_scroll_y == 0,
         "table-only mode should disable outer document scrolling");
  expect(table_only_stats.table_scroll_regions.size() == 1,
         "table-only mode should preserve independent table scroll regions");
  expect(table_only_stats.table_scroll_regions.front().max_scroll_x > 0 &&
             table_only_stats.table_scroll_regions.front().max_scroll_y > 0,
         "table-only mode should keep table-local scrolling active");

  HtmlScrollModel model;
  model.update_metrics(stats);
  const PointI table_point = center(table.viewport_rect);
  const PointI document_point{
      stats.document_viewport_rect.x + 8,
      stats.document_viewport_rect.y + stats.document_viewport_rect.height - 24,
  };

  expect(model.scroll_wheel(0.0, -12.5, table_point, true, false),
         "precise vertical trackpad input should scroll the table under the pointer");
  auto snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger").y == 13,
         "precise trackpad deltas should preserve the platform-provided distance");
  expect(snapshot.document.y == 0,
         "table scrolling should not move the document before reaching a table edge");

  expect(model.scroll_wheel(-9.0, 0.0, table_point, true, false),
         "precise horizontal trackpad input should scroll a wide table");
  snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger").x == 9,
         "horizontal trackpad input should preserve the platform-provided distance");

  expect(model.scroll_wheel(0.0, -1.0, table_point, false, true),
         "Shift+wheel should map vertical wheel input to horizontal scrolling");
  snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger").x == 49,
         "non-precise wheel input should use a stable line-sized step");

  const int document_before_chain = snapshot.document.y;
  expect(model.scroll_wheel(0.0, -100000.0, table_point, true, false),
         "large table scroll should reach its boundary and chain to the document");
  snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger").y == table.max_scroll_y,
         "table scrolling should clamp at its exact vertical boundary");
  expect(snapshot.document.y > document_before_chain,
         "unused table delta should chain into document scrolling");

  model.reset();
  model.update_metrics(stats);
  expect(model.scroll_wheel(0.0, -1.0, document_point, false, false),
         "wheel input outside a table should scroll the document");
  snapshot = model.snapshot();
  expect(snapshot.document.y == 40 && snapshot.tables.at("ledger").y == 0,
         "document routing should leave independent table offsets unchanged");

  model.reset();
  model.update_metrics(stats);
  const PointI track_after_thumb{
      table.vertical_track_rect.x + table.vertical_track_rect.width / 2,
      std::min(table.vertical_track_rect.y + table.vertical_track_rect.height - 1,
               table.vertical_thumb_rect.y + table.vertical_thumb_rect.height + 8),
  };
  expect(model.pointer_down(track_after_thumb),
         "clicking a table scrollbar track should be consumed");
  snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger").y > 0,
         "clicking after a scrollbar thumb should page forward");
  const int track_click_offset = snapshot.tables.at("ledger").y;
  expect(!model.pointer_drag(
             0.0, -20.0, PointI{track_after_thumb.x, track_after_thumb.y - 20}),
         "moving after a scrollbar track click should not begin content panning");
  expect(model.snapshot().tables.at("ledger").y == track_click_offset,
         "scrollbar track capture should remain stable until pointer release");
  model.pointer_up();

  model.reset();
  model.update_metrics(stats);
  const PointI thumb_center = center(table.horizontal_thumb_rect);
  expect(model.pointer_down(thumb_center),
         "pressing a table scrollbar thumb should begin dragging");
  expect(model.pointer_drag(30.0, 0.0, PointI{thumb_center.x + 30, thumb_center.y}),
         "dragging a horizontal scrollbar thumb should update its offset");
  model.pointer_up();
  snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger").x > 0,
         "scrollbar thumb drag should move the corresponding table only");

  model.reset();
  model.update_metrics(stats);
  const PointI content_drag_start{
      table.viewport_rect.x + table.viewport_rect.width / 2,
      table.viewport_rect.y + table.viewport_rect.height / 2,
  };
  expect(!model.pointer_down(content_drag_start),
         "pressing table content should not be mistaken for a scrollbar");
  expect(model.pointer_drag(
             -24.0,
             -30.0,
             PointI{content_drag_start.x - 24, content_drag_start.y - 30}),
         "dragging table content up and left should pan it down and right");
  model.pointer_up();
  snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger").x == 24 && snapshot.tables.at("ledger").y == 30,
         "content drag should use responsive movement on both axes");

  model.reset();
  HtmlRenderStats multiple_metrics = stats;
  HtmlScrollRegion second_table = table;
  second_table.target = "ledger-secondary";
  second_table.viewport_rect.y += table.viewport_rect.height + 20;
  second_table.horizontal_track_rect.y += table.viewport_rect.height + 20;
  second_table.horizontal_thumb_rect.y += table.viewport_rect.height + 20;
  second_table.vertical_track_rect.y += table.viewport_rect.height + 20;
  second_table.vertical_thumb_rect.y += table.viewport_rect.height + 20;
  multiple_metrics.table_scroll_regions.push_back(second_table);
  model.update_metrics(multiple_metrics);
  const PointI second_table_point = center(second_table.viewport_rect);
  expect(model.scroll_wheel(-11.0, -17.0, second_table_point, true, false),
         "scroll input should route to the second independent table");
  snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger").x == 0 && snapshot.tables.at("ledger").y == 0,
         "scrolling the second table should not alter the first table");
  expect(snapshot.tables.at("ledger-secondary").x == 11 &&
             snapshot.tables.at("ledger-secondary").y == 17,
         "the second table should retain its own two-axis offset");

  model.update_metrics(stats);
  snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger-secondary").x == 11 &&
             snapshot.tables.at("ledger-secondary").y == 17,
         "an off-screen table offset should remain available to the next render");
  model.update_metrics(multiple_metrics);
  snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger-secondary").x == 11 &&
             snapshot.tables.at("ledger-secondary").y == 17,
         "a temporarily off-screen table should retain its scroll position");

  HtmlRenderStats reduced_metrics = multiple_metrics;
  reduced_metrics.document_max_scroll_x = 7;
  reduced_metrics.document_max_scroll_y = 9;
  for (auto& region : reduced_metrics.table_scroll_regions) {
    region.max_scroll_x = 5;
    region.max_scroll_y = 6;
  }
  expect(model.command("end"),
         "End should move the active table before resize clamping");
  (void)model.scroll_wheel(-1000.0, 0.0, second_table_point, true, false);
  model.update_metrics(reduced_metrics);
  snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger-secondary").x <= 5 &&
             snapshot.tables.at("ledger-secondary").y <= 6,
         "table offsets should clamp when resize reduces overflow");
  expect(snapshot.document.x <= 7 && snapshot.document.y <= 9,
         "document offsets should clamp when resize reduces overflow");

  model.reset();
  model.update_metrics(stats);
  expect(model.scroll_wheel(
             0.0, -static_cast<double>(table.max_scroll_y), table_point, true, false),
         "table should scroll exactly to its lower boundary");
  expect(model.command("pan_down"),
         "keyboard movement at a table edge should propagate to the document");
  snapshot = model.snapshot();
  expect(snapshot.tables.at("ledger").y == table.max_scroll_y && snapshot.document.y > 0,
         "keyboard edge propagation should preserve the table boundary and move the document");

  model.reset();
  model.update_metrics(stats);
  (void)model.scroll_wheel(0.0, 0.0, document_point, true, false);
  expect(model.command("page_down"), "Page Down should scroll by a viewport-sized step");
  const int page_offset = model.document_offset_px().y;
  expect(page_offset >= static_cast<int>(stats.document_viewport_rect.height * 0.8),
         "Page Down should move close to one visible page");
  expect(model.command("end"), "End should jump to the document boundary");
  expect(model.document_offset_px().y == stats.document_max_scroll_y,
         "End should land exactly at the maximum document offset");
  expect(model.command("home"), "Home should return to the document origin");
  expect(model.document_offset_px().y == 0,
         "Home should land exactly at the document origin");

  render_scroll_frames(document, model, viewport, stats);
}

void test_retina_scene_scaling() {
  using namespace pluss::view::window;

  Scene scene;
  scene.add(FillRoundedRectCommand{
      RectI{3, 5, 40, 20},
      4,
      Color::rgba(10, 20, 30, 255),
  });
  scene.add(TextCommand{
      PointI{7, 11},
      "Retina",
      13,
      Color::rgba(40, 50, 60, 255),
  });

  const auto scaled = scale_scene(scene, 2.0);
  expect(scaled.commands().size() == 2, "Retina scaling should preserve scene command count");
  const auto* box = std::get_if<FillRoundedRectCommand>(&scaled.commands()[0]);
  const auto* text = std::get_if<TextCommand>(&scaled.commands()[1]);
  expect(box != nullptr && box->rect.x == 6 && box->rect.y == 10 &&
             box->rect.width == 80 && box->rect.height == 40 && box->radius == 8,
         "Retina scaling should scale box geometry and corner radius");
  expect(text != nullptr && text->anchor.x == 14 && text->anchor.y == 22 &&
             text->pixel_size == 26,
         "Retina scaling should scale text position and raster size");
}

void test_form_control_alignment_and_caret() {
  using namespace pluss::view::window;

  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html><head><style>
  body { margin:0; }
  .shell { padding:14px; }
  .frame { border:1px solid #cbd7e3; }
  .hero {
    padding:14px 16px; display:flex; gap:14px; align-items:center;
    justify-content:space-between; flex-wrap:wrap;
  }
  .tools {
    display:flex; gap:1px; align-items:center;
    justify-content:flex-end; margin-left:auto; flex-wrap:nowrap;
  }
  .tools input, .tools button {
    margin:0; height:40px; min-height:40px; padding:0 12px;
    border:1px solid #b9c7d5;
  }
  .tools input { min-width:220px; font-size:13px; text-align:left; }
  .tools button { min-width:88px; font-size:13px; text-align:center; }
</style></head><body>
  <div class="shell">
    <div class="frame">
      <header class="hero">
        <div><h1>DataFrame - head</h1><p>20 cases, 12 features</p></div>
        <div class="tools">
          <input id="table-search" placeholder="Search visible rows" />
          <button id="reset-table-view">Reset</button>
        </div>
      </header>
    </div>
  </div>
</body></html>
)HTML");

  std::vector<HtmlHitRegion> hits;
  HtmlRenderConfig config{};
  config.draw_top_bar = false;
  config.root_padding_px = 0;
  config.hit_regions = &hits;
  config.active_input_target = "table-search";
  const auto scene = build_html_document_scene(document, SizeI{640, 240}, config);
  const auto input = std::find_if(hits.begin(), hits.end(), [](const auto& hit) {
    return hit.action == "input.focus" && hit.target == "table-search";
  });
  const auto reset = std::find_if(hits.begin(), hits.end(), [](const auto& hit) {
    return hit.action == "button.click" && hit.target == "reset-table-view";
  });
  expect(input != hits.end() && reset != hits.end(),
         "search and reset controls should publish interaction geometry");
  expect(input->rect.y == reset->rect.y && input->rect.height == reset->rect.height,
         "search and reset controls should share the same top edge and height");
  expect(reset->rect.x + reset->rect.width >= 596,
         "auto-margin tools should align search and reset controls to the right edge (actual " +
             std::to_string(reset->rect.x + reset->rect.width) + ")");

  const auto caret = std::find_if(scene.commands().begin(), scene.commands().end(), [](const auto& command) {
    return std::holds_alternative<TextCaretCommand>(command);
  });
  expect(caret != scene.commands().end(), "focused search input should render a caret");
  const auto* caret_command = std::get_if<TextCaretCommand>(&*caret);
  expect(caret_command != nullptr &&
             caret_command->text_anchor.x > input->rect.x &&
             caret_command->text_anchor.x <= input->rect.x + 30 &&
             // "Short" means shorter than the input box. The caret is the glyph height plus a margin, and the glyph height comes from the
             // installed font, so an absolute pixel limit would depend on the machine's fonts.
             caret_command->y1 - caret_command->y0 < input->rect.height,
         "an empty search input should use a short caret at its left inset");

  config.show_input_caret = false;
  const auto blink_off_scene =
      build_html_document_scene(document, SizeI{640, 240}, config);
  expect(std::none_of(
             blink_off_scene.commands().begin(),
             blink_off_scene.commands().end(),
             [](const auto& command) {
               return std::holds_alternative<TextCaretCommand>(command);
             }),
         "the blink-off phase should hide only the focused input caret");

  config.show_input_caret = true;
  config.active_input_value = "abc";
  const auto text_scene =
      build_html_document_scene(document, SizeI{640, 240}, config);
  config.active_input_value = "abc ";
  const auto spaced_scene =
      build_html_document_scene(document, SizeI{640, 240}, config);
  const auto caret_x = [](const Scene& candidate) {
    for (const auto& command : candidate.commands()) {
      const auto* caret = std::get_if<TextCaretCommand>(&command);
      if (caret != nullptr) {
        return caret->text_anchor.x + caret->fallback_width;
      }
    }
    return -1;
  };
  expect(caret_x(text_scene) >= 0 &&
             caret_x(spaced_scene) > caret_x(text_scene),
         "an explicit trailing space should advance the caret");

  config.active_input_value = "Nnor";
  const auto exact_scene =
      build_html_document_scene(document, SizeI{640, 240}, config);
  const auto text_command = std::find_if(
      exact_scene.commands().begin(), exact_scene.commands().end(),
      [](const auto& command) {
        const auto* text = std::get_if<TextCommand>(&command);
        return text != nullptr && text->text == "Nnor";
      });
  const auto exact_caret = std::find_if(
      exact_scene.commands().begin(), exact_scene.commands().end(),
      [](const auto& command) {
        return std::holds_alternative<TextCaretCommand>(command);
      });
  expect(text_command != exact_scene.commands().end() &&
             exact_caret != exact_scene.commands().end(),
         "typed input should publish paired text and device-scale caret commands");

  Scene text_only;
  text_only.add(*text_command);
  Scene caret_only;
  caret_only.add(*exact_caret);
  text_only = scale_scene(text_only, 2.0);
  caret_only = scale_scene(caret_only, 2.0);
  PixelCanvas text_canvas(SizeI{1280, 480});
  PixelCanvas caret_canvas(SizeI{1280, 480});
  text_canvas.clear(Color::rgba(0, 0, 0, 0));
  caret_canvas.clear(Color::rgba(0, 0, 0, 0));
  SoftwareRenderer renderer;
  std::vector<RenderEvent> events;
  expect(renderer.render(text_only, text_canvas, events),
         "typed input text should rasterize");
  events.clear();
  expect(renderer.render(caret_only, caret_canvas, events),
         "typed input caret should rasterize");
  const auto rightmost_alpha_x = [](const PixelCanvas& canvas) {
    const auto size = canvas.size();
    const auto bytes = canvas.bytes();
    for (int x = size.width - 1; x >= 0; --x) {
      for (int y = 0; y < size.height; ++y) {
        const std::size_t alpha =
            (static_cast<std::size_t>(y) *
                 static_cast<std::size_t>(size.width) +
             static_cast<std::size_t>(x)) *
                4U +
            3U;
        if (bytes[alpha] != 0U) {
          return x;
        }
      }
    }
    return -1;
  };
  const auto leftmost_alpha_x = [](const PixelCanvas& canvas) {
    const auto size = canvas.size();
    const auto bytes = canvas.bytes();
    for (int x = 0; x < size.width; ++x) {
      for (int y = 0; y < size.height; ++y) {
        const std::size_t alpha =
            (static_cast<std::size_t>(y) *
                 static_cast<std::size_t>(size.width) +
             static_cast<std::size_t>(x)) *
                4U +
            3U;
        if (bytes[alpha] != 0U) {
          return x;
        }
      }
    }
    return -1;
  };
  const int text_right = rightmost_alpha_x(text_canvas);
  const int caret_left = leftmost_alpha_x(caret_canvas);
  // Touching means no gap; the last glyph column may be shared (text rasterisers differ by a pixel).
  expect(text_right >= 0 && caret_left >= text_right - 1 && caret_left <= text_right + 1,
         "Retina caret should touch the final rendered glyph without a gap "
         "(text right " +
             std::to_string(text_right) + ", caret left " +
             std::to_string(caret_left) + ")");
}

void test_table_body_values_share_left_edge() {
  using namespace pluss::view::window;

  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html><head><style>
  body { margin:0; }
  table { width:100%; height:220px; }
  th, td { padding:8px 12px; }
  thead th.num { text-align:center; }
  tbody td.num { text-align:left; }
  td.missing { background:#fff0ee; }
</style></head><body>
  <table><thead><tr><th class="num">value</th></tr></thead><tbody>
    <tr><td class="num">10.23</td></tr>
    <tr><td class="num">323.4958</td></tr>
    <tr><td class="num missing">NA</td></tr>
  </tbody></table>
</body></html>
)HTML");

  HtmlRenderConfig config{};
  config.draw_top_bar = false;
  config.root_padding_px = 0;
  const auto scene = build_html_document_scene(document, SizeI{480, 260}, config);
  const auto text_x = [&](const std::string_view value) {
    for (const auto& command : scene.commands()) {
      if (const auto* text = std::get_if<TextCommand>(&command);
          text != nullptr && text->text == value) {
        return text->anchor.x;
      }
    }
    return -1;
  };
  const int short_number_x = text_x("10.23");
  const int long_number_x = text_x("323.4958");
  const int missing_x = text_x("NA");
  expect(short_number_x >= 0 && short_number_x == long_number_x &&
             long_number_x == missing_x,
         "numeric and missing body values should share one left edge");
  expect(std::any_of(scene.commands().begin(), scene.commands().end(), [](const auto& command) {
           const auto* fill = std::get_if<FillRectCommand>(&command);
           return fill != nullptr && fill->color.r == 255 && fill->color.g == 240 &&
                  fill->color.b == 238;
         }),
         "left-aligned missing values should retain their special cell color");
}

void test_virtual_table_colours_follow_the_stylesheet() {
  using namespace pluss::view;
  using namespace pluss::view::window;

  class KindTable final : public VirtualTableModel {
   public:
    std::int64_t row_count() const noexcept override { return 4; }
    std::int64_t column_count() const noexcept override { return 1; }
    std::string row_header() const override { return "case"; }
    std::string row_label(const std::int64_t row) const override { return std::to_string(row); }
    std::string column_name(std::int64_t) const override { return "x"; }
    bool column_numeric(std::int64_t) const noexcept override { return true; }
    VirtualTableCell cell(const std::int64_t row, std::int64_t) const override {
      if (row == 0) return VirtualTableCell{.text = "NULL", .missing = true};
      if (row == 1) return VirtualTableCell{.text = "NAN", .nonfinite = true};
      if (row == 2) return VirtualTableCell{.text = "INF", .nonfinite = true, .infinite = true};
      return VirtualTableCell{.text = "1"};
    }
    bool row_matches(std::int64_t, const std::string_view) const override { return true; }
  } model;

  // Only a missing cell is marked, and only by its text: the stylesheet's colour and weight. NaN and infinity are
  // values and are drawn like any other, whatever the stylesheet says about their classes.
  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html><head><style>
  body { margin:0; }
  table { width:100%; height:300px; }
  td { background:#ffffff; }
  .missing { color:#aa0000; font-weight:800; }
  .nonfinite.nan { background:#00aa00; }
  .nonfinite.inf { background:#0000aa; }
</style></head><body>
  <table id="kinds" data-virtual-table="true"></table>
</body></html>
)HTML");

  HtmlRenderConfig config{};
  config.draw_top_bar = false;
  config.root_padding_px = 0;
  config.virtual_table_model = &model;
  const auto scene = build_html_document_scene(document, SizeI{600, 400}, config);
  const auto has_fill = [&](const Color wanted) {
    return std::any_of(scene.commands().begin(), scene.commands().end(), [&](const auto& command) {
      const auto* fill = std::get_if<FillRectCommand>(&command);
      return fill != nullptr && fill->color.r == wanted.r && fill->color.g == wanted.g && fill->color.b == wanted.b;
    });
  };
  const auto text_of = [&](const std::string_view text) -> const TextCommand* {
    for (const auto& command : scene.commands()) {
      if (const auto* drawn = std::get_if<TextCommand>(&command); drawn != nullptr && drawn->text == text) {
        return drawn;
      }
    }
    return nullptr;
  };
  const auto* missing = text_of("NULL");
  expect(missing != nullptr && missing->color.r == 0xaa && missing->color.g == 0x00 && missing->color.b == 0x00,
         "a missing cell's text takes the stylesheet's missing colour");
  expect(missing != nullptr && missing->font_weight == 800, "a missing cell's text takes the stylesheet's weight");
  const auto* nan = text_of("NAN");
  expect(nan != nullptr && nan->font_weight == 400 && !(nan->color.r == 0xaa && nan->color.g == 0x00),
         "a NaN cell is drawn like an ordinary value");
  expect(!has_fill(Color::rgba(0x00, 0xaa, 0x00, 255)) && !has_fill(Color::rgba(0x00, 0x00, 0xaa, 255)),
         "NaN and infinity cells are not painted a colour of their own");
}

void test_virtual_table_last_column_takes_the_slack() {
  using namespace pluss::view;
  using namespace pluss::view::window;

  class Narrow final : public VirtualTableModel {
   public:
    explicit Narrow(const std::int64_t columns) : columns_(columns) {}
    std::int64_t row_count() const noexcept override { return 20000; }  // long enough to use the virtual path
    std::int64_t column_count() const noexcept override { return columns_; }
    std::string row_header() const override { return "case"; }
    std::string row_label(const std::int64_t row) const override { return std::to_string(row); }
    std::string column_name(const std::int64_t column) const override { return "c" + std::to_string(column); }
    bool column_numeric(std::int64_t) const noexcept override { return true; }
    VirtualTableCell cell(const std::int64_t row, std::int64_t) const override { return VirtualTableCell{.text = std::to_string(row)}; }
    bool row_matches(std::int64_t, const std::string_view) const override { return true; }

   private:
    std::int64_t columns_;
  };

  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html><head><style>body { margin:0; } table { width:100%; height:300px; }</style></head><body>
  <table id="t" data-virtual-table="true"></table>
</body></html>
)HTML");
  const auto render = [&](const Narrow& model) {
    HtmlRenderStats stats{};
    HtmlRenderConfig config{};
    config.draw_top_bar = false;
    config.root_padding_px = 0;
    config.virtual_table_model = &model;
    config.render_stats = &stats;
    (void)build_html_document_scene(document, SizeI{900, 400}, config);
    return stats;
  };

  // A few narrow columns end well short of the panel on their own: the last one takes the rest, so the table
  // reaches the panel's right edge and has nothing to scroll sideways.
  const auto few = render(Narrow(3));
  expect(few.table_max_scroll_x == 0, "a long table of a few columns does not scroll sideways");
  expect(few.table_content_rect.width == few.table_viewport_rect.width,
         "the last column of a long table fills the panel to its right edge");

  // Many columns are wider than the panel: the last column keeps its own width and the table scrolls.
  const auto many = render(Narrow(60));
  expect(many.table_max_scroll_x > 0, "a long table wider than the panel still scrolls sideways");
}

void test_virtual_table_columns_can_be_resized() {
  using namespace pluss::view;
  using namespace pluss::view::window;

  class Wide final : public VirtualTableModel {
   public:
    std::int64_t row_count() const noexcept override { return 20000; }  // long enough to use the virtual path
    std::int64_t column_count() const noexcept override { return 3; }
    std::string row_header() const override { return "case"; }
    std::string row_label(const std::int64_t row) const override { return std::to_string(row); }
    std::string column_name(const std::int64_t column) const override { return "c" + std::to_string(column); }
    bool column_numeric(std::int64_t) const noexcept override { return true; }
    VirtualTableCell cell(const std::int64_t row, std::int64_t) const override { return VirtualTableCell{.text = std::to_string(row)}; }
    bool row_matches(std::int64_t, const std::string_view) const override { return true; }
  };

  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html><head><style>body { margin:0; } table { width:100%; height:300px; }</style></head><body>
  <table id="t" data-virtual-table="true"></table>
</body></html>
)HTML");
  const Wide model;
  struct Result {
    std::vector<HtmlHitRegion> hits;
    int hover_lines{0};
  };
  const auto render = [&](const std::unordered_map<std::string, std::unordered_map<int, int>>* widths,
                          const std::string& hover_target, const int hover_column) {
    Result out;
    HtmlRenderConfig config{};
    config.draw_top_bar = false;
    config.root_padding_px = 0;
    config.virtual_table_model = &model;
    config.hit_regions = &out.hits;
    config.table_column_widths = widths;
    config.hover_resize_target = hover_target;
    config.hover_resize_column = hover_column;
    const auto scene = build_html_document_scene(document, SizeI{900, 400}, config);
    for (const auto& command : scene.commands()) {
      if (const auto* line = std::get_if<LineCommand>(&command); line != nullptr && line->thickness == 3) {
        ++out.hover_lines;
      }
    }
    return out;
  };
  const auto with_action = [](const Result& r, const std::string_view action) {
    std::vector<HtmlHitRegion> out;
    for (const auto& hit : r.hits) {
      if (hit.action == action) {
        out.push_back(hit);
      }
    }
    return out;
  };

  // Columns are numbered as in a short table: the label column is 0, the model's columns follow. Every column
  // but the last (the automatic one) has a divider to drag, and every column has the sort arrows.
  const auto initial = render(nullptr, "", -1);
  const auto handles = with_action(initial, "table.resize.column");
  expect(handles.size() == 3, "the label column and the first two of three columns have drag handles; the last takes the slack");
  expect(with_action(initial, "table.sort.asc").size() == 4 && with_action(initial, "table.sort.desc").size() == 4,
         "every header of a long table, the label column included, has sort arrows");
  expect(initial.hover_lines == 0, "no divider is highlighted while the pointer is elsewhere");

  // The divider under the pointer is highlighted, as in a short table.
  const auto hovered = render(nullptr, handles.front().target, 1);
  expect(hovered.hover_lines == 1, "the divider under the pointer is highlighted in a long table");

  // A dragged width is applied to its column.
  const auto handle_for = [](const std::vector<HtmlHitRegion>& found, const int column) {
    return std::find_if(found.begin(), found.end(), [column](const HtmlHitRegion& h) { return h.value == column; });
  };
  const int dragged = handle_for(handles, 1)->aux * 3 / 2;  // wider than measured, with its divider still on screen
  std::unordered_map<std::string, std::unordered_map<int, int>> widths;
  widths[handles.front().target][1] = dragged;
  const auto after = with_action(render(&widths, "", -1), "table.resize.column");
  const auto widened = handle_for(after, 1);
  expect(widened != after.end() && widened->aux == dragged, "a dragged width is applied to its column in a long table");
}

void test_virtual_table_double_click_fits_a_squeezed_column() {
  using namespace pluss::view;
  using namespace pluss::view::window;

  constexpr std::int64_t kLongRow = 12345;
  class Crowded final : public VirtualTableModel {
   public:
    std::int64_t row_count() const noexcept override { return 20000; }
    std::int64_t column_count() const noexcept override { return 12; }
    std::string row_header() const override { return "case"; }
    std::string row_label(const std::int64_t row) const override { return std::to_string(row); }
    std::string column_name(const std::int64_t column) const override { return "column_" + std::to_string(column); }
    bool column_numeric(std::int64_t) const noexcept override { return false; }
    VirtualTableCell cell(const std::int64_t row, std::int64_t) const override {
      // One row deep in the table, which the rows read for the first layout do not reach, is much the longest.
      return VirtualTableCell{.text = row == kLongRow ? std::string(40, 'W') : "a fairly long cell value " + std::to_string(row)};
    }
    bool row_matches(std::int64_t, const std::string_view) const override { return true; }
  };
  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html><head><style>body { margin:0; } table { width:100%; height:300px; }</style></head><body>
  <table id="t" data-virtual-table="true"></table>
</body></html>
)HTML");
  const Crowded model;
  const auto column_width = [&](const std::unordered_map<std::string, std::unordered_map<int, int>>* widths) {
    std::vector<HtmlHitRegion> hits;
    HtmlRenderConfig config{};
    config.draw_top_bar = false;
    config.root_padding_px = 0;
    config.virtual_table_model = &model;
    config.hit_regions = &hits;
    config.table_column_widths = widths;
    (void)build_html_document_scene(document, SizeI{900, 400}, config);
    std::string target;
    int width = -1;
    for (const auto& hit : hits) {
      if (hit.action == "table.resize.column" && hit.value == 1) {
        width = hit.aux;
        target = hit.target;
      }
    }
    return std::pair{target, width};
  };
  const auto [target, squeezed] = column_width(nullptr);
  expect(squeezed > 0, "the squeezed column has a divider to double-click");
  std::unordered_map<std::string, std::unordered_map<int, int>> widths;
  widths[target][1] = 0;  // a double-click: fit
  const int fitted = column_width(&widths).second;
  expect(fitted > squeezed, "fitting a squeezed column of a long table makes it as wide as its values");
  const auto geo = resolve_table_geometry(13);
  expect(fitted > geo.max_column,
         "the fit reaches the widest value over every row, not only the rows read for the first layout");
  widths[target][1] = 0;  // a second double-click: the column stays fitted (a guide, not a toggle)
  expect(column_width(&widths).second == fitted, "double-clicking a fitted column leaves it as it is");
  widths[target][1] = geo.min_resize;  // dragged narrow
  expect(column_width(&widths).second < fitted, "a drag still sizes the column freely");
  widths[target][1] = 0;  // and a double-click brings it back to the fit
  expect(column_width(&widths).second == fitted, "double-clicking a dragged column returns it to the fit");
}

void test_no_matching_rows_is_centred() {
  using namespace pluss::view;
  using namespace pluss::view::window;

  class Few final : public VirtualTableModel {
   public:
    std::int64_t row_count() const noexcept override { return 20000; }
    std::int64_t column_count() const noexcept override { return 2; }
    std::string row_header() const override { return "case"; }
    std::string row_label(const std::int64_t row) const override { return std::to_string(row); }
    std::string column_name(const std::int64_t column) const override { return "c" + std::to_string(column); }
    bool column_numeric(std::int64_t) const noexcept override { return true; }
    VirtualTableCell cell(const std::int64_t row, std::int64_t) const override { return VirtualTableCell{.text = std::to_string(row)}; }
    bool row_matches(std::int64_t, const std::string_view) const override { return false; }
  };
  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html><head><style>body { margin:0; } table { width:100%; height:300px; }</style></head><body>
  <table id="t" data-virtual-table="true"></table>
</body></html>
)HTML");
  const Few model;
  const std::vector<std::int64_t> none;
  HtmlRenderConfig config{};
  config.draw_top_bar = false;
  config.root_padding_px = 0;
  config.virtual_table_model = &model;
  config.virtual_row_order = &none;
  const SizeI size{900, 400};
  const auto scene = build_html_document_scene(document, size, config);
  int start = -1;
  for (const auto& command : scene.commands()) {
    if (const auto* text = std::get_if<TextCommand>(&command); text != nullptr && text->text == "No matching rows") {
      start = text->anchor.x;
    }
  }
  expect(start > 0, "an empty table says there are no matching rows");
  // Centred text starts left of the middle by half its own width, which is well clear of the table's left edge.
  expect(start > size.width / 3 && start < size.width / 2, "the message sits in the middle of the table, not at its left edge");
}

void test_table_ends_at_its_last_row() {
  using namespace pluss::view;
  using namespace pluss::view::window;

  class Rows final : public VirtualTableModel {
   public:
    explicit Rows(const std::int64_t rows) : rows_(rows) {}
    std::int64_t row_count() const noexcept override { return rows_; }
    std::int64_t column_count() const noexcept override { return 2; }
    std::string row_header() const override { return "case"; }
    std::string row_label(const std::int64_t row) const override { return std::to_string(row); }
    std::string column_name(const std::int64_t column) const override { return "c" + std::to_string(column); }
    bool column_numeric(std::int64_t) const noexcept override { return true; }
    VirtualTableCell cell(const std::int64_t row, std::int64_t) const override { return VirtualTableCell{.text = std::to_string(row)}; }
    bool row_matches(std::int64_t, const std::string_view) const override { return true; }

   private:
    std::int64_t rows_;
  };
  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html><head><style>body { margin:0; } table { width:100%; height:500px; }</style></head><body>
  <table id="t" data-virtual-table="true"></table>
</body></html>
)HTML");
  struct Shape {
    int height{0};
    int font_px{0};
  };
  const auto measure = [&](const std::int64_t rows) {
    const Rows model(rows);
    HtmlRenderStats stats{};
    HtmlRenderConfig config{};
    config.draw_top_bar = false;
    config.root_padding_px = 0;
    config.virtual_table_model = &model;
    config.render_stats = &stats;
    (void)build_html_document_scene(document, SizeI{900, 700}, config);
    return Shape{stats.table_viewport_rect.height, stats.table_font_px};
  };
  const auto few = measure(4);
  const auto geo = resolve_table_geometry(few.font_px);
  expect(few.height == geo.header_height + 4 * geo.row_height,
         "a table of four rows ends at its fourth row, with nothing below it");
  expect(measure(0).height == geo.header_height + geo.row_height,
         "an empty table keeps one row of room for its message");
  expect(measure(20000).height > few.height, "a long table still fills the viewport");
}

void test_sizing_a_column_moves_only_its_right_edge() {
  using namespace pluss::view;
  using namespace pluss::view::window;

  class Five final : public VirtualTableModel {
   public:
    std::int64_t row_count() const noexcept override { return 20000; }
    std::int64_t column_count() const noexcept override { return 5; }
    std::string row_header() const override { return "case"; }
    std::string row_label(const std::int64_t row) const override { return std::to_string(row); }
    std::string column_name(const std::int64_t column) const override { return "c" + std::to_string(column); }
    bool column_numeric(std::int64_t) const noexcept override { return true; }
    VirtualTableCell cell(const std::int64_t row, std::int64_t) const override { return VirtualTableCell{.text = std::to_string(row)}; }
    bool row_matches(std::int64_t, const std::string_view) const override { return true; }
  };
  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html><head><style>body { margin:0; } table { width:100%; height:300px; }</style></head><body>
  <table id="t" data-virtual-table="true"></table>
</body></html>
)HTML");
  const Five model;
  // Where each divider sits: the divider of column n is the right edge of column n, the left edge of the next one.
  const auto dividers = [&](const std::unordered_map<std::string, std::unordered_map<int, int>>* widths,
                            std::string* target) {
    std::vector<HtmlHitRegion> hits;
    HtmlRenderConfig config{};
    config.draw_top_bar = false;
    config.root_padding_px = 0;
    config.virtual_table_model = &model;
    config.hit_regions = &hits;
    config.table_column_widths = widths;
    (void)build_html_document_scene(document, SizeI{900, 400}, config);
    std::map<int, int> at;
    for (const auto& hit : hits) {
      if (hit.action == "table.resize.column") {
        at[hit.value] = hit.rect.x + hit.rect.width / 2;
        if (target != nullptr) {
          *target = hit.target;
        }
      }
    }
    return at;
  };
  std::string target;
  const auto before = dividers(nullptr, &target);
  expect(before.size() >= 4, "the table has dividers to drag");
  // Drag column 3 wider: its left edge (the divider of column 2) must not move, its right edge moves right.
  std::unordered_map<std::string, std::unordered_map<int, int>> widths;
  const int wider = (before.at(3) - before.at(2)) * 3 / 2;
  widths[target][3] = wider;
  const auto after = dividers(&widths, nullptr);
  expect(after.at(1) == before.at(1) && after.at(2) == before.at(2),
         "widening a column leaves every divider to its left where it was");
  expect(after.at(3) > before.at(3), "the widened column's right edge moves right");
  // And a fit (a double-click) is the same: the left edge stays.
  widths[target].clear();
  widths[target][3] = 0;
  const auto fitted = dividers(&widths, nullptr);
  expect(fitted.at(1) == before.at(1) && fitted.at(2) == before.at(2),
         "fitting a column leaves every divider to its left where it was");
}

void test_virtual_table_sorts_by_a_column() {
  using namespace pluss::view;

  // Rows 0..5 by label; "n" holds numbers with a NaN, an infinity and a missing cell; "t" holds text.
  class Sortable final : public VirtualTableModel {
   public:
    std::int64_t row_count() const noexcept override { return 6; }
    std::int64_t column_count() const noexcept override { return 2; }
    std::string row_header() const override { return "case"; }
    std::string row_label(const std::int64_t row) const override { return std::to_string(row); }
    std::string column_name(const std::int64_t column) const override { return column == 0 ? "n" : "t"; }
    bool column_numeric(const std::int64_t column) const noexcept override { return column == 0; }
    VirtualTableCell cell(const std::int64_t row, const std::int64_t column) const override {
      static const char* numbers[] = {"10", "2", "NAN", "INF", "NULL", "-3"};
      static const char* words[] = {"pear", "apple", "fig", "kiwi", "date", "plum"};
      if (column == 0) {
        return VirtualTableCell{.text = numbers[row], .missing = row == 4, .nonfinite = row == 2 || row == 3, .infinite = row == 3};
      }
      return VirtualTableCell{.text = words[row]};
    }
    bool row_matches(std::int64_t, const std::string_view) const override { return true; }
  } model;

  const auto sorted = [&](const int column, const bool ascending) {
    std::vector<std::int64_t> rows;
    sort_model_rows(model, rows, column, ascending);
    return rows;
  };
  using Rows = std::vector<std::int64_t>;
  // Numbers sort by value (so 2 comes before 10), then NaN, then the missing cell, in either direction.
  expect(sorted(1, true) == Rows({5, 1, 0, 3, 2, 4}), "a numeric column sorts ascending by value, then NaN, then missing");
  expect(sorted(1, false) == Rows({3, 0, 1, 5, 2, 4}), "descending reverses the numbers but keeps NaN and missing last");
  expect(sorted(2, true) == Rows({1, 4, 2, 3, 0, 5}), "a text column sorts alphabetically");
  expect(sorted(2, false) == Rows({5, 0, 3, 2, 4, 1}), "a text column sorts descending");
  expect(sorted(0, false) == Rows({5, 4, 3, 2, 1, 0}), "the label column sorts by its labels");
}

void test_long_and_short_tables_draw_headers_alike() {
  using namespace pluss::view;
  using namespace pluss::view::window;

  class Two final : public VirtualTableModel {
   public:
    std::int64_t row_count() const noexcept override { return 20000; }
    std::int64_t column_count() const noexcept override { return 1; }
    std::string row_header() const override { return "case"; }
    std::string row_label(const std::int64_t row) const override { return std::to_string(row); }
    std::string column_name(std::int64_t) const override { return "name"; }
    bool column_numeric(std::int64_t) const noexcept override { return false; }
    VirtualTableCell cell(const std::int64_t row, std::int64_t) const override { return VirtualTableCell{.text = std::to_string(row)}; }
    bool row_matches(std::int64_t, const std::string_view) const override { return true; }
  } model;

  const std::string style = "<style>body { margin:0; } table { width:100%; height:300px; } thead th { font-weight:700; color:#223344; }</style>";
  const auto long_doc = parse_html_document("<!doctype html><html><head>" + style +
                                            "</head><body><table id=\"t\" data-virtual-table=\"true\"></table></body></html>");
  const auto short_doc = parse_html_document(
      "<!doctype html><html><head>" + style +
      "</head><body><table id=\"t\"><thead><tr><th class=\"sortable\">name</th></tr></thead>"
      "<tbody><tr><td>1</td></tr></tbody></table></body></html>");

  const auto header_text = [](const HtmlDocument& document, const VirtualTableModel* virtual_model) {
    HtmlRenderConfig config{};
    config.draw_top_bar = false;
    config.root_padding_px = 0;
    config.virtual_table_model = virtual_model;
    const auto scene = build_html_document_scene(document, SizeI{900, 400}, config);
    for (const auto& command : scene.commands()) {
      if (const auto* drawn = std::get_if<TextCommand>(&command); drawn != nullptr && drawn->text == "name") {
        return *drawn;
      }
    }
    return TextCommand{};
  };
  const auto long_header = header_text(long_doc, &model);
  const auto short_header = header_text(short_doc, nullptr);
  expect(long_header.text == "name" && short_header.text == "name", "both tables draw their column name");
  expect(long_header.font_weight == 700 && short_header.font_weight == 700,
         "a column name takes the stylesheet's weight in a long table as in a short one");
  expect(long_header.color.r == short_header.color.r && long_header.color.g == short_header.color.g &&
             long_header.color.b == short_header.color.b,
         "a column name takes the stylesheet's colour in a long table as in a short one");
}

void test_row_window_bounds() {
  using namespace pluss::view;
  using Kind = RowWindow::Kind;
  const auto bounds = [](const Kind kind, const std::int64_t total, const std::int64_t count = 10) {
    return window_bounds(RowWindow{.kind = kind, .count = count}, total);
  };
  using Pair = std::pair<std::int64_t, std::int64_t>;
  // Fewer rows than asked for: every window is all of them.
  expect(bounds(Kind::First, 5) == Pair(0, 5) && bounds(Kind::Middle, 5) == Pair(0, 5) && bounds(Kind::Last, 5) == Pair(0, 5),
         "with 5 rows the first, middle and last 10 are all 5 rows");
  // 11 rows: the middle is counted from the top, so it is rows 2 to 11, the same as the last.
  expect(bounds(Kind::First, 11) == Pair(0, 10) && bounds(Kind::Middle, 11) == Pair(1, 11) && bounds(Kind::Last, 11) == Pair(1, 11),
         "with 11 rows the first 10 are 1-10 and the middle and last 10 are 2-11");
  // 12 rows: all three differ.
  expect(bounds(Kind::First, 12) == Pair(0, 10) && bounds(Kind::Middle, 12) == Pair(1, 11) && bounds(Kind::Last, 12) == Pair(2, 12),
         "with 12 rows the first, middle and last 10 are 1-10, 2-11 and 3-12");
  expect(bounds(Kind::Middle, 13) == Pair(2, 12), "the middle stays counted from the top: 3-12 of 13");
  expect(bounds(Kind::First, 0) == Pair(0, 0), "an empty table has no rows to visit");
  const auto range = [](const std::int64_t begin, const std::int64_t end, const std::int64_t total) {
    return window_bounds(RowWindow{.kind = Kind::Range, .begin = begin, .end = end}, total);
  };
  expect(range(3, 8, 20) == Pair(3, 9), "a range includes both ends: 3 to 8 is the rows 3, 4, 5, 6, 7 and 8");
  expect(range(5, 5, 20) == Pair(5, 6), "a range of one position is that one row");
  expect(range(-5, 99, 20) == Pair(15, 20), "a negative begin counts from the back and an end past the table is clamped");
  expect(range(0, -1, 20) == Pair(0, 20), "-1 is the last row, so 0 to -1 is every row");
  expect(range(9, 4, 20) == Pair(9, 9), "an end before the begin is an empty range");
}

void test_virtual_table_only_materializes_viewport_cells() {
  using namespace pluss::view;
  using namespace pluss::view::window;

  class CountingTable final : public VirtualTableModel {
   public:
    std::int64_t row_count() const noexcept override { return 5000000; }
    std::int64_t column_count() const noexcept override { return 1000; }
    std::string row_header() const override { return "case"; }
    std::string row_label(const std::int64_t row) const override {
      return "row-" + std::to_string(row);
    }
    std::string column_name(const std::int64_t column) const override {
      return "feature_" + std::to_string(column);
    }
    bool column_numeric(std::int64_t) const noexcept override { return true; }
    VirtualTableCell cell(const std::int64_t row,
                          const std::int64_t column) const override {
      ++cell_calls;
      min_cell_row = std::min(min_cell_row, row);
      max_cell_row = std::max(max_cell_row, row);
      return VirtualTableCell{
          .text = std::to_string(row * 1000 + column),
          .missing = false,
      };
    }
    bool row_matches(const std::int64_t row,
                     const std::string_view query) const override {
      return row_label(row).find(query) != std::string::npos;
    }
    mutable int cell_calls{0};
    mutable std::int64_t min_cell_row{
        std::numeric_limits<std::int64_t>::max()};
    mutable std::int64_t max_cell_row{0};
  } model;

  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html><head><style>
  body { margin:0; }
  table { width:100%; height:400px; border:1px solid #c8d4df; }
</style></head><body>
  <table id="stress-table" data-virtual-table="true"></table>
</body></html>
)HTML");

  HtmlRenderStats stats{};
  HtmlLayoutCache layout_cache{};
  HtmlRenderConfig config{};
  config.draw_top_bar = false;
  config.root_padding_px = 0;
  config.virtual_table_model = &model;
  config.render_stats = &stats;
  config.layout_cache = &layout_cache;
  const auto first =
      build_html_document_scene(document, SizeI{900, 520}, config);
  expect(!first.commands().empty(), "virtual table should produce a scene");
  // 1000 columns, none narrower than the geometry's narrowest, and 5 million rows of the fixed row height.
  const auto geometry = resolve_table_geometry(stats.table_font_px);
  expect(stats.table_max_scroll_x > 1000 * geometry.min_column - 900 &&
             stats.table_max_scroll_y > 100000000,
         "large virtual table should expose complete two-axis scroll ranges");
  expect(model.cell_calls > 0 && model.cell_calls < 200,
         "initial virtual table frame should format only viewport cells");

  model.cell_calls = 0;
  model.min_cell_row = std::numeric_limits<std::int64_t>::max();
  config.table_scroll_x_px = 90000;
  config.table_scroll_y_px = 100000000;
  const auto scrolled =
      build_html_document_scene(document, SizeI{900, 520}, config);
  expect(!scrolled.commands().empty(),
         "far virtual table scroll should produce a scene");
  expect(model.cell_calls > 0 && model.cell_calls < 200,
         "far virtual table frame should remain bounded by viewport cells");
  expect(model.min_cell_row > 2000000,
         "vertical virtualization should fetch rows near the requested offset");
  expect(std::any_of(
             scrolled.commands().begin(), scrolled.commands().end(),
             [](const auto& command) {
               const auto* text = std::get_if<TextCommand>(&command);
               return text != nullptr &&
                      text->text.rfind("feature_", 0) == 0;
             }),
         "cached virtual geometry should retain visible header labels");

  const std::vector<std::int64_t> filtered_rows{4999999, 42};
  model.cell_calls = 0;
  model.max_cell_row = 0;
  config.table_scroll_x_px = 0;
  config.table_scroll_y_px = 0;
  config.virtual_row_order = &filtered_rows;
  (void)build_html_document_scene(document, SizeI{900, 520}, config);
  expect(model.cell_calls > 0 && model.max_cell_row == 4999999,
         "virtual search results should map display rows back to source rows");
}

void test_svg_styles_override_presentation_size() {
  using namespace pluss::view::window;

  const auto document = parse_html_document(R"HTML(
<!doctype html>
<html><head><style>
  .frame { width:100%; overflow:auto; }
  svg { display:block; width:100%; height:auto; min-height:320px; }
</style></head><body>
  <div class="frame">
    <svg width="2292" height="2292" viewBox="0 0 2292 2292">
      <rect x="0" y="0" width="2292" height="2292" fill="#ffffff" />
      <text x="1146" y="80" text-anchor="middle">Centered plot</text>
    </svg>
  </div>
</body></html>
)HTML");

  HtmlRenderStats stats{};
  HtmlRenderConfig config{};
  config.render_stats = &stats;
  const auto scene = build_html_document_scene(document, SizeI{760, 520}, config);
  expect(stats.document_max_scroll_x == 0,
         "CSS width should override an SVG presentation width without document overflow");
  const auto title = std::find_if(scene.commands().begin(), scene.commands().end(), [](const auto& command) {
    const auto* text = std::get_if<TextCommand>(&command);
    return text != nullptr && text->text == "Centered plot";
  });
  expect(title != scene.commands().end(), "fitted SVG title should render");
  const auto* text = std::get_if<TextCommand>(&*title);
  expect(text != nullptr && text->anchor.x >= 0 && text->anchor.x < 760,
         "fitted SVG title should remain inside the viewport");
}

} // namespace

int main() {
  try {
    test_automatic_table_identity();
    test_layout_metrics_and_scroll_model();
    test_retina_scene_scaling();
    test_form_control_alignment_and_caret();
    test_table_body_values_share_left_edge();
    test_sticky_case_column_expands_for_text_labels();
    test_virtual_table_only_materializes_viewport_cells();
    test_virtual_table_colours_follow_the_stylesheet();
    test_virtual_table_last_column_takes_the_slack();
    test_virtual_table_columns_can_be_resized();
    test_virtual_table_double_click_fits_a_squeezed_column();
    test_no_matching_rows_is_centred();
    test_table_ends_at_its_last_row();
    test_sizing_a_column_moves_only_its_right_edge();
    test_virtual_table_sorts_by_a_column();
    test_row_window_bounds();
    test_long_and_short_tables_draw_headers_alike();
    test_svg_styles_override_presentation_size();
    std::cout << "[PASS] automatic_table_identity\n";
    std::cout << "[PASS] html_layout_metrics_and_scroll_model\n";
    std::cout << "[PASS] retina_scene_scaling\n";
    std::cout << "[PASS] form_control_alignment_and_caret\n";
    std::cout << "[PASS] table_body_values_share_left_edge\n";
    std::cout << "[PASS] sticky_case_column_expands_for_text_labels\n";
    std::cout << "[PASS] virtual_table_only_materializes_viewport_cells\n";
    std::cout << "[PASS] virtual_table_colours_follow_the_stylesheet\n";
    std::cout << "[PASS] virtual_table_last_column_takes_the_slack\n";
    std::cout << "[PASS] svg_styles_override_presentation_size\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }
}
