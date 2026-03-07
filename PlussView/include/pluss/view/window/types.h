#pragma once

#include <cstdint>

namespace pluss::view::window {

struct PointI {
  int x{0};
  int y{0};
};

struct SizeI {
  int width{0};
  int height{0};
};

struct RectI {
  int x{0};
  int y{0};
  int width{0};
  int height{0};

  [[nodiscard]] constexpr bool empty() const noexcept {
    return width <= 0 || height <= 0;
  }
};

struct Color {
  std::uint8_t r{0};
  std::uint8_t g{0};
  std::uint8_t b{0};
  std::uint8_t a{255};

  [[nodiscard]] static constexpr Color rgba(std::uint8_t red,
                                            std::uint8_t green,
                                            std::uint8_t blue,
                                            std::uint8_t alpha = 255) noexcept {
    return Color{red, green, blue, alpha};
  }
};

} // namespace pluss::view::window
