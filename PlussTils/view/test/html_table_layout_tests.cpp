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

void test_resizing_changes_only_that_column() {
  // Like a spreadsheet: the column you size changes alone, and every other column keeps its width.
  const std::string html = table_html(5, 3);
  const SizeI viewport{1400, 600};
  const auto initial = render(html, viewport);
  const auto geo = resolve_table_geometry(initial.stats.table_font_px);
  for (const int column : {0, 2, 4}) {
    for (const int target : {geo.min_resize * 2, initial.widths[static_cast<std::size_t>(column)] + 60}) {
      std::unordered_map<std::string, std::unordered_map<int, int>> widths;
      widths["grid"][column] = target;
      HtmlRenderConfig config{};
      config.table_column_widths = &widths;
      const auto resized = render(html, viewport, config);
      // A column scrolled out of view has no handle to read, so only the columns in view are compared.
      for (std::size_t c = 0; c < std::min(initial.widths.size(), resized.widths.size()); ++c) {
        if (resized.widths[c] <= 0) {
          continue;
        }
        const int expected = static_cast<int>(c) == column ? target : initial.widths[c];
        expect(resized.widths[c] == expected, "only column " + std::to_string(column) + " changes (column " +
                                                  std::to_string(c) + " is " + std::to_string(resized.widths[c]) +
                                                  ", expected " + std::to_string(expected) + ")");
      }
    }
  }
}

void test_a_narrow_table_ends_in_a_continuation_not_a_hole() {
  const std::string html = table_html(3, 3);
  const SizeI viewport{1400, 600};
  const auto initial = render(html, viewport);
  const auto geo = resolve_table_geometry(initial.stats.table_font_px);
  std::unordered_map<std::string, std::unordered_map<int, int>> widths;
  for (int column = 0; column < 3; ++column) {
    widths["grid"][column] = geo.min_resize * 2;  // every column narrow: the columns end well short of the panel
  }
  HtmlRenderConfig config{};
  config.table_column_widths = &widths;
  const auto narrow = render(html, viewport, config);
  expect(sum(narrow.widths) < initial.stats.table_viewport_rect.width, "the columns end short of the panel");
  expect(narrow.stats.table_viewport_rect.width == initial.stats.table_viewport_rect.width,
         "the panel keeps its width");
  // The header band spans the whole panel, with the grid carrying on past the last column.
  const auto document = parse_html_document(html);
  HtmlRenderStats stats;
  config.render_stats = &stats;
  const auto scene = build_html_document_scene(document, viewport, config);
  const int panel_right = stats.table_viewport_rect.x + stats.table_viewport_rect.width;
  int widest_fill = 0;
  for (const auto& command : scene.commands()) {
    RectI rect{};
    bool is_fill = false;
    if (const auto* fill = std::get_if<FillRectCommand>(&command)) {
      rect = fill->rect;
      is_fill = true;
    } else if (const auto* rounded = std::get_if<FillRoundedRectCommand>(&command)) {
      rect = rounded->rect;
      is_fill = true;
    }
    if (is_fill && rect.y >= stats.table_viewport_rect.y && rect.y < stats.table_viewport_rect.y + 2) {
      widest_fill = std::max(widest_fill, rect.x + rect.width);
    }
  }
  expect(widest_fill >= panel_right - 2, "the header band runs to the panel's right edge");
  // Widening past the panel scrolls instead.
  widths["grid"][1] = initial.stats.table_viewport_rect.width;
  const auto wide = render(html, viewport, config);
  expect(wide.stats.table_max_scroll_x > 0, "a table wider than its panel scrolls sideways");
}

void test_double_click_fit_and_restore() {
  const std::string html = table_html(3, 3);
  const SizeI viewport{1400, 600};
  const auto initial = render(html, viewport);
  std::unordered_map<std::string, std::unordered_map<int, int>> widths;
  HtmlRenderConfig config{};
  config.table_column_widths = &widths;
  widths["grid"][1] = 0;  // auto-fit
  const auto fitted = render(html, viewport, config);
  expect(fitted.widths[0] == initial.widths[0] && fitted.widths[2] == initial.widths[2],
         "auto-fitting a column leaves the others as they were");
  widths["grid"].clear();  // fit again: back to the flexible width
  const auto restored = render(html, viewport, config);
  expect(restored.widths == initial.widths, "removing the override restores the original widths");
}


void test_header_never_gets_taller_when_a_column_shrinks() {
  const std::string html = table_html(5, 3);
  const SizeI viewport{1400, 600};
  const auto header_height = [](const Rendered& r) {
    for (const auto& hit : r.hits) {
      if (hit.action == "table.sort.column") {
        return hit.rect.height;
      }
    }
    return -1;
  };
  const auto initial = render(html, viewport);
  const int base = header_height(initial);
  expect(base > 0, "the header row has a height");
  const auto geo = resolve_table_geometry(initial.stats.table_font_px);
  for (const int target : {geo.min_resize, geo.min_resize * 2, geo.squeeze_floor}) {
    std::unordered_map<std::string, std::unordered_map<int, int>> widths;
    widths["grid"][2] = target;
    HtmlRenderConfig config{};
    config.table_column_widths = &widths;
    expect(header_height(render(html, viewport, config)) == base,
           "a header never wraps to a second line when its column is " + std::to_string(target) + " px");
  }
  // And a table squeezed by sheer column count keeps one-line headers too.
  const auto many = render(table_html(30, 3), viewport);
  expect(header_height(many) == base, "a squeezed table has one-line headers");
}


[[nodiscard]] std::vector<std::string> drawn_texts(const std::string& html, const SizeI viewport,
                                                  HtmlRenderConfig config = {}) {
  const auto document = parse_html_document(html);
  const auto scene = build_html_document_scene(document, viewport, config);
  std::vector<std::string> texts;
  for (const auto& command : scene.commands()) {
    if (const auto* text = std::get_if<TextCommand>(&command)) {
      texts.push_back(text->text);
    }
  }
  return texts;
}

void test_resizing_a_column_never_changes_a_row() {
  // Text does not wrap by default, so a row's height follows the font and not the column width.
  std::string html = table_html(4, 5);
  const SizeI viewport{1400, 600};
  const auto initial = render(html, viewport);
  std::unordered_map<std::string, std::unordered_map<int, int>> widths;
  widths["grid"][1] = resolve_table_geometry(initial.stats.table_font_px).min_resize;
  HtmlRenderConfig config{};
  config.table_column_widths = &widths;
  const auto narrow = render(html, viewport, config);
  expect(narrow.stats.table_content_rect.height == initial.stats.table_content_rect.height,
         "narrowing a column leaves every row the same height");
}

void test_a_number_that_does_not_fit_is_marked_and_never_cut() {
  const std::string html =
      "<html><head><style>table{width:100%;max-height:28.6em;} th,td{padding:8px;}</style></head><body><table "
      "id=\"grid\"><thead><tr><th>name</th><th>amount</th></tr></thead><tbody><tr><td>a long text value that will "
      "not fit</td><td>123456789.123456</td></tr></tbody></table></body></html>";
  const SizeI viewport{900, 400};
  const auto initial = render(html, viewport);
  const int min_resize = resolve_table_geometry(initial.stats.table_font_px).min_resize;
  std::unordered_map<std::string, std::unordered_map<int, int>> widths;
  widths["grid"][1] = min_resize + min_resize / 2;  // only the number column is sized; the text column fills
  HtmlRenderConfig config{};
  config.table_column_widths = &widths;
  const auto texts = drawn_texts(html, viewport, config);
  const std::string marker = html_table_geometry().numeric_overflow_marker;
  const bool number_marked = std::any_of(texts.begin(), texts.end(), [&](const std::string& t) {
    return !t.empty() && t.find_first_not_of(marker) == std::string::npos;
  });
  const bool number_cut = std::any_of(texts.begin(), texts.end(), [](const std::string& t) {
    return t.find("1234") != std::string::npos && t != "123456789.123456";
  });
  std::string listing;
  for (const auto& t : texts) listing += "[" + t + "]";
  expect(number_marked, "a number that does not fit is shown as the overflow marker; drawn: " + listing);
  expect(!number_cut, "a number is never cut");
  // A text cell that does not fit ends in an ellipsis in a squeezed table.
  const auto many = drawn_texts(table_html(30, 2), viewport);
  expect(std::any_of(many.begin(), many.end(), [](const std::string& t) { return t.find("...") != std::string::npos; }),
         "text that does not fit ends in an ellipsis");
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
    test_resizing_changes_only_that_column();
    test_a_narrow_table_ends_in_a_continuation_not_a_hole();
    test_double_click_fit_and_restore();
    test_header_never_gets_taller_when_a_column_shrinks();
    test_resizing_a_column_never_changes_a_row();
    test_a_number_that_does_not_fit_is_marked_and_never_cut();
    test_pdf_export_writes_a_valid_file();
    std::cout << "[PASS] html_table_layout_tests\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }
}
