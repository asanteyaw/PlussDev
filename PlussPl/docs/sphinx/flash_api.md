# Flash Plotting API

`pluss::pl::flash` provides high-level plotting families built on PlussPl primitives.

Header:

```cpp
#include <pluss/pl/flash.h>
```

## Data containers and matrix helpers

```cpp
struct Dataset {
  std::optional<Note> numeric_names;
  std::vector<Series> numeric_values;
  std::optional<Note> note_names;
  std::vector<Note> note_values;

  std::optional<Series> numeric(std::string_view name) const;
  std::optional<Note> note(std::string_view name) const;
};

enum class Axis : std::uint8_t { Cols, Rows };

struct MatrixFrameOptions {
  Axis feature_axis{Axis::Cols};
  std::vector<std::string> feature_names{};
};

struct MatrixJointOptions {
  Axis feature_axis{Axis::Cols};
  std::int64_t x_feature{0};
  std::int64_t y_feature{1};
};

Dataset make_dataset(std::vector<Series> numeric_values,
                     std::vector<std::string> numeric_names = {});
Dataset make_dataset(const Series& matrix,
                     std::vector<std::string> feature_names = {},
                     Axis feature_axis = Axis::Cols);
```

## 2D relational and distribution families

Primary functions:

```cpp
scatterplot(pane, x, y)
lineplot(pane, x, y)
stepplot(pane, x, y)
relplot(pane_or_studio, x, y, kind)

histplot(pane, values, bins)
displot(pane, values, bins)
kdeplot(pane, values[, points])
ecdfplot(pane, values)
areaplot(pane, x, y[, base])
rugplot(pane, values[, height])
```

## Categorical families

```cpp
barplot(pane, x, y, BarPlotOptions)
countplot(pane, values_or_categories)
pieplot(pane, values[, labels])

stripplot(pane, group, values)
swarmplot(pane, group, values)
boxplot(pane, values | group+values)
violinplot(pane, values | group+values[, ViolinOptions])
boxenplot(pane, values)
pointplot(pane, x, y, err)
whiskplot(pane, x, y, ...)
```

Advanced categorical API:

```cpp
enum class CatPlotKind { Strip, Swarm, Box, Violin, Bar, Count };
struct CatPlotOptions { ... };
catplot(pane, group, values[, hue], CatPlotOptions)
```

## Regression and diagnostics

```cpp
struct RegPlotOptions { ... };
struct QqPlotOptions { ... };
struct ResidHistPlotOptions { ... };

regplot(pane, x, y[, RegPlotOptions])
lmplot(studio, x, y[, RegPlotOptions])
residplot(pane, x, y[, RegPlotOptions])
qqplot(pane_or_studio, values[, QqPlotOptions])
resid_histplot(pane_or_studio, x, y[, ResidHistPlotOptions])
```

## Heatmap and cluster map

```cpp
struct HeatmapOptions {
  bool annotate_levels;
  bool scale_guide;
  bool borderless;
  std::optional<double> vmin, vmax;
  std::optional<Series> mask;
};

struct ClusterMapOptions {
  bool cluster_rows;
  bool cluster_cols;
  HeatmapOptions heatmap;
};

heatmap(pane, x, y, z[, HeatmapOptions])
clustermap(pane, x, y, z[, ClusterMapOptions])
```

## Grid, pair, and joint families

Layout helpers:

```cpp
FacetGrid(studio, rows, cols)
PairGrid(studio, dimensions)
JointGrid(studio)
```

Typed grid options:

```cpp
struct RelGridPlotOptions { ... };
struct CatGridPlotOptions { ... };
struct DisGridPlotOptions { ... };

relgridplot(studio, xs, ys[, options])
catgridplot(studio, groups, values[, hues][, options])
disgridplot(studio, values[, options])
```

Pair/joint:

```cpp
enum class PairPlotTriangle { Full, Lower, Upper };
enum class PairPlotKind { Scatter, Reg };
enum class PairPlotDiagKind { Histogram, Density };
struct PairPlotOptions { ... };

pairplot(studio, columns | Dataset | matrix[, MatrixFrameOptions][, PairPlotOptions])

struct JointPlotOptions { ... };
jointplot(studio, x, y[, kind|JointPlotOptions])
jointplot(studio, Dataset, x_name, y_name[, kind|JointPlotOptions])
jointplot(studio, matrix, x_name, y_name[, MatrixFrameOptions][, kind|JointPlotOptions])
```

## Time-series style diagnostics

```cpp
struct LagPlotOptions { ... };
struct AutocorrelationPlotOptions { ... };
struct AcfPlotOptions { ... };
struct PacfPlotOptions { ... };
struct CcfPlotOptions { ... };
struct ResidAcfPlotOptions { ... };
struct BootstrapPlotOptions { ... };

lagplot(studio, values[, LagPlotOptions])
actplot(studio, values[, AutocorrelationPlotOptions])
acfplot(studio, values[, AcfPlotOptions])
pacfplot(studio, values[, PacfPlotOptions])
ccfplot(studio, x, y[, CcfPlotOptions])
resid_acfplot(studio, x, y[, ResidAcfPlotOptions])
bootstrapplot(studio, values[, BootstrapPlotOptions])
```

## Matrix-driven feature plots

```cpp
struct TriplotOptions { ... };
struct ParallelCoordinatesOptions { ... };
struct AndrewsCurvesOptions { ... };
struct RadvizOptions { ... };

triplot(studio, columns | Dataset | matrix[, MatrixFrameOptions][, TriplotOptions])
parallelplot(studio, columns | Dataset | matrix[, MatrixFrameOptions][, ParallelCoordinatesOptions])
andrews_curves(studio, columns | Dataset | matrix[, MatrixFrameOptions][, AndrewsCurvesOptions])
radviz(studio, columns | Dataset | matrix[, MatrixFrameOptions][, RadvizOptions])
```

## Summary and utility functions

```cpp
struct SummaryOptions { ... };
summaryplot(studio, columns, labels[, SummaryOptions])

PaletteRamp color_palette(std::string_view name = "ocean");
PaletteRamp blend_palette(std::vector<Rgba> colors, std::string name = "blend");
PaletteRamp hls_palette(...);
PaletteRamp husl_palette(...);
PaletteRamp cubehelix_palette(...);
PaletteRamp light_palette(...);
PaletteRamp dark_palette(...);
PaletteRamp diverging_palette(...);
PaletteRamp mpl_palette(...);
PaletteRamp xkcd_palette();
PaletteRamp crayon_palette();

Note get_dataset_names();
std::optional<Dataset> load_dataset(std::string_view name);
```

## 3D and 4D entry points

See [Flash 3D and 4D API](flash_scene_animation_api.md) for option structs and behavior details.

Quick list:

```cpp
path3, cloud3, sheet, lattice, spire3, ridge3, isoshell, hist3d
path4, cloud4
animate3, animate4, export4_frames
scene3d, trace3d
```
