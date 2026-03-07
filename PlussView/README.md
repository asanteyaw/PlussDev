# PlussView

PlussView is a shared window/runtime layer for GUI display across Pluss libraries
(PlussPl, PlussMic, and future modules).

## Scope in this tranche

- Shared native HTML popup API (`pluss::view::show_html_window`)
- Shared Linux GTK/WebKit host (when dependencies are available)
- Common macOS WebKit host
- Common Windows WebView2 host
- Runtime GUI environment checks

Caller libraries no longer own platform window backends.

## Public API

- Header: `pluss/view/html_window.h`
- Header: `pluss/view/session.h`
- Header: `pluss/view/window.h`
- Namespace: `pluss::view`

```cpp
pluss::view::HtmlWindowOptions opts;
opts.title = "PlussView";
opts.snapshot_name = "plot-view.png";
opts.width = 1280;
opts.height = 760;
opts.backend_preference = pluss::view::BackendPreference::Auto;
// Use PlussWebView to prefer the in-house HTML renderer backend.
// opts.backend_preference = pluss::view::BackendPreference::PlussWebView;

const bool opened = pluss::view::show_html_window(html, opts);
```

```cpp
const auto backend = pluss::view::detect_native_backend();
const auto caps = pluss::view::native_host_capabilities();
```

```cpp
pluss::view::window::ViewerRuntime runtime;
pluss::view::window::Scene scene;
// populate scene with window draw commands
const bool ok = runtime.submit(scene);
```

```cpp
std::vector<double> y = {/*...*/};
pluss::view::window::LinePlotConfig cfg;
cfg.title = "First plot";
auto scene = pluss::view::window::build_line_plot_scene(std::span<const double>(y.data(), y.size()), cfg);
```

```cpp
auto shell = pluss::view::window::make_window_shell(
    pluss::view::window::ShellSelectionPolicy::PortableOnly);
(void)shell->open({"PlussView", runtime.canvas().size()});
(void)shell->present(runtime.canvas());
```

`make_window_shell` is policy-driven:
- `PortableOnly`: always cross-platform null shell (no native window dependency).
- `PortablePreferred`: null shell unless `PLUSSVIEW_WINDOW_NATIVE=1`.
- `NativePreferred`: native shell when available, else null shell.
- `NativeOnly`: native shell only (returns null if unavailable).

Default is `PortableOnly` to keep the in-house runtime path platform-agnostic during rollout.

## Design intent

PlussView follows the same high-level pattern observed in MATLAB's graphics stack:

- thin caller API
- host selection delegated to a platform layer
- centralized lifecycle and save behavior in the host

This keeps rendering code (PlussPl/PlussMic) separate from windowing concerns.
