#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

#include "plot/modules/style.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {

struct ResolvedTheme {
  std::string name{"paper"};
  std::string page_background{"#f4efe6"};
  std::string panel_background{"#fffdf8"};
  std::string panel_border{"#ddd2c3"};
  std::string plot_background{"#fffaf2"};
  std::string plot_border{"#eee4d8"};
  std::string title_color{"#1f1a17"};
  std::string subplot_title_color{"#2a241f"};
  std::string axis_label_color{"#3e352d"};
  std::string grid_color{"#ece2d5"};
  std::string tick_color{"#8a7d6f"};
  std::string tick_label_color{"#6a5f54"};
  std::string axis_color{"#2f2924"};
  std::string legend_background{"#fffaf2"};
  std::string legend_border{"#d9cdbd"};
  std::string legend_title_color{"#2e261f"};
  std::string legend_text_color{"#4a4138"};
  std::string annotation_color{"#8a3b12"};
  std::string heatmap_cell_border{"#fffdf8"};
  std::string font_family{"Avenir Next, Helvetica Neue, Arial, sans-serif"};
  std::vector<std::string> palette{
      "#0f766e", "#c2410c", "#1d4ed8", "#b91c1c", "#4d7c0f",
      "#0369a1", "#b45309", "#0f172a", "#be123c", "#57534e"};
};

namespace detail {

inline std::string normalize_theme_name(std::string_view name) {
  std::string normalized;
  normalized.reserve(name.size());
  for (const unsigned char ch : std::string(name)) {
    if (std::isalnum(ch) != 0) {
      normalized.push_back(static_cast<char>(std::tolower(ch)));
    }
  }
  return normalized;
}

inline const ResolvedTheme& theme_preset(std::string_view requested_name) {
  static const ResolvedTheme paper{};
  static const ResolvedTheme slate{
      .name = "slate",
      .page_background = "#eef3f8",
      .panel_background = "#ffffff",
      .panel_border = "#d8e0ea",
      .plot_background = "#f8fbfe",
      .plot_border = "#e7edf5",
      .title_color = "#18212b",
      .subplot_title_color = "#223040",
      .axis_label_color = "#304256",
      .grid_color = "#dde6f0",
      .tick_color = "#6b7d90",
      .tick_label_color = "#5a6b7e",
      .axis_color = "#243242",
      .legend_background = "#fbfdff",
      .legend_border = "#d3dde8",
      .legend_title_color = "#223040",
      .legend_text_color = "#34475b",
      .annotation_color = "#9a3412",
      .heatmap_cell_border = "#f8fbfe",
      .font_family = "Avenir Next, Helvetica Neue, Arial, sans-serif",
      .palette = {"#1d4ed8", "#0f766e", "#b45309", "#b91c1c", "#0369a1",
                  "#4d7c0f", "#334155", "#be123c", "#0f172a", "#7c2d12"},
  };
  static const ResolvedTheme citrus{
      .name = "citrus",
      .page_background = "#fff7e8",
      .panel_background = "#fffdf7",
      .panel_border = "#ead7ba",
      .plot_background = "#fff9ef",
      .plot_border = "#f2e5d0",
      .title_color = "#312012",
      .subplot_title_color = "#4a2b12",
      .axis_label_color = "#5f3a1c",
      .grid_color = "#f1e1c8",
      .tick_color = "#9a6b33",
      .tick_label_color = "#7c5b35",
      .axis_color = "#4f3117",
      .legend_background = "#fffaf0",
      .legend_border = "#e9d7bc",
      .legend_title_color = "#4a2b12",
      .legend_text_color = "#694726",
      .annotation_color = "#b45309",
      .heatmap_cell_border = "#fff9ef",
      .font_family = "Avenir Next, Helvetica Neue, Arial, sans-serif",
      .palette = {"#d97706", "#0f766e", "#b91c1c", "#2563eb", "#65a30d",
                  "#c2410c", "#0f172a", "#be123c", "#0369a1", "#854d0e"},
  };

  const std::string normalized = normalize_theme_name(requested_name);
  if (normalized == "slate") {
    return slate;
  }
  if (normalized == "citrus") {
    return citrus;
  }
  return paper;
}

inline ResolvedTheme resolve_theme(const Style& style) {
  ResolvedTheme theme = theme_preset(style.theme);
  if (!style.font_family.empty()) {
    theme.font_family = style.font_family;
  }
  return theme;
}

inline std::string palette_color(const ResolvedTheme& theme, std::size_t index) {
  if (theme.palette.empty()) {
    return "#0f766e";
  }
  return theme.palette[index % theme.palette.size()];
}

inline std::string default_trace_color_2d(const Trace2D& trace, const ResolvedTheme& theme, std::size_t index) {
  if (trace.kind == Trace2DKind::Annotation) {
    return theme.annotation_color;
  }
  return palette_color(theme, index);
}

inline std::string default_trace_color_3d(const Trace3D&, const ResolvedTheme& theme, std::size_t index) {
  return palette_color(theme, index);
}

inline void apply_trace_style_defaults(Trace2D& trace, const ResolvedTheme& theme, std::size_t index) {
  if (trace.style.color.empty()) {
    trace.style.color = default_trace_color_2d(trace, theme, index);
  }
  if (trace.style.font_family.empty()) {
    trace.style.font_family = theme.font_family;
  }
}

inline void apply_trace_style_defaults(Trace3D& trace, const ResolvedTheme& theme, std::size_t index) {
  if (trace.style.color.empty()) {
    trace.style.color = default_trace_color_3d(trace, theme, index);
  }
  if (trace.style.font_family.empty()) {
    trace.style.font_family = theme.font_family;
  }
}

} // namespace detail
} // namespace pluss::plotism
