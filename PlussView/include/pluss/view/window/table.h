#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "pluss/view/window/scene.h"

namespace pluss::view::window {

struct TableConfig {
  RectI table_rect{100, 100, 960, 560};
  std::string title{"Table"};
  Color background{Color::rgba(248, 248, 250, 255)};
  Color frame_color{Color::rgba(52, 58, 64, 255)};
  Color header_fill{Color::rgba(229, 234, 240, 255)};
  Color row_even_fill{Color::rgba(254, 254, 255, 255)};
  Color row_odd_fill{Color::rgba(245, 248, 252, 255)};
  Color grid_color{Color::rgba(206, 212, 220, 255)};
  Color text_color{Color::rgba(20, 24, 28, 255)};
  int row_height{28};
  int title_pixel_size{18};
  int text_pixel_size{12};
  int cell_padding{8};
};

struct TableViewWindow {
  std::size_t row_offset{0};
};

enum class TableInteractionKind : std::uint8_t {
  NextPage = 0,
  PreviousPage = 1,
  SetOffset = 2,
};

struct TableInteraction {
  TableInteractionKind kind{TableInteractionKind::SetOffset};
  std::size_t offset{0};
};

[[nodiscard]] Scene build_table_scene(const std::vector<std::string>& headers,
                                      const std::vector<std::vector<std::string>>& rows,
                                      const TableConfig& config = {},
                                      const TableViewWindow& window = {});

class TableController {
 public:
  TableController(std::vector<std::string> headers,
                  std::vector<std::vector<std::string>> rows,
                  TableConfig config = {});

  [[nodiscard]] bool apply(const TableInteraction& interaction);
  [[nodiscard]] Scene build_scene() const;

  [[nodiscard]] std::size_t row_offset() const noexcept;
  [[nodiscard]] std::size_t row_count() const noexcept;
  [[nodiscard]] std::size_t visible_row_count() const noexcept;

 private:
  [[nodiscard]] std::size_t page_size_() const noexcept;
  void clamp_offset_() noexcept;

  std::vector<std::string> headers_{};
  std::vector<std::vector<std::string>> rows_{};
  TableConfig config_{};
  TableViewWindow window_{};
};

} // namespace pluss::view::window
