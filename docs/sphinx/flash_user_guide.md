# Flash User Guide

Flash is the high-level plotting layer on top of PlussPl core panes/traces.

## API Signature

- `#include <pluss/pl/flash.h>`
- namespace: `pluss::pl::flash`

## Notes

Use Flash when you want concise plotting syntax (`scatterplot`, `lineplot`, `pairplot`, `jointplot`, etc.) with Pluss-native types.

## Inputs

- vectors, `Series`, `Note`, `Timeline`, `TimeIndex`
- optional style/theme options

## Output

- `Trace&` for single-trace APIs
- `Studio&` for grid/workflow APIs

## Options

- Theme/style: `FlashStyle`, `FlashPalette`, `FlashContext`, `FlashThemeOptions`
- Plot options: `RegPlotOptions`, `HeatmapOptions`, `ClusterMapOptions`, `PairPlotOptions`, `JointPlotOptions`, ...

## Examples

### 1) Quick 2D relational plots

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig = pl::studio("flash_rel");
auto& p = fig.pane2("Rel");

flash::scatterplot(p, std::vector<double>{1,2,3,4}, std::vector<double>{2,1,4,3});
flash::lineplot(p, std::vector<double>{1,2,3,4}, std::vector<double>{1.5,2.2,2.9,3.5});

fig.save_png("flash_rel.png", 180);
```

### Expected output

- A pane with both scatter and line traces
- PNG export at 180 DPI

### 2) Distribution + regression helpers

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig = pl::studio("flash_stats");
auto& left = fig.pane2("Distribution");
auto& right = fig.pane2("Regression");

flash::histplot(left, std::vector<double>{1,2,2,3,3,3,4,5}, 8);
flash::regplot(right,
               std::vector<double>{1,2,3,4,5},
               std::vector<double>{2,4,5,8,10},
               flash::RegPlotOptions{});

fig.save_svg("flash_stats.svg");
```

### Expected output

- Left pane histogram
- Right pane regression scatter + fit overlay

### 3) Matrix/correlation style plot

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig = pl::studio("flash_heat");
auto& p = fig.pane2("Heatmap");

std::vector<double> x{0,1,2,0,1,2,0,1,2};
std::vector<double> y{0,0,0,1,1,1,2,2,2};
std::vector<double> z{1,2,3,2,3,4,3,4,5};

flash::heatmap(p, x, y, z, flash::HeatmapOptions{});
fig.save_pdf("flash_heat.pdf");
```

### Expected output

- Heatmap rendered from `x/y/z` grid points
- PDF export

### 4) High-level pair/joint workflow

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig = pl::studio("flash_pair_joint");
std::vector<pluss::Series> cols{
    pluss::make_series(std::vector<double>{1,2,3,4,5}),
    pluss::make_series(std::vector<double>{2,1,3,5,4}),
    pluss::make_series(std::vector<double>{5,4,3,2,1})
};

flash::pairplot(fig, cols, flash::PairPlotOptions{});
fig.save_svg("flash_pairplot.svg");
```

### Expected output

- Pair-grid figure with diagonals/off-diagonals generated from columns

### 5) 3D/4D helpers

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig3 = pl::studio("flash_3d");
auto& p3 = fig3.pane3("3D");
flash::plot3D(p3,
              std::vector<double>{0,1,2,3},
              std::vector<double>{0,1,0,1},
              std::vector<double>{1,3,2,4});
fig3.save_png("flash_3d.png");
```

### Expected output

- 3D trace rendered on volumetric pane

## Style helpers

```cpp
#include <pluss/pl/flash.h>

namespace pl = pluss::pl;
namespace flash = pluss::pl::flash;

auto fig = pl::studio("styled");
flash::set_theme(fig, flash::FlashThemeOptions{
    .style = flash::FlashStyle::Lightgrid,
    .context = flash::FlashContext::Notebook,
    .borderless = false,
});
flash::tight_layout(fig, 6.0);
flash::move_legend(fig, pl::KeyDock::East);
```

### Expected output

- Figure-level theme/context applied
- Tight layout and legend docking active
