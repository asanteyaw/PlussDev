#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "pluss/tils/view/window/html_document.h"
#include "pluss/tils/view/window/scene.h"

namespace pluss::view::window {

struct HtmlHitRegion {
  RectI rect{};
  std::string action{};
  std::string target{};
  int value{0};
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
  int table_scroll_x_px{0};
  int table_scroll_y_px{0};
  const std::unordered_map<std::string, PointI>* table_scroll_offsets{nullptr};
  bool clip_table_viewport{true};
  bool draw_scrollbars{true};
  double widget_scale{1.0};
  int table_sort_column{-1};
  bool table_sort_ascending{true};
  std::string table_sort_target{};
  std::string table_filter_query{};
  std::string table_filter_target{};
  std::string active_input_target{};
  std::string active_input_value{};
  bool show_input_caret{true};
  std::vector<HtmlHitRegion>* hit_regions{nullptr};
  HtmlRenderStats* render_stats{nullptr};
  HtmlLayoutCache* layout_cache{nullptr};
};

[[nodiscard]] Scene build_html_document_scene(const HtmlDocument& document,
                                              SizeI viewport,
                                              HtmlRenderConfig config = {});

} // namespace pluss::view::window
