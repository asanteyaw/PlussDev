#pragma once

#include <algorithm>
#include <cmath>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "pluss/tils/view/window/html_document.h"
#include "pluss/tils/view/window/scene.h"

namespace pluss::view {
class VirtualTableModel;
}

namespace pluss::view::window {

// What happens to the room a table's columns leave over (or lack) when the user sizes a column.
enum class HtmlTableSlack : unsigned char {
  Continuation = 0,  // the grid carries on past the last column as blank columns, as in a spreadsheet
  LastColumn = 1,    // (default) the last column is the table's automatic column: it stretches or shrinks to keep
                     // the table at the panel's width, and has no divider of its own
};

// Every size a table is laid out with, in em: multiples of the table's own font size, so the
// geometry scales with the theme and font instead of being tied to a pixel count. This is the one
// place to change a table's proportions; the layout, the drag limits and the exports all read it
// through html_table_geometry(). The defaults reproduce the previous look at a 14 px font.
struct HtmlTableGeometry {
  // Column widths.
  double min_column_em{5.0};        // narrowest a column starts out (short content)
  double max_column_em{17.0};       // widest a column starts out before its text wraps
  double squeeze_floor_em{10.0};     // narrowest columns are squeezed to when there are many
  double min_resize_em{3.5};        // narrowest a user can drag a column to
  // Past the last column the grid carries on as blank columns this wide (a spreadsheet's empty cells),
  // so a table narrower than its panel never ends in a hole.
  double continuation_column_em{8.0};
  HtmlTableSlack slack{HtmlTableSlack::Continuation};
  double max_autofit_viewports{2.0};  // auto-fit never exceeds this many viewport widths
  std::size_t fit_sample_rows{48};  // rows read to size a column that has not been resized
  // Lines of text a cell may use before it ends in an ellipsis. Like a spreadsheet, text does not wrap
  // unless asked to: one line per cell, so a row's height follows the font and not the column's width,
  // and resizing a column never changes a row. Raise value_max_lines (or label_max_lines) to let
  // long text wrap; headers stay on one line either way, so shrinking a column never makes the
  // header row taller.
  int header_max_lines{1};
  int value_max_lines{1};
  int label_max_lines{1};  // row-label cells
  // A number too wide for its column is never cut (a cut number reads as a different number): it is
  // shown as this marker repeated across the cell, the way a spreadsheet shows "####".
  std::string numeric_overflow_marker{"#"};
  // Sort arrows: a stacked pair of triangles at the right of a header.
  double sort_control_em{0.8};      // width of the control
  double sort_pad_em{0.43};         // air between a header's text and the control
  double sort_triangle_height_em{0.42};
  double sort_triangle_base_em{0.6};
  double sort_triangle_gap_em{0.14};  // between the two triangles (0 = they touch)
  // Virtual (very large) tables, whose rows are fixed height.
  double header_height_em{3.0};
  double row_height_em{2.7};
  double row_label_width_em{6.0};
  double text_column_min_em{10.7};
  double numeric_column_min_em{9.4};
  double column_max_em{20.0};
  double max_viewport_height_em{28.6};  // tallest a virtual table's viewport is, absent a CSS height
};

// The geometry of one table resolved to pixels for a given font size.
struct HtmlTableGeometryPx {
  int min_column{0};
  int max_column{0};
  int squeeze_floor{0};
  int min_resize{0};
  int continuation_column{0};
  HtmlTableSlack slack{HtmlTableSlack::Continuation};
  std::size_t fit_sample_rows{0};
  int header_max_lines{1};
  int value_max_lines{1};
  int label_max_lines{1};
  int sort_control{0};
  int sort_pad{0};
  int sort_gutter{0};  // what a header's text leaves for the control: control + pad
  int sort_triangle_height{0};
  int sort_triangle_base{0};
  int sort_triangle_gap{0};
  int header_height{0};
  int row_height{0};
  int row_label_width{0};
  int text_column_min{0};
  int numeric_column_min{0};
  int column_max{0};
  int max_viewport_height{0};
  double max_autofit_viewports{0.0};
  std::string numeric_overflow_marker{};
};

[[nodiscard]] inline HtmlTableGeometry& html_table_geometry() {
  static HtmlTableGeometry geometry{};
  return geometry;
}

[[nodiscard]] inline HtmlTableGeometryPx resolve_table_geometry(const int font_px,
                                                                const HtmlTableGeometry& g = html_table_geometry()) {
  const auto px = [font_px](const double em) {
    return std::max(1, static_cast<int>(std::lround(em * static_cast<double>(std::max(1, font_px)))));
  };
  HtmlTableGeometryPx out;
  out.min_column = px(g.min_column_em);
  out.max_column = px(g.max_column_em);
  out.squeeze_floor = px(g.squeeze_floor_em);
  out.min_resize = px(g.min_resize_em);
  out.continuation_column = px(g.continuation_column_em);
  out.slack = g.slack;
  out.fit_sample_rows = g.fit_sample_rows;
  out.header_max_lines = std::max(1, g.header_max_lines);
  out.value_max_lines = std::max(1, g.value_max_lines);
  out.label_max_lines = std::max(1, g.label_max_lines);
  out.sort_control = px(g.sort_control_em);
  out.sort_pad = px(g.sort_pad_em);
  out.sort_gutter = out.sort_control + out.sort_pad;
  out.sort_triangle_height = px(g.sort_triangle_height_em);
  out.sort_triangle_base = px(g.sort_triangle_base_em);
  out.sort_triangle_gap = g.sort_triangle_gap_em <= 0.0 ? 0 : px(g.sort_triangle_gap_em);
  out.header_height = px(g.header_height_em);
  out.row_height = px(g.row_height_em);
  out.row_label_width = px(g.row_label_width_em);
  out.text_column_min = px(g.text_column_min_em);
  out.numeric_column_min = px(g.numeric_column_min_em);
  out.column_max = px(g.column_max_em);
  out.max_viewport_height = px(g.max_viewport_height_em);
  out.max_autofit_viewports = g.max_autofit_viewports;
  out.numeric_overflow_marker = g.numeric_overflow_marker;
  return out;
}

struct HtmlHitRegion {
  RectI rect{};
  std::string action{};
  std::string target{};
  int value{0};
  int aux{0};  // action-specific extra (the current column width for resize handles)
};

struct HtmlScrollRegion {
  std::string target{};
  RectI viewport_rect{};
  int max_scroll_x{0};
  int max_scroll_y{0};
  RectI horizontal_track_rect{};
  RectI horizontal_thumb_rect{};
  RectI vertical_track_rect{};
  RectI vertical_thumb_rect{};
};

struct HtmlRenderStats {
  RectI document_viewport_rect{};
  RectI table_viewport_rect{};
  // The part of the table that is actually drawn: the column headers plus the visible rows,
  // as wide as the visible columns. Never larger than table_viewport_rect.
  RectI table_content_rect{};
  int table_font_px{0};  // font size of the table that table_content_rect describes
  int document_max_scroll_x{0};
  int document_max_scroll_y{0};
  int table_max_scroll_x{0};
  int table_max_scroll_y{0};
  int table_body_row_height_px{0};
  RectI document_horizontal_track_rect{};
  RectI document_horizontal_thumb_rect{};
  RectI document_vertical_track_rect{};
  RectI document_vertical_thumb_rect{};
  std::vector<HtmlScrollRegion> table_scroll_regions{};
  bool has_table{false};
};

struct HtmlTableGeometryCacheEntry {
  std::uintptr_t node_identity{0};
  int viewport_width{0};
  int content_width{0};
  std::size_t row_count{0};
  std::size_t column_count{0};
  std::vector<std::size_t> source_indices{};
  std::vector<int> column_widths{};
  std::vector<int> row_heights{};
  std::uint64_t column_override_signature{0};  // user-set widths this entry was laid out with
  // What a column resize reuses, so dragging a border re-measures only the column that moved:
  // the widths before any user override, and the height each cell needs (row-major, one entry
  // per row and visible column). A row's height is the largest of its cells.
  std::vector<int> base_widths{};
  std::vector<bool> base_single_line{};
  std::vector<bool> column_single_line{};  // after the user's widths are applied
  std::shared_ptr<void> measure_state{};   // per-cell measurements a column resize reuses
  std::vector<int> cell_heights{};
};

struct HtmlLayoutCache {
  std::unordered_map<std::string, HtmlTableGeometryCacheEntry> tables{};

  void clear() noexcept {
    tables.clear();
  }
};

struct HtmlRenderConfig {
  int root_padding_px{18};
  int top_bar_height_px{54};
  bool draw_top_bar{true};
  int document_scroll_x_px{0};
  int document_scroll_y_px{0};
  bool clip_document_viewport{true};
  bool enable_document_scroll{true};
  int table_scroll_x_px{0};
  int table_scroll_y_px{0};
  const std::unordered_map<std::string, PointI>* table_scroll_offsets{nullptr};
  bool clip_table_viewport{true};
  bool draw_scrollbars{true};
  bool table_sort_controls{true};  // sort arrows on column headers (off for exported images)
  double widget_scale{1.0};
  int table_sort_column{-1};
  bool table_sort_ascending{true};
  std::string table_sort_target{};
  // User-set column widths (px) per table target, by column index; the viewer fills it as
  // the user drags column borders. Columns without an entry use the default width.
  const std::unordered_map<std::string, std::unordered_map<int, int>>* table_column_widths{nullptr};
  // The column border under the pointer (its header border is drawn highlighted).
  std::string hover_resize_target{};
  int hover_resize_column{-1};
  std::string table_filter_query{};
  std::string table_filter_target{};
  std::string active_input_target{};
  std::string active_input_value{};
  bool show_input_caret{true};
  const VirtualTableModel* virtual_table_model{nullptr};
  const std::vector<std::int64_t>* virtual_row_order{nullptr};
  std::vector<HtmlHitRegion>* hit_regions{nullptr};
  HtmlRenderStats* render_stats{nullptr};
  HtmlLayoutCache* layout_cache{nullptr};
};

[[nodiscard]] Scene build_html_document_scene(const HtmlDocument& document,
                                              SizeI viewport,
                                              HtmlRenderConfig config = {});

} // namespace pluss::view::window
