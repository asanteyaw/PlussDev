# PlussPl

`PlussPl` is organized as a header-first plotting library with modular internals and thin public facades.

## Layout

- `include/pluss/plotism/`
- Public API surface and forwarding headers.
- `src/plotism/`
- Internal implementation headers grouped by module.
- `src/plotism/modules/core.h`
- Common utilities, formatting, escaping, and file IO helpers.
- `src/plotism/modules/{axes.h,style.h,trace.h}`
- Data/model objects for axes, style, traces.
- `src/plotism/modules/{figure2d.h,figure3d.h,figure.h}`
- Figure API and per-dimension behavior.
- `src/plotism/modules/backend/`
- Backend abstraction and implementations.
- `src/plotism/modules/backend/svg/`
- SVG renderer primitives + 2D/3D renderers.
- `src/plotism/modules/backend/pdf/`
- PDF renderer primitives + 2D/3D renderers.
- `examples/`
- Runnable demos by dimension.
- `tests/`
- Executable tests by dimension.

## Build Flags

- `PLUSSPL_ENABLE_INTERACTIVE_SHOW`
- `ON`: `Figure::show()` tries system viewer (`open`, `xdg-open`, `start`) and falls back to text preview.
- `OFF`: `Figure::show()` always prints text preview.
- `PLUSSPL_BUILD_EXAMPLES`
- Build `examples/`.
- `PLUSSPL_BUILD_TESTS`
- Build `tests/`.

## Quick Commands

```bash
cmake -S . -B build -DPLUSSPL_ENABLE_INTERACTIVE_SHOW=ON
cmake --build build -j4
./build/PlussPl/examples/2D/plusspl_2d_interactive_show_demo
./build/PlussPl/examples/3D/plusspl_3d_interactive_show_demo
./build/PlussPl/tests/2D/plusspl_test_2d_test_export_pdf_and_show
./build/PlussPl/tests/3D/plusspl_test_3d_test_export_pdf_and_show
```

## Notes

- Library targets are compiled with C++20 (`target_compile_features(... cxx_std_20)`).
- `fmt` is used in core formatting and demo/test output; headers are consumed via `ExLibs/fmt/include`.
