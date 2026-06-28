#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "pluss/tils/view/window/html_layout.h"

namespace pluss::view::window {

struct HtmlScrollSnapshot {
  PointI document{};
  std::unordered_map<std::string, PointI> tables{};
};

class HtmlScrollModel {
 public:
  HtmlScrollModel() = default;

  void update_metrics(const HtmlRenderStats& metrics);
  void reset() noexcept;

  [[nodiscard]] bool scroll_wheel(double delta_x,
                                  double delta_y,
                                  PointI location,
                                  bool precise_delta,
                                  bool shift_modifier);
  [[nodiscard]] bool pointer_down(PointI location);
  [[nodiscard]] bool pointer_drag(double delta_x, double delta_y, PointI location);
  void pointer_up() noexcept;
  [[nodiscard]] bool command(std::string_view command);

  [[nodiscard]] PointI document_offset_px() const noexcept;
  [[nodiscard]] std::unordered_map<std::string, PointI> table_offsets_px() const;
  [[nodiscard]] HtmlScrollSnapshot snapshot() const;

 private:
  struct Offset {
    double x{0.0};
    double y{0.0};
  };

  enum class DragSurface {
    None,
    Document,
    Table,
  };

  enum class DragAxis {
    Horizontal,
    Vertical,
  };

  struct ScrollbarDrag {
    DragSurface surface{DragSurface::None};
    DragAxis axis{DragAxis::Vertical};
    std::string target{};
    RectI track{};
    RectI thumb{};
  };

  HtmlRenderStats metrics_{};
  Offset document_{};
  std::unordered_map<std::string, Offset> tables_{};
  PointI last_pointer_{-1, -1};
  ScrollbarDrag scrollbar_drag_{};
  bool pointer_is_down_{false};
  bool suppress_content_drag_{false};
};

} // namespace pluss::view::window
