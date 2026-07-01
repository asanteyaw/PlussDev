#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include "window/render/async_compositor.h"

namespace {

void expect(const bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

pluss::view::window::Color pixel_at(
    const pluss::view::window::PixelCanvas& canvas,
    const int x,
    const int y) {
  const auto size = canvas.size();
  const auto bytes = canvas.bytes();
  const std::size_t index =
      (static_cast<std::size_t>(y) * static_cast<std::size_t>(size.width) +
       static_cast<std::size_t>(x)) *
      4U;
  return pluss::view::window::Color::rgba(
      bytes[index], bytes[index + 1U], bytes[index + 2U],
      bytes[index + 3U]);
}

bool same_rgb(const pluss::view::window::Color lhs,
              const pluss::view::window::Color rhs) {
  return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

} // namespace

int main() {
  using namespace pluss::view::window;
  using namespace pluss::view::window::detail;

  try {
    AsyncSoftwareCompositor compositor;
    const SizeI size{32, 24};
    const Color red = Color::rgba(210, 20, 30, 255);
    const Color blue = Color::rgba(15, 60, 220, 255);
    const Color green = Color::rgba(25, 180, 80, 255);

    Scene initial;
    initial.add(ClearCommand{red});
    compositor.submit(AsyncCompositorRequest{
        .frame_id = 1,
        .base_frame_id = 0,
        .canvas_size = size,
        .background = red,
        .full_scene = std::move(initial),
    });
    auto frame = compositor.wait_for_frame(1, std::chrono::seconds(2));
    expect(frame.has_value(), "initial asynchronous frame timed out");
    expect(same_rgb(pixel_at(frame->canvas, 2, 2), red),
           "full frame should paint the initial canvas");

    Scene full_second;
    full_second.add(ClearCommand{red});
    full_second.add(FillRectCommand{RectI{4, 4, 8, 8}, blue});
    Scene damage_second;
    damage_second.add(FillRectCommand{RectI{4, 4, 8, 8}, blue});
    compositor.submit(AsyncCompositorRequest{
        .frame_id = 2,
        .base_frame_id = 1,
        .canvas_size = size,
        .background = red,
        .full_scene = std::move(full_second),
        .incremental_scene = std::move(damage_second),
    });
    frame = compositor.wait_for_frame(2, std::chrono::seconds(2));
    expect(frame.has_value(), "incremental asynchronous frame timed out");
    expect(same_rgb(pixel_at(frame->canvas, 6, 6), blue),
           "incremental frame should paint its damage region");
    expect(same_rgb(pixel_at(frame->canvas, 20, 18), red),
           "incremental frame should retain pixels outside its damage region");

    Scene recovery;
    recovery.add(ClearCommand{green});
    Scene invalid_damage;
    invalid_damage.add(FillRectCommand{RectI{0, 0, 1, 1}, blue});
    compositor.submit(AsyncCompositorRequest{
        .frame_id = 4,
        .base_frame_id = 3,
        .canvas_size = size,
        .background = green,
        .full_scene = std::move(recovery),
        .incremental_scene = std::move(invalid_damage),
    });
    frame = compositor.wait_for_frame(4, std::chrono::seconds(2));
    expect(frame.has_value(), "recovery asynchronous frame timed out");
    expect(same_rgb(pixel_at(frame->canvas, 20, 18), green),
           "a skipped base frame must force full-scene recovery");
    expect(!compositor.failed(), "compositor should remain healthy");

    for (std::uint64_t frame_id = 5; frame_id < 45; ++frame_id) {
      Scene text_scene;
      text_scene.add(ClearCommand{Color::rgba(255, 255, 255, 255)});
      text_scene.add(TextCommand{
          .anchor = PointI{3, 18},
          .text = "Bold frame",
          .pixel_size = 14,
          .color = Color::rgba(10, 20, 35, 255),
          .font_weight = 700,
          .font_family = "system-ui",
      });
      compositor.submit(AsyncCompositorRequest{
          .frame_id = frame_id,
          .base_frame_id = frame_id - 1,
          .canvas_size = SizeI{120, 28},
          .background = Color::rgba(255, 255, 255, 255),
          .full_scene = std::move(text_scene),
      });
      frame =
          compositor.wait_for_frame(frame_id, std::chrono::seconds(2));
      expect(frame.has_value(), "repeated full text frame timed out");
      std::size_t dark_pixels = 0;
      const auto bytes = frame->canvas.bytes();
      for (std::size_t index = 0; index + 3U < bytes.size(); index += 4U) {
        if (bytes[index] < 180U || bytes[index + 1U] < 180U ||
            bytes[index + 2U] < 180U) {
          ++dark_pixels;
        }
      }
      expect(dark_pixels > 20,
             "repeated full frames must not drop bold text rasterization");
    }

    std::cout << "[PASS] async compositor full, damage, and recovery frames\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }
}
