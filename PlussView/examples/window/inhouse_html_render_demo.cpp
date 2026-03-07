#include "pluss/view/window.h"
#include "demo_support.h"

#include <string>

int main() {
  using namespace pluss::view::window;

  const std::string html = R"HTML(
<!doctype html>
<html>
<head>
  <title>InHouse HTML Render Demo</title>
  <style>
    body { color: #1f2328; background: #f6f8fa; }
    .hero { font-size: 22px; margin: 10px; color: #0b3d91; }
    .card { background: #ffffff; border: 1px solid #d0d7de; margin: 8px; padding: 10px; }
    #metrics td { color: #2a2f35; }
    #metrics th { color: #0f172a; }
  </style>
</head>
<body>
  <div class="hero">PlussView In-House HTML Engine</div>
  <div class="card">
    <p>Deterministic parser + style cascade + layout + software render.</p>
    <button style="padding:6px 12px; border:1px solid #7d8590;">Export</button>
  </div>
  <table id="metrics" class="card">
    <tr><th>Metric</th><th>Value</th></tr>
    <tr><td>Backend</td><td>PlussWebView</td></tr>
    <tr><td>Renderer</td><td>SoftwareRenderEngine</td></tr>
    <tr><td>Status</td><td>Smoke Pass</td></tr>
  </table>
</body>
</html>
)HTML";

  const auto document = parse_html_document(html);
  const auto scene = build_html_document_scene(document, SizeI{1240, 820});

  ViewerRuntime runtime(RuntimeConfig{SizeI{1240, 820}, Color::rgba(245, 248, 252, 255)});
  if (!runtime.submit(scene)) {
    return 1;
  }

  if (!save_canvas(runtime.canvas(), "PlussView/examples/window/inhouse_html_render_demo.png")) {
    return 2;
  }

  auto shell = make_window_shell(ShellSelectionPolicy::NativeOnly);
  if (!shell) {
    return 3;
  }
  if (!shell->open(ShellConfig{"PlussView In-House HTML Render Demo", runtime.canvas().size()})) {
    return 4;
  }
  if (!shell->present(runtime.canvas())) {
    shell->close();
    return 5;
  }
  demo::hold_shell_open(*shell, demo::demo_hold_duration());
  shell->close();

  return 0;
}
