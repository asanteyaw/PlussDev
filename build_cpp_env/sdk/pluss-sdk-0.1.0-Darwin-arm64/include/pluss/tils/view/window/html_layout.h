#pragma once

#include "pluss/tils/view/window/html_document.h"
#include "pluss/tils/view/window/scene.h"

namespace pluss::view::window {

struct HtmlHitRegion {
  RectI rect{};
  std::string action{};
  std::string target{};
  int value{0};
};

struct HtmlRenderStats {
  RectI document_viewport_rect{};
  RectI table_viewport_rect{};
  int document_max_scroll_x{0};
  int document_max_scroll_y{0};
  int table_max_scroll_x{0};
  int table_max_scroll_y{0};
  int table_body_row_height_px{0};
  bool has_table{false};
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
  bool clip_table_viewport{true};
  int table_sort_column{-1};
  bool table_sort_ascending{true};
  std::string table_sort_target{};
  std::string table_filter_query{};
  std::string table_filter_target{};
  std::string active_input_target{};
  std::string active_input_value{};
  std::vector<HtmlHitRegion>* hit_regions{nullptr};
  HtmlRenderStats* render_stats{nullptr};
};

[[nodiscard]] Scene build_html_document_scene(const HtmlDocument& document,
                                              SizeI viewport,
                                              HtmlRenderConfig config = {});

} // namespace pluss::view::window
