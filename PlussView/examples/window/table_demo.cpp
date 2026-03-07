#include <array>
#include <string>
#include <vector>

#include "pluss/view/window.h"
#include "demo_support.h"

int main() {
  using namespace pluss::view::window;

  auto& registry = WindowRegistry::instance();
  registry.close_all();
  (void)registry.prune_closed();

  std::vector<std::string> headers{
      "timestamp",
      "symbol",
      "return",
      "volatility",
      "regime",
      "note",
  };

  std::vector<std::vector<std::string>> rows;
  rows.reserve(72);
  for (int i = 0; i < 72; ++i) {
    const std::string symbol = (i % 2 == 0) ? "ASSET_A" : "ASSET_B";
    const double ret = 0.015 * (static_cast<double>(i % 9) - 4.0);
    const double vol = 0.12 + 0.005 * static_cast<double>(i % 11);
    const std::string regime = (i % 3 == 0) ? "calm" : ((i % 3 == 1) ? "normal" : "stress");
    rows.push_back({
        "2026-03-" + std::to_string(1 + (i % 28)),
        symbol,
        std::to_string(ret),
        std::to_string(vol),
        regime,
        "row_" + std::to_string(i + 1),
    });
  }

  TableConfig cfg{};
  cfg.title = "In-House Table Session Demo";
  cfg.table_rect = RectI{80, 90, 1120, 600};
  cfg.row_height = 30;
  cfg.text_pixel_size = 12;

  LinePlotWindowSessionConfig session_cfg{};
  session_cfg.runtime = RuntimeConfig{SizeI{1280, 820}, Color::rgba(247, 248, 250, 255)};
  session_cfg.shell.initial_size = session_cfg.runtime.canvas_size;
  session_cfg.shell_policy = ShellSelectionPolicy::NativePreferred;

  const WindowSessionId id = registry.create_table_session(
      TableController(std::move(headers), std::move(rows), cfg), session_cfg);

  if (id == 0 || !registry.open(id)) {
    return 1;
  }
  (void)registry.save_canvas(id, "PlussView/examples/window/table_demo_page_0.png");

  (void)registry.enqueue(id, WindowCommand{.kind = WindowCommandKind::TableNextPage});
  (void)registry.tick(id);
  (void)registry.save_canvas(id, "PlussView/examples/window/table_demo_page_1.png");

  (void)registry.enqueue(id, WindowCommand{
                                 .kind = WindowCommandKind::TableSetOffset,
                                 .table_interaction = TableInteraction{
                                     .kind = TableInteractionKind::SetOffset,
                                     .offset = 50,
                                 },
                             });
  (void)registry.tick(id);
  (void)registry.save_canvas(id, "PlussView/examples/window/table_demo_page_2.png");
  const std::array<WindowSessionId, 1> session_ids{id};
  demo::run_registry_preview(registry, std::span<const WindowSessionId>(session_ids), demo::demo_hold_duration());

  registry.close_all();
  (void)registry.prune_closed();
  return 0;
}
