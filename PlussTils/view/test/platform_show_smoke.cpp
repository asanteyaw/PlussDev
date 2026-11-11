#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>

#include "pluss/tils/view/html_window.h"
#include "pluss/tils/view/session.h"
#include "pluss/tils/view/window.h"

namespace {

using namespace pluss::view::window;

constexpr std::string_view kTable = R"HTML(
<!doctype html><html><body style="background:#fff">
<table><thead><tr><th>Region</th><th>Revenue</th></tr></thead>
<tbody><tr><td>North</td><td>120</td></tr>
<tr><td>South</td><td>95</td></tr></tbody></table>
</body></html>)HTML";

constexpr std::string_view kPlot = R"HTML(
<!doctype html><html><body style="background:#fff">
<svg width="480" height="300" viewBox="0 0 480 300">
<rect x="0" y="0" width="480" height="300" fill="#ffffff"/>
<polyline points="20,260 100,210 180,180 260,120 340,90 440,30"
          fill="none" stroke="#d12b2b" stroke-width="4"/>
</svg></body></html>)HTML";

bool render_case(const std::string_view name, const std::string_view html,
                 const bool expect_table, const std::filesystem::path& output,
                 PixelCanvas* result) {
  HtmlRenderStats stats{};
  HtmlRenderConfig layout{};
  layout.draw_top_bar = false;
  layout.render_stats = &stats;
  const auto document = parse_html_document(std::string(html), std::string(name));
  const auto scene = build_html_document_scene(document, SizeI{640, 420}, layout);
  if (stats.has_table != expect_table) return false;

  bool has_text = false;
  bool has_graph = false;
  for (const auto& command : scene.commands()) {
    has_text |= std::holds_alternative<TextCommand>(command);
    has_graph |= std::holds_alternative<PolylineCommand>(command);
  }
  if (expect_table ? !has_text : !has_graph) return false;

  ViewerRuntime runtime(RuntimeConfig{SizeI{640, 420}, Color::rgba(255, 255, 255, 255)});
  if (!runtime.submit(scene) || runtime.canvas().empty()) return false;
  if (!save_canvas_png(runtime.canvas(), output.string())) return false;
  *result = runtime.canvas();
  return true;
}

bool present_window(const PixelCanvas& canvas) {
  auto shell = make_window_shell(ShellSelectionPolicy::NativeOnly);
  if (!shell || !is_native_backend(shell->backend())) return false;
  const auto capabilities = shell->capabilities();
  if (!capabilities.supports_live_present) return false;
  if (!shell->open(ShellConfig{"PlussII built-in renderer smoke", canvas.size()})) return false;
  const bool presented = shell->present(canvas);
  shell->close();
  return presented;
}

} // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 3) {
    std::cerr << "usage: platform_show_smoke OUTPUT_DIR [--window]\n";
    return 1;
  }
  const std::filesystem::path output(argv[1]);
  std::error_code ec;
  std::filesystem::create_directories(output, ec);
  if (ec) return 2;

  PixelCanvas table;
  PixelCanvas plot;
  if (!render_case("Table", kTable, true, output / "table.png", &table)) return 3;
  if (!render_case("Plot", kPlot, false, output / "plot.png", &plot)) return 4;

  if (argc == 3) {
    if (std::string_view(argv[2]) != "--window") return 5;
    if (!pluss::view::runtime_gui_environment_available() ||
        !pluss::view::query_host_capabilities(pluss::view::SessionBackend::PlussWebView).available) return 6;
    if (!present_window(table) || !present_window(plot)) return 7;
  }
  std::cout << "table and plot rendered with the built-in software renderer\n";
  return 0;
}
