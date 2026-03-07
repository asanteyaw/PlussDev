#include "pluss/view/window/canvas.h"

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
