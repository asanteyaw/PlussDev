// Layout checks for tables: geometry that scales with the font, columns that fill or squeeze to
// the width, tables that are only as tall as their rows, exports without sort arrows, resizing
// that re-measures only the changed column, and the portable PDF writer. None of it needs a
// window, so it runs the same on every platform.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "pluss/tils/view/html_window.h"
#include "pluss/tils/view/window.h"

namespace {

using namespace pluss::view::window;

[[noreturn]] void fail(const std::string& message) { throw std::runtime_error(message); }

void expect(const bool condition, const std::string& message) {
  if (!condition) {
    fail(message);
  }
}

[[nodiscard]] std::string table_html(const int columns, const int rows) {
  std::string html =
      "<html><head><style>table{width:100%;max-height:28.6em;margin:0;} th,td{padding:8px;}"
      "</style></head><body><div><table id=\"grid\"><thead><tr>";
  for (int c = 0; c < columns; ++c) {
    html += "<th>measurement_" + std::to_string(c) + "</th>";
  }
  html += "</tr></thead><tbody>";
  for (int r = 0; r < rows; ++r) {
    html += "<tr>";
    for (int c = 0; c < columns; ++c) {
      html += "<td>value " + std::to_string(r) + "-" + std::to_string(c) + "</td>";
    }
    html += "</tr>";
  }
  html += "</tbody></table></div></body></html>";
  return html;
}

struct Rendered {
  HtmlRenderStats stats{};
  std::vector<HtmlHitRegion> hits{};
  std::vector<int> widths{};  // current width of each column, from its resize handle
};

[[nodiscard]] Rendered render(const std::string& html, const SizeI viewport, HtmlRenderConfig config = {},
                              HtmlLayoutCache* cache = nullptr) {
  Rendered out;
  const auto document = parse_html_document(html);
  config.render_stats = &out.stats;
  config.hit_regions = &out.hits;
  config.layout_cache = cache;
  (void)build_html_document_scene(document, viewport, config);
  for (const auto& hit : out.hits) {
    if (hit.action == "table.resize.column") {
      if (static_cast<std::size_t>(hit.value) >= out.widths.size()) {
        out.widths.resize(static_cast<std::size_t>(hit.value) + 1U, 0);
      }
      out.widths[static_cast<std::size_t>(hit.value)] = hit.aux;
    }
  }
  return out;
}

[[nodiscard]] int sum(const std::vector<int>& values) {
  int total = 0;
  for (const int v : values) total += v;
  return total;
}

void test_geometry_scales_with_font() {
  const auto small = resolve_table_geometry(14);
  const auto large = resolve_table_geometry(28);
  expect(large.min_column == small.min_column * 2 && large.squeeze_floor == small.squeeze_floor * 2,
         "column limits should scale with the font size");
  expect(large.sort_gutter == large.sort_control + large.sort_pad, "the arrow gutter is the control plus its air");
  expect(small.squeeze_floor > small.min_column && small.max_column > small.squeeze_floor,
         "min < squeeze floor < max for the default geometry");
}

void test_few_columns_fill_the_width() {
  const auto result = render(table_html(3, 3), SizeI{1000, 600});
  expect(result.stats.has_table, "the page has a table");
  expect(result.widths.size() == 3, "three column handles");
  const int total = sum(result.widths);
  expect(std::abs(total - result.stats.table_viewport_rect.width) <= 3,
         "three columns should fill the table width (got " + std::to_string(total) + " of " +
             std::to_string(result.stats.table_viewport_rect.width) + ")");
  expect(result.stats.table_content_rect.width >= result.stats.table_viewport_rect.width - 3,
         "the drawn content spans the table width");
}

void test_short_table_is_only_as_tall_as_its_rows() {
  const auto result = render(table_html(3, 3), SizeI{1000, 600});
  const auto& viewport = result.stats.table_viewport_rect;
  const auto& content = result.stats.table_content_rect;
  expect(content.height > 0 && content.height <= viewport.height, "content fits its viewport");
  expect(viewport.height < resolve_table_geometry(result.stats.table_font_px).max_viewport_height + 400,
         "a short table must not be given a tall viewport");
  const auto tall = render(table_html(3, 400), SizeI{1000, 600});
  expect(tall.stats.table_viewport_rect.height > viewport.height,
         "a long table gets a taller viewport than a short one");
}

void test_many_columns_squeeze_then_scroll() {
  const auto result = render(table_html(30, 3), SizeI{1000, 600});
  const int font = result.stats.table_font_px;
  expect(font > 0, "the table reports its font size");
  const auto geo = resolve_table_geometry(font);
  int visible = 0;
  for (const int width : result.widths) {
    if (width <= 0) {
      continue;  // a column scrolled out of view has no handle
    }
    ++visible;
    expect(width >= geo.squeeze_floor, "no column is squeezed below the floor (" + std::to_string(width) + " < " +
                                           std::to_string(geo.squeeze_floor) + ")");
  }
  expect(visible >= 2 && visible < 30, "some, but not all, of thirty columns are in view");
  expect(result.stats.table_max_scroll_x > 0, "thirty columns should scroll sideways");
}

void test_export_leaves_out_sort_arrows() {
  HtmlRenderConfig with_arrows{};
  const auto shown = render(table_html(3, 3), SizeI{1000, 600}, with_arrows);
  const auto has_sort = [](const Rendered& r) {
    return std::any_of(r.hits.begin(), r.hits.end(), [](const HtmlHitRegion& h) {
      return h.action == "table.sort.asc" || h.action == "table.sort.desc";
    });
  };
  expect(has_sort(shown), "the window shows sort arrows");
  HtmlRenderConfig export_config{};
  export_config.table_sort_controls = false;
  const auto exported = render(table_html(3, 3), SizeI{1000, 600}, export_config);
  expect(!has_sort(exported), "an export has no sort arrows");
}

void test_resize_matches_a_fresh_layout() {
  std::unordered_map<std::string, std::unordered_map<int, int>> widths;
  widths["grid"][1] = 260;
  HtmlRenderConfig config{};
  config.table_column_widths = &widths;

  HtmlLayoutCache cache;
  const std::string html = table_html(6, 40);
  const SizeI viewport{1000, 600};
  (void)render(html, viewport, HtmlRenderConfig{}, &cache);        // fills the cache
  const auto incremental = render(html, viewport, config, &cache);  // re-measures one column
  const auto fresh = render(html, viewport, config, nullptr);       // no cache at all
  expect(incremental.widths == fresh.widths, "an incremental resize gives the same widths as a fresh layout");
  expect(incremental.widths.size() > 1 && incremental.widths[1] == 260, "the dragged column takes its new width");
  expect(incremental.stats.table_content_rect.height == fresh.stats.table_content_rect.height,
         "an incremental resize gives the same height as a fresh layout");
}

void test_user_sized_columns_leave_no_gap() {
  const std::string html = table_html(3, 3);
  const SizeI viewport{1000, 600};
  const auto initial = render(html, viewport);
  const int fill = sum(initial.widths);
  // Narrowing one column hands its space to the others, so the table still fills the width.
  std::unordered_map<std::string, std::unordered_map<int, int>> widths;
  widths["grid"][1] = resolve_table_geometry(initial.stats.table_font_px).min_resize * 2;
  HtmlRenderConfig config{};
  config.table_column_widths = &widths;
  const auto narrowed = render(html, viewport, config);
  expect(narrowed.widths.size() == 3 && narrowed.widths[1] == widths["grid"][1], "the sized column keeps its width");
  expect(std::abs(sum(narrowed.widths) - fill) <= 3,
         "the other columns grow into the freed space (" + std::to_string(sum(narrowed.widths)) + " vs " +
             std::to_string(fill) + ")");
  // Auto-fit and then back (the override is removed): the original widths return.
  widths["grid"].clear();
  const auto restored = render(html, viewport, config);
  expect(restored.widths == initial.widths, "removing the override restores the original widths");
}

void test_other_columns_absorb_a_resize_in_a_wide_table() {
  // With many columns, resizing one must keep the table the width it was: the columns that were
  // not touched share the difference, as long as they stay above the squeeze floor.
  const std::string html = table_html(5, 3);
  const SizeI viewport{1400, 600};
  const auto initial = render(html, viewport);
  const int total = sum(initial.widths);
  const auto geo = resolve_table_geometry(initial.stats.table_font_px);
  for (const int column : {0, 2, 4}) {
    for (const int target : {geo.min_resize * 2, initial.widths[static_cast<std::size_t>(column)] + 60}) {
      std::unordered_map<std::string, std::unordered_map<int, int>> widths;
      widths["grid"][column] = target;
      HtmlRenderConfig config{};
      config.table_column_widths = &widths;
      const auto resized = render(html, viewport, config);
      expect(resized.widths[static_cast<std::size_t>(column)] == target, "the resized column keeps its width");
      expect(std::abs(sum(resized.widths) - total) <= 5,
             "the table keeps its width when column " + std::to_string(column) + " becomes " + std::to_string(target) +
                 " (" + std::to_string(sum(resized.widths)) + " vs " + std::to_string(total) + ")");
    }
  }
  // Two columns sized by the user: the other three still make up the difference.
  std::unordered_map<std::string, std::unordered_map<int, int>> widths;
  widths["grid"][1] = geo.min_resize * 3;
  widths["grid"][3] = geo.min_resize * 2;
  HtmlRenderConfig config{};
  config.table_column_widths = &widths;
  const auto two = render(html, viewport, config);
  expect(std::abs(sum(two.widths) - total) <= 5, "two sized columns still leave the table at its width");
}

void test_pdf_export_writes_a_valid_file() {
  PixelCanvas canvas(SizeI{64, 40});
  canvas.clear(Color::rgba(255, 255, 255, 255));
  canvas.fill_rect(RectI{4, 4, 20, 10}, Color::rgba(200, 30, 30, 255));
  const auto path = (std::filesystem::temp_directory_path() / "plussview_layout_test.pdf").string();
  expect(save_canvas_pdf(canvas, path), "the PDF should be written");
  std::ifstream file(path, std::ios::binary);
  const std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  std::remove(path.c_str());
  expect(data.rfind("%PDF-1.4", 0) == 0, "a PDF starts with its header");
  expect(data.find("/Width 64") != std::string::npos && data.find("/Height 40") != std::string::npos,
         "the image has the canvas size");
  expect(data.find("%%EOF") != std::string::npos, "a PDF ends with %%EOF");
}

}  // namespace

int main() {
  try {
    test_geometry_scales_with_font();
    test_few_columns_fill_the_width();
    test_short_table_is_only_as_tall_as_its_rows();
    test_many_columns_squeeze_then_scroll();
    test_export_leaves_out_sort_arrows();
    test_resize_matches_a_fresh_layout();
    test_user_sized_columns_leave_no_gap();
    test_other_columns_absorb_a_resize_in_a_wide_table();
    test_pdf_export_writes_a_valid_file();
    std::cout << "[PASS] html_table_layout_tests\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }
}
