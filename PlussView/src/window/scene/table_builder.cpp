#include "pluss/view/window/table.h"

#include <algorithm>
#include <numeric>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "pluss/view/window/text_layout.h"

namespace pluss::view::window {

namespace {

[[nodiscard]] int clamp_px(const int v, const int lo) {
  return std::max(lo, v);
}

[[nodiscard]] int compute_visible_rows(const TableConfig& cfg) {
  const int row_h = clamp_px(cfg.row_height, 20);
  const int usable = cfg.table_rect.height - row_h;
  return std::max(1, usable / row_h);
}

[[nodiscard]] std::size_t column_count(const std::vector<std::string>& headers,
                                       const std::vector<std::vector<std::string>>& rows) {
  std::size_t cols = headers.size();
  for (const auto& row : rows) {
    cols = std::max(cols, row.size());
  }
  return std::max<std::size_t>(1, cols);
}

[[nodiscard]] std::string default_header(const std::size_t idx) {
  return "col_" + std::to_string(idx + 1);
}

[[nodiscard]] int text_width_px(const std::string& text, const int pixel_size) {
  return layout_math_text(text, pixel_size).width;
}

[[nodiscard]] std::string truncate_to_width(const std::string& text,
                                            const int pixel_size,
                                            const int max_width) {
  if (max_width <= 0 || text.empty()) {
    return {};
  }
  if (text_width_px(text, pixel_size) <= max_width) {
    return text;
  }

  const std::string ellipsis = "...";
  const int ellipsis_w = text_width_px(ellipsis, pixel_size);
  std::string out = text;
  while (!out.empty() && text_width_px(out, pixel_size) + ellipsis_w > max_width) {
    out.pop_back();
  }
  if (out.empty()) {
    return ellipsis;
  }
  out += ellipsis;
  return out;
}

void draw_frame(Scene& scene, const TableConfig& cfg) {
  scene.add(ClearCommand{cfg.background});
  scene.add(StrokeRectCommand{cfg.table_rect, 2, cfg.frame_color});
  scene.add(TextCommand{
      PointI{cfg.table_rect.x, cfg.table_rect.y - 24},
      cfg.title,
      clamp_px(cfg.title_pixel_size, 12),
      cfg.text_color,
  });
}

} // namespace

Scene build_table_scene(const std::vector<std::string>& headers,
                        const std::vector<std::vector<std::string>>& rows,
                        const TableConfig& config,
                        const TableViewWindow& window) {
  Scene scene;
  draw_frame(scene, config);

  const int row_h = clamp_px(config.row_height, 20);
  const int text_px = clamp_px(config.text_pixel_size, 10);
  const int pad = clamp_px(config.cell_padding, 4);

  const std::size_t cols = column_count(headers, rows);
  const int col_w = std::max(60, config.table_rect.width / static_cast<int>(cols));
  const int total_w = col_w * static_cast<int>(cols);
  const int left = config.table_rect.x;
  const int top = config.table_rect.y;
  const int visible_rows = compute_visible_rows(config);
  const std::size_t row_start = std::min(window.row_offset, rows.size());
  const std::size_t row_end = std::min(rows.size(), row_start + static_cast<std::size_t>(visible_rows));

  scene.add(FillRectCommand{RectI{left, top, total_w, row_h}, config.header_fill});

  for (std::size_t c = 0; c < cols; ++c) {
    const int x = left + static_cast<int>(c) * col_w;
    const std::string header = c < headers.size() ? headers[c] : default_header(c);
    scene.add(TextCommand{
        PointI{x + pad, top + row_h - pad},
        truncate_to_width(header, text_px, col_w - (pad * 2)),
        text_px,
        config.text_color,
    });
    scene.add(LineCommand{PointI{x, top}, PointI{x, top + config.table_rect.height}, 1, config.grid_color});
  }
  scene.add(LineCommand{PointI{left + total_w, top},
                        PointI{left + total_w, top + config.table_rect.height},
                        1,
                        config.grid_color});

  std::size_t draw_row = 0;
  for (std::size_t r = row_start; r < row_end; ++r, ++draw_row) {
    const int y = top + row_h + static_cast<int>(draw_row) * row_h;
    const Color fill = (draw_row % 2 == 0) ? config.row_even_fill : config.row_odd_fill;
    scene.add(FillRectCommand{RectI{left, y, total_w, row_h}, fill});
    scene.add(LineCommand{PointI{left, y}, PointI{left + total_w, y}, 1, config.grid_color});

    const auto& row = rows[r];
    for (std::size_t c = 0; c < cols; ++c) {
      const int x = left + static_cast<int>(c) * col_w;
      const std::string value = c < row.size() ? row[c] : std::string{};
      scene.add(TextCommand{
          PointI{x + pad, y + row_h - pad},
          truncate_to_width(value, text_px, col_w - (pad * 2)),
          text_px,
          config.text_color,
      });
    }
  }

  const int bottom = top + row_h + static_cast<int>(draw_row) * row_h;
  scene.add(LineCommand{PointI{left, bottom}, PointI{left + total_w, bottom}, 1, config.grid_color});

  const std::string footer = "rows " + std::to_string(row_start + 1) + "-" +
                             std::to_string(row_end) + " of " + std::to_string(rows.size());
  scene.add(TextCommand{
      PointI{left, top + config.table_rect.height + 22},
      footer,
      text_px,
      config.text_color,
  });

  return scene;
}

TableController::TableController(std::vector<std::string> headers,
                                 std::vector<std::vector<std::string>> rows,
                                 TableConfig config)
    : headers_(std::move(headers)),
      rows_(std::move(rows)),
      config_(std::move(config)) {
  clamp_offset_();
}

bool TableController::apply(const TableInteraction& interaction) {
  const std::size_t ps = page_size_();
  switch (interaction.kind) {
    case TableInteractionKind::NextPage:
      window_.row_offset = std::min(rows_.size(), window_.row_offset + ps);
      clamp_offset_();
      return true;
    case TableInteractionKind::PreviousPage:
      window_.row_offset = (window_.row_offset > ps) ? (window_.row_offset - ps) : 0;
      clamp_offset_();
      return true;
    case TableInteractionKind::SetOffset:
      window_.row_offset = interaction.offset;
      clamp_offset_();
      return true;
  }
  return false;
}

Scene TableController::build_scene() const {
  return build_table_scene(headers_, rows_, config_, window_);
}

std::size_t TableController::row_offset() const noexcept {
  return window_.row_offset;
}

std::size_t TableController::row_count() const noexcept {
  return rows_.size();
}

std::size_t TableController::visible_row_count() const noexcept {
  return page_size_();
}

std::size_t TableController::page_size_() const noexcept {
  return static_cast<std::size_t>(compute_visible_rows(config_));
}

void TableController::clamp_offset_() noexcept {
  if (rows_.empty()) {
    window_.row_offset = 0;
    return;
  }
  const std::size_t ps = page_size_();
  if (window_.row_offset >= rows_.size()) {
    window_.row_offset = rows_.size() - 1;
  }
  if (window_.row_offset + ps > rows_.size()) {
    window_.row_offset = rows_.size() > ps ? rows_.size() - ps : 0;
  }
}

} // namespace pluss::view::window
