#include <cstddef>
#include <numeric>
#include <random>
#include <vector>

#include "pluss/view/window.h"
#include "demo_support.h"

int main() {
  using namespace pluss::view::window;

  constexpr int n = 860;
  std::vector<double> x(static_cast<std::size_t>(n));
  std::vector<double> y(static_cast<std::size_t>(n));
  std::iota(x.begin(), x.end(), 0.0);

  std::mt19937_64 rng(20260306ULL);
  std::normal_distribution<double> noise(0.0, 0.08);
  for (int i = 0; i < n; ++i) {
    const double t = static_cast<double>(i) / 20.0;
    y[static_cast<std::size_t>(i)] = 0.35 * std::sin(t) + 0.22 * std::cos(0.22 * t) + noise(rng);
  }

  LinePlotConfig cfg{};
  cfg.plot_rect = RectI{120, 90, 980, 560};
  cfg.title = "Window Session Demo (Command Queue)";
  cfg.x_label = "Observation";
  cfg.y_label = "Return";

  LinePlotController controller(std::span<const double>(x.data(), x.size()),
                                std::span<const double>(y.data(), y.size()),
                                cfg);

  LinePlotWindowSessionConfig session_cfg{};
  session_cfg.runtime = RuntimeConfig{SizeI{1280, 780}, Color::rgba(246, 247, 250, 255)};
  session_cfg.shell = ShellConfig{"PlussView Session Demo", session_cfg.runtime.canvas_size};
  session_cfg.shell_policy = ShellSelectionPolicy::NativePreferred;
  session_cfg.input.key_pan_step_ratio = 0.12;
  session_cfg.input.key_zoom_factor = 1.20;
  session_cfg.input.scroll_zoom_factor = 1.08;

  LinePlotWindowSession session(std::move(controller), session_cfg);
  if (!session.open()) {
    return 1;
  }
  (void)save_canvas(session.canvas(), "PlussView/examples/window/session_demo_frame_0.png");

  (void)session.enqueue(WindowCommand{
      .kind = WindowCommandKind::Zoom,
      .interaction = LinePlotInteraction{
          .kind = LinePlotInteractionKind::Zoom,
          .zoom_factor = 1.7,
          .anchor_x = 0.58,
          .anchor_y = 0.42,
      },
  });
  if (!session.tick()) {
    return 1;
  }
  (void)save_canvas(session.canvas(), "PlussView/examples/window/session_demo_frame_1_zoom.png");

  (void)session.enqueue(WindowCommand{
      .kind = WindowCommandKind::Pan,
      .interaction = LinePlotInteraction{
          .kind = LinePlotInteractionKind::Pan,
          .dx_ratio = 0.18,
          .dy_ratio = -0.12,
      },
  });
  if (!session.tick()) {
    return 1;
  }
  (void)save_canvas(session.canvas(), "PlussView/examples/window/session_demo_frame_2_pan.png");

  (void)session.enqueue(WindowCommand{
      .kind = WindowCommandKind::ResetView,
      .interaction = LinePlotInteraction{.kind = LinePlotInteractionKind::Reset},
  });
  if (!session.tick()) {
    return 1;
  }
  (void)save_canvas(session.canvas(), "PlussView/examples/window/session_demo_frame_3_reset.png");

  demo::run_session_preview(session, demo::demo_hold_duration());
  session.close();
  return 0;
}
