#include <array>
#include <cstddef>
#include <fstream>
#include <numeric>
#include <string_view>
#include <vector>

#include "pluss/tils/view/window.h"
#include "demo_support.h"

namespace {

[[nodiscard]] std::string_view to_string(const pluss::view::window::ShellBackend backend) {
  using pluss::view::window::ShellBackend;
  switch (backend) {
    case ShellBackend::None:
      return "None";
    case ShellBackend::Null:
      return "Null";
    case ShellBackend::Cocoa:
      return "Cocoa";
    case ShellBackend::X11:
      return "X11";
    case ShellBackend::Win32:
      return "Win32";
  }
  return "Unknown";
}

} // namespace

int main() {
  using namespace pluss::view::window;

  constexpr int n = 300;
  std::vector<double> x(static_cast<std::size_t>(n));
  std::vector<double> y(static_cast<std::size_t>(n));
  std::iota(x.begin(), x.end(), 0.0);
  std::iota(y.begin(), y.end(), 0.0);

  LinePlotConfig cfg{};
  cfg.plot_rect = RectI{100, 80, 760, 420};
  cfg.title = "Shell Backend Demo";
  cfg.x_label = "x";
  cfg.y_label = "y";

  LinePlotWindowSessionConfig session_cfg{};
  session_cfg.runtime = RuntimeConfig{SizeI{980, 620}, Color::rgba(246, 248, 250, 255)};
  session_cfg.shell.initial_size = session_cfg.runtime.canvas_size;
  session_cfg.shell.title = "PlussTilsView Shell Backend Demo";
  session_cfg.shell_policy = ShellSelectionPolicy::NativePreferred;

  auto& registry = WindowRegistry::instance();
  registry.close_all();
  (void)registry.prune_closed();

  const WindowSessionId id = registry.create_line_plot_session(
      LinePlotController(std::span<const double>(x.data(), x.size()),
                         std::span<const double>(y.data(), y.size()),
                         cfg),
      session_cfg);
  if (id == 0 || !registry.open(id)) {
    return 1;
  }
  (void)registry.tick(id);
  (void)registry.save_canvas(id, "PlussTils/view/examples/window/shell_backend_demo.png");
  const std::array<WindowSessionId, 1> session_ids{id};
  demo::run_registry_preview(registry, std::span<const WindowSessionId>(session_ids), demo::demo_hold_duration());

  ShellBackend backend = ShellBackend::None;
  ShellCapabilities capabilities{};
  for (const auto& snapshot : registry.snapshots()) {
    if (snapshot.id == id) {
      backend = snapshot.backend;
      capabilities = snapshot.capabilities;
      break;
    }
  }

  std::ofstream meta("PlussTils/view/examples/window/shell_backend_demo_backend.txt",
                     std::ios::out | std::ios::trunc);
  if (!meta.is_open()) {
    return 2;
  }
  meta << to_string(backend) << '\n';
  meta << "native=" << (is_native_backend(backend) ? "true" : "false") << '\n';
  meta << "supports_live_present=" << (capabilities.supports_live_present ? "true" : "false") << '\n';
  meta << "supports_resize_events=" << (capabilities.supports_resize_events ? "true" : "false") << '\n';
  meta << "supports_shortcut_events=" << (capabilities.supports_shortcut_events ? "true" : "false")
       << '\n';
  meta << "supports_pointer_scroll=" << (capabilities.supports_pointer_scroll ? "true" : "false")
       << '\n';

  registry.close_all();
  (void)registry.prune_closed();
  return 0;
}
