# PlussPl

`PlussPl` is organized as a header-first plotting library with modular internals and thin public facades.

## Layout

- `include/pluss/plot/`
- Primary public API surface and forwarding headers.
- `src/plot/`
- Internal implementation headers grouped by module.
- `src/plot/modules/core.h`
- Common utilities, formatting, escaping, and file IO helpers.
- `src/plot/modules/{axes.h,style.h,trace.h}`
- Data/model objects for axes, style, traces.
- `src/plot/modules/{figure2d.h,figure3d.h,figure.h}`
- Figure API and per-dimension behavior.
- `src/plot/modules/backend/`
- Backend abstraction and implementations.
- `src/plot/modules/backend/svg/`
- SVG renderer primitives + 2D/3D renderers.
- `src/plot/modules/backend/pdf/`
- PDF renderer primitives + 2D/3D renderers.
- `examples/`
- Runnable demos by dimension.
- `tests/`
- Executable tests by dimension.

## Build Flags

- `PLUSSPL_ENABLE_INTERACTIVE_SHOW`
- `ON`: `Figure::show()` opens a popup viewer when an embedded backend is available, otherwise it falls back to the system viewer (`open`, `xdg-open`, `start`), then to text preview.
- `OFF`: `Figure::show()` always prints text preview.
- `PLUSSPL_ENABLE_POPUP_VIEWER`
- `ON`: compile embedded popup backends when platform dependencies are available. macOS uses native `WKWebView`; Linux uses GTK3 + WebKitGTK.
- `OFF`: keep `show()` and `show_window()` on external viewer / text preview paths only.
- `PLUSSPL_BUILD_EXAMPLES`
- Build `examples/`.
- `PLUSSPL_BUILD_TESTS`
- Build `tests/`.

## Quick Commands

```bash
cmake -S . -B build -DPLUSSPL_ENABLE_INTERACTIVE_SHOW=ON -DPLUSSPL_ENABLE_POPUP_VIEWER=ON
cmake --build build -j4
./build/PlussPl/examples/2D/plusspl_2d_interactive_2d_show_demo
./build/PlussPl/examples/3D/plusspl_3d_interactive_3d_show_demo
./build/PlussPl/examples/4D/plusspl_4d_interactive_4d_encoding_demo
```

## Viewer Modes

- `show()`: best-effort interactive display. It prefers the embedded popup viewer, then the system viewer, then text preview.
- `show_popup()`: blocking popup-style display for `Figure2D`, `Figure3D`, and `View4D`. If the popup backend is unavailable it falls back to the system viewer.
- `show_window(false, ...)`: explicit non-blocking viewer launch.
- `save_svg(...)`, `save_pdf(...)`, `save_png(...)`: stable export paths for reports, notebooks, CI artifacts, and headless environments.

## Notes

- Library targets are compiled with C++20 (`target_compile_features(... cxx_std_20)`).
- `fmt` is used in internal core/rendering formatting paths; demos/tests stay standard C++ usage.
- Example outputs are written under `outputs/` rather than the repository root.
- Architecture and backlog docs live in `docs/plot_architecture.md` and `docs/engineering_backlog.md`.
