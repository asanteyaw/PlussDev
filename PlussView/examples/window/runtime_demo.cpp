#include <cmath>
#include <cstddef>
#include <utility>

#include "pluss/view/window.h"
#include "demo_support.h"

int main() {
  using namespace pluss::view::window;

  ViewerRuntime runtime(RuntimeConfig{SizeI{1100, 700}, Color::rgba(246, 247, 248, 255)});

  Scene scene;
  scene.add(ClearCommand{Color::rgba(246, 247, 248, 255)});
  scene.add(StrokeRectCommand{RectI{70, 70, 950, 560}, 2, Color::rgba(34, 34, 34, 255)});
  scene.add(LineCommand{PointI{120, 560}, PointI{980, 560}, 2, Color::rgba(24, 24, 24, 255)});
  scene.add(LineCommand{PointI{120, 560}, PointI{120, 120}, 2, Color::rgba(24, 24, 24, 255)});

  PolylineCommand trend{};
  trend.thickness = 1;
  trend.color = Color::rgba(20, 115, 210, 255);
  for (int step = 0; step < 820; ++step) {
    const double t = static_cast<double>(step) / 64.0;
    const int y = 340 + static_cast<int>(90.0 * std::sin(t) + 45.0 * std::cos(0.42 * t));
    trend.points.push_back(PointI{150 + step, y});
  }
  scene.add(DrawCommand{trend});

  MarkerCloudCommand markers{};
  markers.radius = 2;
  markers.color = Color::rgba(213, 70, 70, 220);
  for (int i = 0; i < 65; ++i) {
    const double t = static_cast<double>(i) * 0.22;
    const int x = 160 + i * 12;
    const int y = 340 + static_cast<int>(95.0 * std::sin(t));
    markers.points.push_back(PointI{x, y});
  }
  scene.add(DrawCommand{std::move(markers)});

  (void)runtime.submit(scene);
  auto shell = make_window_shell(ShellSelectionPolicy::NativePreferred);
  (void)shell->open(ShellConfig{"PlussView Window Runtime Demo", runtime.canvas().size()});
  (void)shell->present(runtime.canvas());
  demo::hold_shell_open(*shell, demo::demo_hold_duration());
  shell->close();
  (void)save_canvas(runtime.canvas(), "PlussView/examples/window/runtime_demo.png");

  return 0;
}
