#include <cmath>
#include <cstddef>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

#include "pluss/view/window.h"
#include "demo_support.h"

int main() {
  using namespace pluss::view::window;

  constexpr int n = 512;
  std::vector<double> x(static_cast<std::size_t>(n));
  std::vector<double> y(static_cast<std::size_t>(n));

  std::iota(x.begin(), x.end(), 0.0);

  std::mt19937_64 rng(42);
  std::normal_distribution<double> noise(0.0, 0.08);

  for (int i = 0; i < n; ++i) {
    const double t = static_cast<double>(i) / 24.0;
    y[static_cast<std::size_t>(i)] = 0.3 * std::sin(t) + 0.15 * std::cos(0.4 * t) + noise(rng);
  }

  LinePlotConfig cfg{};
  cfg.plot_rect = RectI{120, 100, 980, 580};
  cfg.title = R"(Returns Plot: $\mu_t = r + \lambda\sigma_t - \frac{1}{2}\sigma_t^2$)";
  cfg.x_label = "Observation";
  cfg.y_label = R"($y_t$)";
  cfg.markers = true;

  Scene scene = build_line_plot_scene(std::span<const double>(x.data(), x.size()),
                                      std::span<const double>(y.data(), y.size()),
                                      cfg);

  ViewerRuntime runtime(RuntimeConfig{SizeI{1280, 820}, Color::rgba(248, 249, 251, 255)});
  (void)runtime.submit(scene);

  auto shell = make_window_shell(ShellSelectionPolicy::NativePreferred);
  (void)shell->open(ShellConfig{"PlussView First Plot Demo", runtime.canvas().size()});
  (void)shell->present(runtime.canvas());
  demo::hold_shell_open(*shell, demo::demo_hold_duration());
  shell->close();

  (void)save_canvas(runtime.canvas(), "PlussView/examples/window/first_plot_demo.png");
  return 0;
}
