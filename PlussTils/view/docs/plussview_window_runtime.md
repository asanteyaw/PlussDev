# PlussTilsView Window Runtime (No WebView Delegation)

## Principle
PlussTilsView will own the render/runtime path directly.
Existing OS-specific web viewers remain as migration fallback only.

## MATLAB-Inspired Design Cues Applied
- `FigureController`: central lifecycle ownership.
- `FigurePlatformHostFactory`: explicit host selection and capability checks.
- `FigureServices`/`webwindowmanager`: typed channels, registry, deterministic cleanup.

## Implemented in this tranche
- `pluss::view::window::Scene`
- `pluss::view::window::PixelCanvas`
- `pluss::view::window::SoftwareRenderer`
- `pluss::view::window::ViewerRuntime`
- `pluss::view::window::IWindowShell` + `NullWindowShell` contract
- `make_window_shell(policy)` factory + native `CocoaWindowShell` (macOS)
  - default policy is strict portable (`PortableOnly`)
  - native path is explicit (`NativePreferred`/`NativeOnly`) or env opt-in `PLUSSTILS_VIEW_WINDOW_NATIVE=1`
- math-aware text layout (`layout_math_text`) and bitmap raster text rendering
- line-plot scene builder (`build_line_plot_scene`)
- Demo target: `plusstils_view_window_runtime_demo`
- Demo target: `plusstils_view_window_first_plot_demo`
- Demo target: `plusstils_view_window_popup_showcase_demo` (native popup showcase, disabled by default and behind explicit CMake opt-in)

## Immediate Gap List
1. No GPU backend yet.
2. Text rendering is bitmap-only and currently Latin-focused (needs full UTF-8 shaping + font fallback).
3. Linux and Windows shell adapters are not implemented yet (macOS shell is now wired).
4. No input model (mouse/keyboard/focus/resize) yet.
5. No direct PNG/SVG/PDF writer from in-house scene/frame yet.

## Next aggressive families
1. Renderer family:
- clipping, anti-aliased stroke/fill, marker and poly primitives, gradient fills.

2. Text/math family:
- UTF-8 shaping, fallback fonts, baseline metrics, superscript/subscript/fraction layout.

3. Window/input family:
- independent shell contract + event pump + DPI scaling and shortcuts.

4. Export/testing family:
- deterministic export + golden image regression + stress tests.

## Compressed estimate
- Original full in-house estimate: 40-60+ patches.
- With high-impact family bundling: 24-36 patches.
