#pragma once

// What every table in PlussView draws on top of its cells, whether it is a short table laid out from its
// HTML rows or a long one painted from a model: the controls on the header cells (sort arrows, the drag
// handle on a divider and its hover highlight) and the scrollbars. Both paths call these, so a feature
// added here reaches every table and none has to be written twice.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "pluss/tils/view/window/html_layout.h"
#include "pluss/tils/view/window/scene.h"

namespace pluss::view::window::detail {

// Table sizes live in HtmlTableGeometry (html_layout.h), in em. The one size kept in pixels is the resize handle:
// it is a pointer target, so its size does not follow the text.
inline constexpr int kResizeHandlePx = 8;

// The colours a table takes from the page's own stylesheet.
struct TablePaint {
  Color grid;
  Color body_bg;
  Color track;
  Color thumb;
  int body_padding{0};     // the stylesheet's padding of a body cell and of a header cell, for sizing columns
  int header_padding{0};
  int header_weight{400};  // and the weight a header's text is drawn with
};

// Where a table's scrollbars go and how far it is scrolled.
struct TableScrollbars {
  RectI viewport{};            // header and body together
  RectI outer{};               // the box around the table: a track never goes past it
  int padding{0};
  int header_h{0};
  int frozen_w{0};             // the label column(s) frozen at the left
  int view_w{0};               // the window onto the scrolling columns
  std::int64_t content_w{0};   // all the scrolling columns together
  int body_view_h{0};          // the window onto the rows
  std::int64_t body_h{0};      // all the rows together
  int scroll_x{0};
  int scroll_y{0};
  int max_scroll_x{0};
  int max_scroll_y{0};
};

// Draws the tracks and thumbs that are needed and reports their rectangles to the render stats.
void draw_table_scrollbars(Scene& scene,
                           const HtmlRenderConfig& config,
                           const HtmlTableGeometryPx& geometry,
                           const TablePaint& paint,
                           const std::string& table_target,
                           const RectI& interactive_viewport,
                           const TableScrollbars& bars);

// The controls on one table's header cells. Add every header cell of the row, then call finish() so the
// drag handles are published last and win over the neighbouring headers.
class HeaderControls {
 public:
  struct Cell {
    std::size_t column{0};  // the column's identity in the table (the label column is 0)
    int x{0};
    int width{0};
    Color ink{};            // the header text colour
    Color paper{};          // the header background
    bool resizable{true};   // false for the automatic last column, which has no divider to drag
    int clip_left{0};       // controls are clipped to the part of the row this cell is visible in
    int clip_right{0};
  };

  HeaderControls(Scene& scene,
                 const HtmlRenderConfig& config,
                 std::string table_target,
                 const HtmlTableGeometryPx& geometry);

  void add(const Cell& cell, int row_y, int row_h);
  void finish(int row_y, int row_h);

 private:
  struct Look {
    int edge;
    std::size_t column;
    Color ink;
  };

  Scene& scene_;
  const HtmlRenderConfig& config_;
  std::string target_;
  const HtmlTableGeometryPx& geo_;
  bool sort_target_matches_;
  std::vector<HtmlHitRegion> handles_;
  std::vector<Look> looks_;
};

}  // namespace pluss::view::window::detail
