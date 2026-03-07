#include <cmath>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#include "pluss/view/window.h"
#include "demo_support.h"

int main() {
  using namespace pluss::view::window;

  constexpr int n = 720;
  std::vector<double> x(static_cast<std::size_t>(n));
  std::vector<double> y(static_cast<std::size_t>(n));
  std::iota(x.begin(), x.end(), 0.0);

  std::mt19937_64 rng(20260306ULL);
  std::normal_distribution<double> noise(0.0, 0.06);

  for (int i = 0; i < n; ++i) {
    const double t = static_cast<double>(i) / 30.0;
    y[static_cast<std::size_t>(i)] = 0.45 * std::sin(t) + 0.12 * std::cos(0.35 * t) + noise(rng);
  }

  LinePlotConfig cfg{};
  cfg.plot_rect = RectI{140, 110, 1080, 640};
  cfg.title = R"(PlussView Popup Showcase: $\mu_t = r + \lambda\sigma_t - \frac{1}{2}\sigma_t^2$)";
  cfg.x_label = "Observation";
  cfg.y_label = R"($y_t$)";
  cfg.markers = true;

  Scene scene = build_line_plot_scene(std::span<const double>(x.data(), x.size()),
                                      std::span<const double>(y.data(), y.size()),
                                      cfg);

  ViewerRuntime runtime(RuntimeConfig{SizeI{1440, 900}, Color::rgba(247, 248, 250, 255)});
  if (!runtime.submit(scene)) {
    std::cerr << "render failed\n";
    return 1;
  }

  auto shell = make_window_shell(ShellSelectionPolicy::NativePreferred);
  if (!shell->open(ShellConfig{"PlussView In-House Popup Showcase", runtime.canvas().size()})) {
    std::cerr << "shell open failed\n";
    return 1;
  }
  if (!shell->present(runtime.canvas())) {
    std::cerr << "shell present failed\n";
    shell->close();
    return 1;
  }

  std::cout << "popup opened; close with the window controls\n";
  demo::hold_shell_open(*shell, demo::demo_hold_duration());

  shell->close();
  return 0;
}
