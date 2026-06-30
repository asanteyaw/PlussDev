#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "pluss/tils/view/window/types.h"

namespace pluss::view::window {

class PixelCanvas {
 public:
  PixelCanvas() = default;
  explicit PixelCanvas(SizeI size);

  [[nodiscard]] bool resize(SizeI size);
  void clear(Color color) noexcept;
  void fill_rect(RectI rect, Color color) noexcept;
  void set_pixel(PointI pixel, Color color) noexcept;
  void blend_pixel(PointI pixel, Color color, std::uint8_t alpha) noexcept;

  [[nodiscard]] SizeI size() const noexcept;
  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept;

 private:
  [[nodiscard]] bool in_bounds_(PointI pixel) const noexcept;
  [[nodiscard]] std::size_t index_of_(PointI pixel) const noexcept;

  SizeI size_{};
  std::vector<std::uint8_t> rgba_{};
};

} // namespace pluss::view::window
