#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "pluss/view/window/html_document.h"
#include "pluss/view/window/scene.h"

namespace pluss::view::window {

struct HtmlLinePlotPayload {
  std::string title{"Line Plot"};
  std::string x_label{"x"};
  std::string y_label{"y"};
  std::vector<double> x{};
  std::vector<double> y{};
};

struct HtmlTablePayload {
  std::string title{"Table"};
  std::vector<std::string> headers{};
  std::vector<std::vector<std::string>> rows{};
  std::size_t row_offset{0};
};

[[nodiscard]] HtmlDocument map_line_plot_payload_to_html_document(const HtmlLinePlotPayload& payload);
[[nodiscard]] HtmlDocument map_table_payload_to_html_document(const HtmlTablePayload& payload);

// Phase-1 bridge stubs for existing runtime sessions where only draw-scene snapshots are available.
[[nodiscard]] HtmlDocument map_line_plot_scene_to_html_document(const Scene& scene, std::string title);
[[nodiscard]] HtmlDocument map_table_scene_to_html_document(const Scene& scene, std::string title);

} // namespace pluss::view::window
