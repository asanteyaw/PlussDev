#include "pluss/tils/view/window/canvas.h"

#include <algorithm>
#include <limits>

namespace pluss::view::window {

namespace {

constexpr std::size_t kChannels = 4;

[[nodiscard]] constexpr bool valid_size(const SizeI size) noexcept {
  return size.width > 0 && size.height > 0;
}

} // namespace

PixelCanvas::PixelCanvas(const SizeI size) {
  (void)resize(size);
}

bool PixelCanvas::resize(const SizeI size) {
  if (!valid_size(size)) {
    size_ = {};
    rgba_.clear();
    return false;
  }

  const auto width = static_cast<std::size_t>(size.width);
  const auto height = static_cast<std::size_t>(size.height);

  if (height != 0 && width > (std::numeric_limits<std::size_t>::max() / kChannels) / height) {
    size_ = {};
    rgba_.clear();
    return false;
  }

  size_ = size;
  rgba_.assign(width * height * kChannels, 0);
  return true;
}

void PixelCanvas::clear(const Color color) noexcept {
  for (std::size_t index = 0; index + 3 < rgba_.size(); index += kChannels) {
    rgba_[index] = color.r;
    rgba_[index + 1] = color.g;
    rgba_[index + 2] = color.b;
    rgba_[index + 3] = color.a;
  }
}

void PixelCanvas::fill_rect(RectI rect, const Color color) noexcept {
  if (empty() || rect.empty()) {
    return;
  }
  const int x0 = std::clamp(rect.x, 0, size_.width);
  const int y0 = std::clamp(rect.y, 0, size_.height);
  const int x1 = std::clamp(rect.x + rect.width, 0, size_.width);
  const int y1 = std::clamp(rect.y + rect.height, 0, size_.height);
  if (x1 <= x0 || y1 <= y0) {
    return;
  }

  const std::size_t row_stride = static_cast<std::size_t>(size_.width) * kChannels;
  const std::size_t start_x = static_cast<std::size_t>(x0) * kChannels;
  const std::size_t span = static_cast<std::size_t>(x1 - x0) * kChannels;
  if (color.a == 255) {
    for (int y = y0; y < y1; ++y) {
      std::uint8_t* pixel =
          rgba_.data() + static_cast<std::size_t>(y) * row_stride + start_x;
      std::uint8_t* const end = pixel + span;
      while (pixel < end) {
        pixel[0] = color.r;
        pixel[1] = color.g;
        pixel[2] = color.b;
        pixel[3] = color.a;
        pixel += kChannels;
      }
    }
    return;
  }

  const auto src_a = static_cast<std::uint16_t>(color.a);
  const auto inv_a = static_cast<std::uint16_t>(255 - color.a);
  for (int y = y0; y < y1; ++y) {
    std::uint8_t* pixel =
        rgba_.data() + static_cast<std::size_t>(y) * row_stride + start_x;
    std::uint8_t* const end = pixel + span;
    while (pixel < end) {
      pixel[0] = static_cast<std::uint8_t>(
          (static_cast<std::uint16_t>(color.r) * src_a +
           static_cast<std::uint16_t>(pixel[0]) * inv_a) /
          255);
      pixel[1] = static_cast<std::uint8_t>(
          (static_cast<std::uint16_t>(color.g) * src_a +
           static_cast<std::uint16_t>(pixel[1]) * inv_a) /
          255);
      pixel[2] = static_cast<std::uint8_t>(
          (static_cast<std::uint16_t>(color.b) * src_a +
           static_cast<std::uint16_t>(pixel[2]) * inv_a) /
          255);
      pixel[3] = 255;
      pixel += kChannels;
    }
  }
}

void PixelCanvas::set_pixel(const PointI pixel, const Color color) noexcept {
  if (!in_bounds_(pixel)) {
    return;
  }

  const auto index = index_of_(pixel);
  rgba_[index] = color.r;
  rgba_[index + 1] = color.g;
  rgba_[index + 2] = color.b;
  rgba_[index + 3] = color.a;
}

void PixelCanvas::blend_pixel(const PointI pixel, const Color color, const std::uint8_t alpha) noexcept {
  if (!in_bounds_(pixel) || alpha == 0) {
    return;
  }

  const auto index = index_of_(pixel);
  const auto src_a = static_cast<std::uint16_t>(alpha);
  const auto inv_a = static_cast<std::uint16_t>(255 - alpha);

  rgba_[index] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(color.r) * src_a +
                                            static_cast<std::uint16_t>(rgba_[index]) * inv_a) /
                                           255);
  rgba_[index + 1] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(color.g) * src_a +
                                                static_cast<std::uint16_t>(rgba_[index + 1]) * inv_a) /
                                               255);
  rgba_[index + 2] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(color.b) * src_a +
                                                static_cast<std::uint16_t>(rgba_[index + 2]) * inv_a) /
                                               255);
  rgba_[index + 3] = 255;
}

SizeI PixelCanvas::size() const noexcept {
  return size_;
}

bool PixelCanvas::empty() const noexcept {
  return rgba_.empty();
}

std::span<const std::uint8_t> PixelCanvas::bytes() const noexcept {
  return std::span<const std::uint8_t>(rgba_.data(), rgba_.size());
}

bool PixelCanvas::in_bounds_(const PointI pixel) const noexcept {
  return pixel.x >= 0 && pixel.y >= 0 && pixel.x < size_.width && pixel.y < size_.height;
}

std::size_t PixelCanvas::index_of_(const PointI pixel) const noexcept {
  const auto row = static_cast<std::size_t>(pixel.y);
  const auto col = static_cast<std::size_t>(pixel.x);
  const auto width = static_cast<std::size_t>(size_.width);
  return (row * width + col) * kChannels;
}

} // namespace pluss::view::window
