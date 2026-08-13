# Flash: High-Level Plotting Layer On PlussPl

`flash` is a convenience layer in `pluss::pl::flash` that maps concise plot-family entry
points onto PlussPl primitives.

## Header

```cpp
#include <pluss/pluss.h>
```

## Core Usage

```cpp
using namespace pluss::pl;
using namespace pluss::pl::flash;

Studio fig("flash");
auto& ax = fig.pane2();
set_theme(fig, Theme::paper());
set_context(fig, 1200.0, 780.0, 1.2, 1.1);
set_style(ax, GridStyle::Major);

scatterplot(ax, x, y);
lineplot(ax, x, y);
histplot(ax, values, 20);
regplot(ax, x, y);
heatmap(ax, gx, gy, z);
```

## Supported Plot Families

- Relational: `relplot`, `scatterplot`, `lineplot`
- Distribution: `displot`, `histplot`, `kdeplot`, `ecdfplot`, `rugplot`
- Categorical: `catplot`, `stripplot`, `swarmplot`, `boxplot`, `violinplot`, `boxenplot`, `pointplot`, `barplot`, `countplot`
- Regression: `regplot`, `lmplot`, `residplot`
- Matrix: `heatmap`, `clustermap`
- Grids: `FacetGrid`, `PairGrid`, `JointGrid`, `pairplot`, `jointplot`
- Palette helpers: `color_palette`, `hls_palette`, `husl_palette`, `cubehelix_palette`, `light_palette`, `dark_palette`, `diverging_palette`, `blend_palette`, `mpl_palette`, `xkcd_palette`, `crayon_palette`
- Styling: `set`, `set_theme`, `set_style`, `axes_style`, `set_context`, `plotting_context`, `despine`, `move_legend`
- Dataset helpers: `load_dataset`, `get_dataset_names` (`Note` names; no DataFrame dependency)
- 3D helpers: `path3`, `cloud3`, `sheet`, `lattice`, `ridge3`, `isoshell`, `spire3`
- 3D convenience aliases: `plot3D`, `scatter3D`, `plot_surface`, `plot_wireframe`, `contour3D`, `bar3d`
- Popup display: `flash::show(studio, ...)` / `studio.show(...)` (native window on macOS, GTK/WebKit on Linux when available, WebView2 on Windows when available; browser fallback otherwise). Embedded controls include zoom/reset, theme toggle, and SVG save.
