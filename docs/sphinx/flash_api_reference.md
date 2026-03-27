# Flash API Reference

Flash is the high-level plotting surface over PlussPl.  
This page is structured in practical blocks: `API Signature -> Notes (inputs/outputs) -> Example -> Expected output`.

## 1) Relational 2D Plots

### API Signature

- `flash::scatterplot(pane: Pane&, x, y) -> Trace&`
- `flash::lineplot(pane: Pane&, x, y) -> Trace&`
- `flash::stepplot(pane: Pane&, x, y) -> Trace&`
- `flash::relplot(pane: Pane&, x, y, kind: string_view = "scatter") -> Trace&`

### Notes

Inputs:
- `pane` target and aligned `x/y` vectors or `Series`.
- `kind` (`"scatter"` or `"line"`) for `relplot`.

Output:
- `Trace&` for styling and legend/title tuning.

### Example

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig = pl::studio("rel");
auto& p = fig.pane2("Rel");

flash::scatterplot(p, std::vector<double>{1,2,3,4}, std::vector<double>{2,1,4,3});
flash::lineplot(p, std::vector<double>{1,2,3,4}, std::vector<double>{1.1,1.8,2.6,3.3});
```

### Expected output

- Pane with one scatter trace and one line trace.

## 2) Distribution Plots

### API Signature

- `flash::histplot(pane: Pane&, values, bins: int = 12) -> Trace&`
- `flash::displot(pane: Pane&, values, bins: int = 12) -> Trace&`
- `flash::kdeplot(pane: Pane&, values, points: int64 = 128) -> Trace&`
- `flash::ecdfplot(pane: Pane&, values) -> Trace&`
- `flash::areaplot(pane: Pane&, x, y, base: double = 0.0) -> Trace&`

### Notes

Inputs:
- One-dimensional numeric values (or `x/y` for area).
- Bin count / density sample points when needed.

Output:
- Distribution-style `Trace&` objects added to the pane.

### Example

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig = pl::studio("dist");
auto& p = fig.pane2("Distribution");

flash::histplot(p, std::vector<double>{1,2,2,3,3,3,4,5}, 8);
flash::kdeplot(p, std::vector<double>{1,2,2,3,3,3,4,5}, 128);
```

### Expected output

- Histogram and KDE traces over the same values.

## 3) Categorical/Statistical Plots

### API Signature

- `flash::stripplot(pane: Pane&, group, values) -> Trace&`
- `flash::swarmplot(pane: Pane&, group, values) -> Trace&`
- `flash::boxplot(pane: Pane&, group, values) -> Trace&`
- `flash::violinplot(pane: Pane&, group, values[, options]) -> Trace&`
- `flash::boxenplot(pane: Pane&, values) -> Trace&`
- `flash::pointplot(pane: Pane&, x, y, err) -> Trace&`
- `flash::barplot(pane: Pane&, x, y[, options]) -> Trace&`
- `flash::countplot(pane: Pane&, values) -> Trace&`
- `flash::pieplot(pane: Pane&, values[, labels]) -> Trace&`

### Notes

Inputs:
- Group labels (`Note` or compatible categorical representation) and numeric values.
- Optional bar/violin options and labels.

Output:
- Categorical comparison traces returned as `Trace&`.

### Example

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig = pl::studio("cat");
auto& p = fig.pane2("Category");

flash::barplot(p,
  std::vector<double>{0,1,2},
  std::vector<double>{12,18,9},
  flash::BarPlotOptions{});
```

### Expected output

- One categorical bar plot trace on the pane.

## 4) Regression And Residual Diagnostics

### API Signature

- `flash::regplot(pane: Pane&, x, y[, options]) -> Trace&`
- `flash::lmplot(studio: Studio&, x, y[, options], title: string = "") -> Trace&`
- `flash::residplot(pane: Pane&, x, y[, options]) -> Trace&`
- `flash::qqplot(pane: Pane&, values[, options]) -> Trace&`

### Notes

Inputs:
- Numeric explanatory/response vectors (`x/y`) or sample values.
- Optional regression/QQ options.

Output:
- Diagnostic traces for fit, residual behavior, and quantile conformity.

### Example

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig = pl::studio("diag");
auto& p = fig.pane2("Regression");

flash::regplot(p,
  std::vector<double>{1,2,3,4,5},
  std::vector<double>{2,4,5,8,10},
  flash::RegPlotOptions{});
```

### Expected output

- Scatter + fitted line diagnostic on one pane.

## 5) Matrix, Joint, And Grid Workflows

### API Signature

- `flash::heatmap(pane: Pane&, x, y, z[, options]) -> Trace&`
- `flash::clustermap(pane: Pane&, x, y, z[, options]) -> Trace&`
- `flash::catplot(pane: Pane&, group, values[, hue/options]) -> Trace&`
- `flash::FacetGrid(studio: Studio&, rows: int64, cols: int64, gx: double = 18.0, gy: double = 18.0) -> Studio&`
- `flash::PairGrid(studio: Studio&, dimensions: int64, gx: double = 14.0, gy: double = 14.0) -> Studio&`
- `flash::JointGrid(studio: Studio&, gx: double = 14.0, gy: double = 14.0) -> Studio&`
- `flash::pairplot(studio: Studio&, columns|dataset|matrix[, options]) -> Studio&`
- `flash::jointplot(studio: Studio&, x, y[, kind/options]) -> Studio&`

### Notes

Inputs:
- Matrix-like features or aligned `(x, y, z)` grids.
- Grid options controlling layout/behavior.

Output:
- Either `Trace&` (single pane) or `Studio&` (multi-pane composition).

### Example

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig = pl::studio("pair");
std::vector<pluss::Series> cols{
  pluss::make_series(std::vector<double>{1,2,3,4,5}),
  pluss::make_series(std::vector<double>{2,1,3,5,4}),
  pluss::make_series(std::vector<double>{5,4,3,2,1})
};

flash::pairplot(fig, cols, flash::PairPlotOptions{});
```

### Expected output

- Pair-grid figure with diagonal/off-diagonal panel mappings.

## 6) 3D And 4D Surface

### API Signature

- `flash::plot3D(pane: Pane&, x, y, z) -> Trace&`
- `flash::scatter3D(pane: Pane&, x, y, z) -> Trace&`
- `flash::plot_surface(pane: Pane&, x, y, z) -> Trace&`
- `flash::plot_wireframe(pane: Pane&, x, y, z) -> Trace&`
- `flash::contour3D(pane: Pane&, x, y, z, level_count: int = 8) -> Trace&`
- `flash::bar3d(pane: Pane&, x, y, z) -> Trace&`
- `flash::hist3d(pane|studio, x, y[, options]) -> Trace&`
- `flash::path4(studio: Studio&, x, y, z, w[, options]) -> Trace&`
- `flash::cloud4(studio: Studio&, x, y, z, w[, options]) -> Trace&`
- `flash::animate4(x, y, z, w[, options]) -> bool`
- `flash::export4_frames(x, y, z, w[, options]) -> bool`

### Notes

Inputs:
- 3D: aligned `x/y/z` data.
- 4D: aligned `x/y/z/w` data plus projection/animation options.

Output:
- 3D/4D traces or animation/export success flags.

### Example

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig3 = pl::studio("flash3d");
auto& p3 = fig3.pane3("3D");

flash::plot3D(p3,
  std::vector<double>{0,1,2,3},
  std::vector<double>{0,1,0,1},
  std::vector<double>{1,3,2,4});
```

### Expected output

- One 3D trace rendered on volumetric pane.

## 7) Theme, Context, Legend, And Render Helpers

### API Signature

- `flash::set(studio: Studio&, theme: Theme = Theme::paper()) -> Studio&`
- `flash::set_theme(studio: Studio&, theme: Theme = Theme::paper()) -> Studio&`
- `flash::set_theme(studio: Studio&, options: FlashThemeOptions) -> Studio&`
- `flash::set_context(studio: Studio&, context: FlashContext) -> Studio&`
- `flash::plotting_context(studio: Studio&, context: FlashContext) -> Studio&`
- `flash::tight_layout(studio: Studio&, padding: double = 6.0) -> Studio&`
- `flash::move_legend(studio: Studio&, dock: KeyDock) -> Studio&`
- `flash::legend(studio: Studio&, dock: KeyDock) -> Studio&`
- `flash::legend_frame(studio: Studio&, frame: KeyFrame) -> Studio&`
- `flash::show(studio: Studio&, title: string_view = "", width: int64 = 1280, height: int64 = 820, launch: bool = true) -> bool`
- `flash::save_png(studio: Studio&, path: string_view, dpi: int64 = 144) -> void`

### Notes

Inputs:
- Figure object + style/context/layout/dock values.

Output:
- Updated `Studio&` for composition.
- Display/export success via `bool` or saved artifact.

### Example

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig = pl::studio("styled");
flash::set_theme(fig, flash::FlashThemeOptions{
  .style = flash::FlashStyle::Lightgrid,
  .context = flash::FlashContext::Notebook,
});
flash::tight_layout(fig, 6.0);
flash::move_legend(fig, pl::KeyDock::East);
flash::save_png(fig, "styled.png", 180);
```

### Expected output

- Theme/context applied, layout tightened, legend moved, and PNG saved.
