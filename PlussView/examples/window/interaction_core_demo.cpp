#include <cstddef>
#include <numeric>
#include <random>
#include <vector>

#include "pluss/view/window.h"

int main() {
  using namespace pluss::view::window;

  constexpr int n = 900;
  std::vector<double> x(static_cast<std::size_t>(n));
  std::vector<double> y(static_cast<std::size_t>(n));
  std::iota(x.begin(), x.end(), 0.0);

  std::mt19937_64 rng(20260306ULL);
  std::normal_distribution<double> noise(0.0, 0.075);
  for (int i = 0; i < n; ++i) {
    const double t = static_cast<double>(i) / 25.0;
    y[static_cast<std::size_t>(i)] = 0.44 * std::sin(t) + 0.18 * std::cos(0.3 * t) + noise(rng);
  }

  LinePlotConfig cfg{};
  cfg.plot_rect = RectI{120, 100, 980, 560};
  cfg.title = "Interaction Core Demo (Zoom/Pan/Reset)";
  cfg.x_label = "Observation";
  cfg.y_label = "Return";
  cfg.markers = false;

  LinePlotController controller(std::span<const double>(x.data(), x.size()),
                                std::span<const double>(y.data(), y.size()),
                                cfg);

  ViewerRuntime runtime(RuntimeConfig{SizeI{1280, 800}, Color::rgba(246, 247, 250, 255)});

  (void)runtime.submit(controller.build_scene());
  (void)save_canvas(runtime.canvas(), "PlussView/examples/window/interaction_core_frame_0.png");

  (void)controller.apply(LinePlotInteraction{
      .kind = LinePlotInteractionKind::Zoom,
      .zoom_factor = 1.6,
      .anchor_x = 0.62,
      .anchor_y = 0.45,
  });
  (void)runtime.submit(controller.build_scene());
  (void)save_canvas(runtime.canvas(), "PlussView/examples/window/interaction_core_frame_1_zoom.png");

  (void)controller.apply(LinePlotInteraction{
      .kind = LinePlotInteractionKind::Pan,
      .dx_ratio = 0.2,
      .dy_ratio = -0.15,
  });
  (void)runtime.submit(controller.build_scene());
  (void)save_canvas(runtime.canvas(), "PlussView/examples/window/interaction_core_frame_2_pan.png");

  (void)controller.apply(LinePlotInteraction{.kind = LinePlotInteractionKind::Reset});
  (void)runtime.submit(controller.build_scene());
  (void)save_canvas(runtime.canvas(), "PlussView/examples/window/interaction_core_frame_3_reset.png");

  return 0;
}
