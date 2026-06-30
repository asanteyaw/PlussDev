#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "pluss/tils/view/window.h"

namespace {

using pluss::view::window::HtmlRenderConfig;
using pluss::view::window::HtmlRenderStats;
using pluss::view::window::HtmlScrollModel;
using pluss::view::window::PointI;
using pluss::view::window::RectI;
using pluss::view::window::SizeI;

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
  expect(save_canvas(runtime.canvas(), "/tmp/plussview_scroll_initial.png"),
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
  expect(save_canvas(runtime.canvas(), "/tmp/plussview_scroll_scrolled.png"),
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
  expect(save_canvas(runtime.canvas(), "/tmp/plussview_scroll_document.png"),
         "document-scrolled validation frame should save");
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
    border:1px solid #b9c7d5; text-align:center;
  }
  .tools input { min-width:220px; font-size:13px; }
  .tools button { min-width:88px; font-size:13px; }
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
    const auto* line = std::get_if<LineCommand>(&command);
    return line != nullptr && line->color.r == 71 && line->color.g == 118 && line->color.b == 184;
  });
  expect(caret != scene.commands().end(), "focused search input should render a caret");
  const auto* caret_line = std::get_if<LineCommand>(&*caret);
  expect(caret_line != nullptr &&
             std::abs(caret_line->start.x - (input->rect.x + input->rect.width / 2)) <= 1 &&
             caret_line->end.y - caret_line->start.y <= 20,
         "an empty centered search input should use a short centered caret");
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
    test_svg_styles_override_presentation_size();
    std::cout << "[PASS] automatic_table_identity\n";
    std::cout << "[PASS] html_layout_metrics_and_scroll_model\n";
    std::cout << "[PASS] retina_scene_scaling\n";
    std::cout << "[PASS] form_control_alignment_and_caret\n";
    std::cout << "[PASS] table_body_values_share_left_edge\n";
    std::cout << "[PASS] svg_styles_override_presentation_size\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }
}
