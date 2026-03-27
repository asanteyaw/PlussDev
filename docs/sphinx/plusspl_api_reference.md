# PlussPl API Reference

This reference is organized by real usage blocks.  
Each block follows: `API Signature -> Notes (inputs/outputs) -> Example -> Expected output`.

## 1) Figure Construction And Pane Creation

### API Signature

- `pl::studio(name: string = "") -> Studio`
- `Studio::pane2(title: string = "") -> Pane&`
- `Studio::pane3(title: string = "") -> Pane&`

### Notes

Inputs:
- Figure name/title strings.
- No data required at creation time.

Output:
- `Studio` is the figure container.
- `Pane&` is where traces are added.

### Example

```cpp
#include <pluss/pl.h>
namespace pl = pluss::pl;

auto fig = pl::studio("core_basics");
auto& p2 = fig.pane2("2D Pane");
auto& p3 = fig.pane3("3D Pane");
```

### Expected output

- Figure with one planar pane and one volumetric pane, ready for traces.

## 2) Layout And Figure-Level Controls

### API Signature

- `Studio::mosaic(rows: int64, cols: int64, gutter_x: double = 18.0, gutter_y: double = 18.0) -> Studio&`
- `Studio::tight_layout(enable: bool = true, padding: double = 6.0) -> Studio&`
- `Studio::headline(text: string) -> Studio&`
- `Studio::theme(theme: Theme) -> Studio&`
- `Studio::legend(dock: KeyDock) -> Studio&`

### Notes

Inputs:
- Grid dimensions and spacing.
- Theme tokens and legend docking.

Output:
- Returns `Studio&` so calls can be chained.
- Applies figure-wide layout/style rules.

### Example

```cpp
#include <pluss/pl.h>
namespace pl = pluss::pl;

auto fig = pl::studio("layout")
  .mosaic(1, 2)
  .tight_layout(true, 6.0)
  .headline("Two-pane comparison")
  .theme(pl::Theme::paper())
  .legend(pl::KeyDock::East);
```

### Expected output

- Two-pane figure with tight spacing, headline, paper theme, legend docked east.

## 3) Core 2D Pane Traces

### API Signature

- `Pane::trend(x, y) -> Trace&`
- `Pane::cloud(x, y[, size[, color]]) -> Trace&`
- `Pane::fieldmap(x, y, z[, c]) -> Trace&`
- `Pane::columns(x, y) -> Trace&`
- `Pane::bucket(values, bins: int = 12) -> Trace&`

### Notes

Inputs:
- `x/y/z` as vectors or `Series` with compatible lengths.
- Optional size/color channels for marker encoding.

Output:
- `Trace&` for per-trace styling (title/color/width/visibility).

### Example

```cpp
#include <pluss/pl.h>
namespace pl = pluss::pl;

auto fig = pl::studio("traces_2d");
auto& p = fig.pane2("Main");

p.trend(std::vector<double>{0,1,2,3}, std::vector<double>{1,3,2,5}).title("trend");
p.cloud(std::vector<double>{0,1,2,3}, std::vector<double>{1,2,1,3}).title("cloud");
p.columns(std::vector<double>{0,1,2}, std::vector<double>{3,4,2}).title("columns");
```

### Expected output

- A 2D pane containing trend, cloud, and column traces.

## 4) Statistical/Categorical Pane Traces

### API Signature

- `Pane::quartet(values) -> Trace&`
- `Pane::quartet(group, values) -> Trace&`
- `Pane::violin(values) -> Trace&`
- `Pane::violin(group, values[, options]) -> Trace&`

### Notes

Inputs:
- Numeric `values`, optionally grouped.
- `ViolinOptions` for width/inner rendering behavior.

Output:
- Distribution/comparison traces that can be styled through returned `Trace&`.

### Example

```cpp
#include <pluss/pl.h>
namespace pl = pluss::pl;

auto fig = pl::studio("dist");
auto& p = fig.pane2("Distribution");

p.quartet(std::vector<double>{2.1, 2.4, 2.8, 3.1, 3.5});
p.violin(std::vector<double>{2.1, 2.4, 2.8, 3.1, 3.5});
```

### Expected output

- Box/quartile-style and violin distribution traces on one pane.

## 5) Core 3D Pane Traces

### API Signature

- `Pane::path3(x, y, z) -> Trace&`
- `Pane::cloud3(x, y, z[, size[, color]]) -> Trace&`
- `Pane::spire3(x, y, z[, color]) -> Trace&`

### Notes

Inputs:
- 3D coordinates as vectors or `Series`.
- Optional marker size/color channels.

Output:
- Volumetric traces in a `pane3` target.

### Example

```cpp
#include <pluss/pl.h>
namespace pl = pluss::pl;

auto fig = pl::studio("traces_3d");
auto& p3 = fig.pane3("Cloud3");

p3.cloud3(
  std::vector<double>{0,1,2,3},
  std::vector<double>{0,1,0,1},
  std::vector<double>{1,3,2,4})
  .title("3D cloud");
```

### Expected output

- A 3D cloud trace rendered on a volumetric pane.

## 6) Alignment, Guides, And Annotations

### API Signature

- `Pane::align_x(group: string) -> Pane&`
- `Pane::align_y(group: string) -> Pane&`
- `Pane::rule_x(value: double, label: string = "") -> Pane&`
- `Pane::band_y(span: Range1d, label: string = "") -> Pane&`
- `Pane::pin(text: string, x: double, y: double, offset_x: double = 38.0, offset_y: double = -18.0) -> Pane&`

### Notes

Inputs:
- Alignment group names and guide coordinates.
- Optional labels for rule/band/pin annotations.

Output:
- `Pane&` for fluent annotation composition.

### Example

```cpp
#include <pluss/pl.h>
namespace pl = pluss::pl;

auto fig = pl::studio("guides");
auto& p = fig.pane2("Annotated");
p.trend(std::vector<double>{0,1,2,3}, std::vector<double>{1,2,1.5,2.8});

p.rule_x(2.0, "event")
 .band_y(pl::Range1d{1.3, 2.2}, "target")
 .pin("peak", 3.0, 2.8);
```

### Expected output

- Trend trace with one vertical rule, one y-band, and one pin annotation.

## 7) Export And Display

### API Signature

- `Studio::save_svg(path: string_view) -> void`
- `Studio::save_pdf(path: string_view, engine: PdfEngine = PdfEngine::Auto) -> void`
- `Studio::save_png(path: string_view, dpi: int64 = 144) -> void`
- `Studio::show(title: string_view = "", width: int64 = 1280, height: int64 = 820, launch: bool = true) -> bool`

### Notes

Inputs:
- output path strings and optional engine/DPI/window settings.

Output:
- File artifacts (`.svg`, `.pdf`, `.png`) and/or preview render success flag.

### Example

```cpp
#include <pluss/pl.h>
namespace pl = pluss::pl;

auto fig = pl::studio("export_demo");
fig.pane2("Line").trend(std::vector<double>{0,1,2}, std::vector<double>{1,2,1});

fig.save_svg("export_demo.svg");
fig.save_pdf("export_demo.pdf");
fig.save_png("export_demo.png", 180);
const bool ok = fig.show("PlussPl Preview", 1200, 760, true);
```

### Expected output

- SVG/PDF/PNG files created.
- `ok == true` when preview rendering succeeds.
