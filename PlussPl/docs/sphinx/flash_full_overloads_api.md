# Flash Full Overload Reference

This page expands flash to one section per callable form (all overloads), using clean snippets and real generated outputs.

Pattern per section: Signature -> Notes -> Example -> Output.

## Complete Overload Catalog

### `acfplot (single form)`

#### Signature

```cpp
Studio& acfplot(self: Studio, SeriesLike values, const AcfPlotOptions& options = {});
```

#### Notes

Autocorrelation bars across lags; useful for seasonality and persistence checks.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/acfplot.cpp
:language: cpp
```

#### Output

```{only} html
![acfplot output](_static/plots/methods/acfplot.svg)
```

```{only} not html
![acfplot output](_static/plots/methods/acfplot.png)
```

`Source section`: `flash_api.md`

### `actplot (single form)`

#### Signature

```cpp
Studio& actplot(self: Studio, SeriesLike values, const AutocorrelationPlotOptions& options = {});
```

#### Notes

Convenience autocorrelation wrapper with a compact options surface.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/actplot.cpp
:language: cpp
```

#### Output

```{only} html
![actplot output](_static/plots/methods/actplot.svg)
```

```{only} not html
![actplot output](_static/plots/methods/actplot.png)
```

`Source section`: `flash_api.md`

### `andrews_curves (overload 1)`

#### Signature

```cpp
Studio& andrews_curves(self: Studio, pluss::msharp::RangeView::Series columns, const AndrewsCurvesOptions& options = {});
```

#### Notes

Maps each row to a smooth periodic curve for multivariate separation analysis.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/andrews_curves.cpp
:language: cpp
```

#### Output

```{only} html
![andrews_curves output](_static/plots/methods/andrews_curves.svg)
```

```{only} not html
![andrews_curves output](_static/plots/methods/andrews_curves.png)
```

`Source section`: `flash_api.md`

### `andrews_curves (overload 2)`

#### Signature

```cpp
Studio& andrews_curves(self: Studio, pluss::msharp::RangeView::Series columns, const Note& class_labels, const AndrewsCurvesOptions& options = {});
```

#### Notes

Maps each row to a smooth periodic curve for multivariate separation analysis.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/andrews_curves.cpp
:language: cpp
```

#### Output

```{only} html
![andrews_curves output](_static/plots/methods/andrews_curves.svg)
```

```{only} not html
![andrews_curves output](_static/plots/methods/andrews_curves.png)
```

`Source section`: `flash_api.md`

### `andrews_curves (overload 3)`

#### Signature

```cpp
Studio& andrews_curves(self: Studio, const Dataset& data, const AndrewsCurvesOptions& options = {});
```

#### Notes

Maps each row to a smooth periodic curve for multivariate separation analysis.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/andrews_curves.cpp
:language: cpp
```

#### Output

```{only} html
![andrews_curves output](_static/plots/methods/andrews_curves.svg)
```

```{only} not html
![andrews_curves output](_static/plots/methods/andrews_curves.png)
```

`Source section`: `flash_api.md`

### `andrews_curves (overload 4)`

#### Signature

```cpp
Studio& andrews_curves(self: Studio, const Series& matrix, const MatrixFrameOptions& frame_options, const AndrewsCurvesOptions& options = {});
```

#### Notes

Maps each row to a smooth periodic curve for multivariate separation analysis.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/andrews_curves.cpp
:language: cpp
```

#### Output

```{only} html
![andrews_curves output](_static/plots/methods/andrews_curves.svg)
```

```{only} not html
![andrews_curves output](_static/plots/methods/andrews_curves.png)
```

`Source section`: `flash_api.md`

### `andrews_curves (overload 5)`

#### Signature

```cpp
Studio& andrews_curves(self: Studio, const Series& matrix, pluss::msharp::RangeView::Of<std::string_view> feature_names, Axis feature_axis, const AndrewsCurvesOptions& options = {});
```

#### Notes

Maps each row to a smooth periodic curve for multivariate separation analysis.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/andrews_curves.cpp
:language: cpp
```

#### Output

```{only} html
![andrews_curves output](_static/plots/methods/andrews_curves.svg)
```

```{only} not html
![andrews_curves output](_static/plots/methods/andrews_curves.png)
```

`Source section`: `flash_api.md`

### `andrews_curves (overload 6)`

#### Signature

```cpp
Studio& andrews_curves(self: Studio, const Series& matrix, const AndrewsCurvesOptions& options = {});
```

#### Notes

Maps each row to a smooth periodic curve for multivariate separation analysis.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/andrews_curves.cpp
:language: cpp
```

#### Output

```{only} html
![andrews_curves output](_static/plots/methods/andrews_curves.svg)
```

```{only} not html
![andrews_curves output](_static/plots/methods/andrews_curves.png)
```

`Source section`: `flash_api.md`

### `areaplot (single form)`

#### Signature

```cpp
Trace& areaplot(self: Pane, SeriesLike x, SeriesLike y, double base = 0.0);
```

#### Notes

Fills the area between the signal and a baseline.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/areaplot.cpp
:language: cpp
```

#### Output

```{only} html
![areaplot output](_static/plots/methods/areaplot.svg)
```

```{only} not html
![areaplot output](_static/plots/methods/areaplot.png)
```

`Source section`: `flash_api.md`

### `bar3d (single form)`

#### Signature

```cpp
Trace& bar3d(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

3D bar-like rendering alias over `spire3`.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/bar3d.cpp
:language: cpp
```

#### Output

```{only} html
![bar3d output](_static/plots/methods/bar3d.svg)
```

```{only} not html
![bar3d output](_static/plots/methods/bar3d.png)
```

`Source section`: `flash_api.md`

### `barplot (overload 1)`

#### Signature

```cpp
Trace& barplot(self: Pane, SeriesLike x, SeriesLike y, const BarPlotOptions& options = {});
```

#### Notes

Categorical or numeric bars, with optional horizontal orientation.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/barplot.cpp
:language: cpp
```

#### Output

```{only} html
![barplot output](_static/plots/methods/barplot.svg)
```

```{only} not html
![barplot output](_static/plots/methods/barplot.png)
```

`Source section`: `flash_api.md`

### `barplot (overload 2)`

#### Signature

```cpp
Trace& barplot(self: Pane, const Note& categories, const Series& values, const BarPlotOptions& options = {});
```

#### Notes

Categorical or numeric bars, with optional horizontal orientation.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/barplot.cpp
:language: cpp
```

#### Output

```{only} html
![barplot output](_static/plots/methods/barplot.svg)
```

```{only} not html
![barplot output](_static/plots/methods/barplot.png)
```

`Source section`: `flash_api.md`

### `bootstrapplot (single form)`

#### Signature

```cpp
Studio& bootstrapplot(self: Studio, SeriesLike values, const BootstrapPlotOptions& options = {});
```

#### Notes

Resampling-based distribution of a summary statistic (mean/median/std).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/bootstrapplot.cpp
:language: cpp
```

#### Output

```{only} html
![bootstrapplot output](_static/plots/methods/bootstrapplot.svg)
```

```{only} not html
![bootstrapplot output](_static/plots/methods/bootstrapplot.png)
```

`Source section`: `flash_api.md`

### `boxenplot (single form)`

#### Signature

```cpp
Trace& boxenplot(self: Pane, SeriesLike values);
```

#### Notes

Letter-value style alias currently routed through boxplot behavior.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/boxenplot.cpp
:language: cpp
```

#### Output

```{only} html
![boxenplot output](_static/plots/methods/boxenplot.svg)
```

```{only} not html
![boxenplot output](_static/plots/methods/boxenplot.png)
```

`Source section`: `flash_api.md`

### `boxplot (overload 1)`

#### Signature

```cpp
Trace& boxplot(self: Pane, SeriesLike values);
```

#### Notes

Quartile/whisker summary for distribution comparison.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/boxplot.cpp
:language: cpp
```

#### Output

```{only} html
![boxplot output](_static/plots/methods/boxplot.svg)
```

```{only} not html
![boxplot output](_static/plots/methods/boxplot.png)
```

`Source section`: `flash_api.md`

### `boxplot (overload 2)`

#### Signature

```cpp
Trace& boxplot(self: Pane, const Note& categories, const Series& values);
```

#### Notes

Quartile/whisker summary for distribution comparison.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/boxplot.cpp
:language: cpp
```

#### Output

```{only} html
![boxplot output](_static/plots/methods/boxplot.svg)
```

```{only} not html
![boxplot output](_static/plots/methods/boxplot.png)
```

`Source section`: `flash_api.md`

### `boxplot (overload 3)`

#### Signature

```cpp
Trace& boxplot(self: Pane, SeriesLike group, SeriesLike values);
```

#### Notes

Quartile/whisker summary for distribution comparison.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/boxplot.cpp
:language: cpp
```

#### Output

```{only} html
![boxplot output](_static/plots/methods/boxplot.svg)
```

```{only} not html
![boxplot output](_static/plots/methods/boxplot.png)
```

`Source section`: `flash_api.md`

### `catgridplot (overload 1)`

#### Signature

```cpp
Studio& catgridplot(self: Studio, pluss::msharp::RangeView::Note groups, pluss::msharp::RangeView::Series values, const CatGridPlotOptions& options = {});
```

#### Notes

Facet grid for repeated categorical comparisons across slices.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/catgridplot.cpp
:language: cpp
```

#### Output

```{only} html
![catgridplot output](_static/plots/methods/catgridplot.svg)
```

```{only} not html
![catgridplot output](_static/plots/methods/catgridplot.png)
```

`Source section`: `flash_api.md`

### `catgridplot (overload 2)`

#### Signature

```cpp
Studio& catgridplot(self: Studio, pluss::msharp::RangeView::Note groups, pluss::msharp::RangeView::Series values, pluss::msharp::RangeView::Note hues, const CatGridPlotOptions& options = {});
```

#### Notes

Facet grid for repeated categorical comparisons across slices.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/catgridplot.cpp
:language: cpp
```

#### Output

```{only} html
![catgridplot output](_static/plots/methods/catgridplot.svg)
```

```{only} not html
![catgridplot output](_static/plots/methods/catgridplot.png)
```

`Source section`: `flash_api.md`

### `catplot (overload 1)`

#### Signature

```cpp
Trace& catplot(self: Pane, const Note& group, const Series& values, const CatPlotOptions& options = {});
```

#### Notes

Unified categorical interface (`strip`, `swarm`, `box`, `violin`, `bar`, `count`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/catplot.cpp
:language: cpp
```

#### Output

```{only} html
![catplot output](_static/plots/methods/catplot.svg)
```

```{only} not html
![catplot output](_static/plots/methods/catplot.png)
```

`Source section`: `flash_api.md`

### `catplot (overload 2)`

#### Signature

```cpp
Trace& catplot(self: Pane, const Note& group, const Series& values, const Note& hue, const CatPlotOptions& options = {});
```

#### Notes

Unified categorical interface (`strip`, `swarm`, `box`, `violin`, `bar`, `count`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/catplot.cpp
:language: cpp
```

#### Output

```{only} html
![catplot output](_static/plots/methods/catplot.svg)
```

```{only} not html
![catplot output](_static/plots/methods/catplot.png)
```

`Source section`: `flash_api.md`

### `catplot (overload 3)`

#### Signature

```cpp
Trace& catplot(self: Pane, const Note& group, const Series& values, std::string_view kind = "strip");
```

#### Notes

Unified categorical interface (`strip`, `swarm`, `box`, `violin`, `bar`, `count`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/catplot.cpp
:language: cpp
```

#### Output

```{only} html
![catplot output](_static/plots/methods/catplot.svg)
```

```{only} not html
![catplot output](_static/plots/methods/catplot.png)
```

`Source section`: `flash_api.md`

### `catplot (overload 4)`

#### Signature

```cpp
Trace& catplot(self: Pane, const Note& group, const Series& values, const Note& hue, std::string_view kind = "strip");
```

#### Notes

Unified categorical interface (`strip`, `swarm`, `box`, `violin`, `bar`, `count`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/catplot.cpp
:language: cpp
```

#### Output

```{only} html
![catplot output](_static/plots/methods/catplot.svg)
```

```{only} not html
![catplot output](_static/plots/methods/catplot.png)
```

`Source section`: `flash_api.md`

### `catplot (overload 5)`

#### Signature

```cpp
Trace& catplot(self: Pane, SeriesLike group, SeriesLike values, std::string_view kind = "strip");
```

#### Notes

Unified categorical interface (`strip`, `swarm`, `box`, `violin`, `bar`, `count`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/catplot.cpp
:language: cpp
```

#### Output

```{only} html
![catplot output](_static/plots/methods/catplot.svg)
```

```{only} not html
![catplot output](_static/plots/methods/catplot.png)
```

`Source section`: `flash_api.md`

### `ccfplot (single form)`

#### Signature

```cpp
Studio& ccfplot(self: Studio, SeriesLike x, SeriesLike y, const CcfPlotOptions& options = {});
```

#### Notes

Cross-correlation against positive/negative lags for two signals.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/ccfplot.cpp
:language: cpp
```

#### Output

```{only} html
![ccfplot output](_static/plots/methods/ccfplot.svg)
```

```{only} not html
![ccfplot output](_static/plots/methods/ccfplot.png)
```

`Source section`: `flash_api.md`

### `cloud3 (overload 1)`

#### Signature

```cpp
Trace& cloud3(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

3D scatter cloud with optional point-size and color channels.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/cloud3.cpp
:language: cpp
```

#### Output

```{only} html
![cloud3 output](_static/plots/methods/cloud3.svg)
```

```{only} not html
![cloud3 output](_static/plots/methods/cloud3.png)
```

`Source section`: `flash_api.md`

### `cloud3 (overload 2)`

#### Signature

```cpp
Trace& cloud3(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, SeriesLike size);
```

#### Notes

3D scatter cloud with optional point-size and color channels.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/cloud3.cpp
:language: cpp
```

#### Output

```{only} html
![cloud3 output](_static/plots/methods/cloud3.svg)
```

```{only} not html
![cloud3 output](_static/plots/methods/cloud3.png)
```

`Source section`: `flash_api.md`

### `cloud3 (overload 3)`

#### Signature

```cpp
Trace& cloud3(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, SeriesLike size, SeriesLike color);
```

#### Notes

3D scatter cloud with optional point-size and color channels.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/cloud3.cpp
:language: cpp
```

#### Output

```{only} html
![cloud3 output](_static/plots/methods/cloud3.svg)
```

```{only} not html
![cloud3 output](_static/plots/methods/cloud3.png)
```

`Source section`: `flash_api.md`

### `cloud3 (overload 4)`

#### Signature

```cpp
Trace& cloud3(self: Studio, SeriesLike x, SeriesLike y, SeriesLike z, std::string title = {});
```

#### Notes

3D scatter cloud with optional point-size and color channels.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/cloud3.cpp
:language: cpp
```

#### Output

```{only} html
![cloud3 output](_static/plots/methods/cloud3.svg)
```

```{only} not html
![cloud3 output](_static/plots/methods/cloud3.png)
```

`Source section`: `flash_api.md`

### `cloud4 (overload 1)`

#### Signature

```cpp
Trace& cloud4(self: Pane
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike, w: SeriesLike, options: HyperPlotOptions = {});

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/cloud4.cpp
:language: cpp
```

#### Output

```{only} html
![cloud4 output](_static/plots/methods/cloud4.svg)
```

```{only} not html
![cloud4 output](_static/plots/methods/cloud4.png)
```

`Source section`: `flash_api.md`

### `cloud4 (overload 2)`

#### Signature

```cpp
Trace& cloud4(self: Pane|Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike, w: SeriesLike, options: HyperPlotOptions = {}, title: std::string = {});
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike, w: SeriesLike, options: HyperPlotOptions = {});

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/cloud4.cpp
:language: cpp
```

#### Output

```{only} html
![cloud4 output](_static/plots/methods/cloud4.svg)
```

```{only} not html
![cloud4 output](_static/plots/methods/cloud4.png)
```

`Source section`: `flash_api.md`

### `clustermap (overload 1)`

#### Signature

```cpp
Trace& clustermap(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

Cluster-aware heatmap that reorders rows/columns by similarity.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/clustermap.cpp
:language: cpp
```

#### Output

```{only} html
![clustermap output](_static/plots/methods/clustermap.svg)
```

```{only} not html
![clustermap output](_static/plots/methods/clustermap.png)
```

`Source section`: `flash_api.md`

### `clustermap (overload 2)`

#### Signature

```cpp
Trace& clustermap(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, const ClusterMapOptions& options);
```

#### Notes

Cluster-aware heatmap that reorders rows/columns by similarity.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/clustermap.cpp
:language: cpp
```

#### Output

```{only} html
![clustermap output](_static/plots/methods/clustermap.svg)
```

```{only} not html
![clustermap output](_static/plots/methods/clustermap.png)
```

`Source section`: `flash_api.md`

### `contour3d (single form)`

#### Signature

```cpp
Trace& contour3D(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, int level_count = 8);
```

#### Notes

3D contour-like ridges (alias over `ridge3`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/contour3D.cpp
:language: cpp
```

#### Output

```{only} html
![contour3D output](_static/plots/methods/contour3D.svg)
```

```{only} not html
![contour3D output](_static/plots/methods/contour3D.png)
```

`Source section`: `flash_api.md`

### `countplot (overload 1)`

#### Signature

```cpp
Trace& countplot(self: Pane, SeriesLike values, int bins = 12);
```

#### Notes

Frequency chart for numeric bins or category counts.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/countplot.cpp
:language: cpp
```

#### Output

```{only} html
![countplot output](_static/plots/methods/countplot.svg)
```

```{only} not html
![countplot output](_static/plots/methods/countplot.png)
```

`Source section`: `flash_api.md`

### `countplot (overload 2)`

#### Signature

```cpp
Trace& countplot(self: Pane, const Note& categories);
```

#### Notes

Frequency chart for numeric bins or category counts.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/countplot.cpp
:language: cpp
```

#### Output

```{only} html
![countplot output](_static/plots/methods/countplot.svg)
```

```{only} not html
![countplot output](_static/plots/methods/countplot.png)
```

`Source section`: `flash_api.md`

### `disgridplot (overload 1)`

#### Signature

```cpp
Studio& disgridplot(self: Studio, pluss::msharp::RangeView::Series values, const DisGridPlotOptions& options = {});
```

#### Notes

Facet grid for distribution views (`hist`, `density`, `ecdf`, `rug`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/disgridplot.cpp
:language: cpp
```

#### Output

```{only} html
![disgridplot output](_static/plots/methods/disgridplot.svg)
```

```{only} not html
![disgridplot output](_static/plots/methods/disgridplot.png)
```

`Source section`: `flash_api.md`

### `disgridplot (overload 2)`

#### Signature

```cpp
Studio& disgridplot(self: Studio, pluss::msharp::RangeView::Series values, std::string_view kind);
```

#### Notes

Facet grid for distribution views (`hist`, `density`, `ecdf`, `rug`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/disgridplot.cpp
:language: cpp
```

#### Output

```{only} html
![disgridplot output](_static/plots/methods/disgridplot.svg)
```

```{only} not html
![disgridplot output](_static/plots/methods/disgridplot.png)
```

`Source section`: `flash_api.md`

### `displot (single form)`

#### Signature

```cpp
Trace& displot(self: Pane, SeriesLike values, int bins = 12);
```

#### Notes

Distribution shortcut alias for histogram.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/displot.cpp
:language: cpp
```

#### Output

```{only} html
![displot output](_static/plots/methods/displot.svg)
```

```{only} not html
![displot output](_static/plots/methods/displot.png)
```

`Source section`: `flash_api.md`

### `ecdfplot (single form)`

#### Signature

```cpp
Trace& ecdfplot(self: Pane, SeriesLike values);
```

#### Notes

Empirical cumulative distribution function.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/ecdfplot.cpp
:language: cpp
```

#### Output

```{only} html
![ecdfplot output](_static/plots/methods/ecdfplot.svg)
```

```{only} not html
![ecdfplot output](_static/plots/methods/ecdfplot.png)
```

`Source section`: `flash_api.md`

### `heatmap (overload 1)`

#### Signature

```cpp
Trace& heatmap(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

2D intensity map with masking/limits/scale-guide controls.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/heatmap.cpp
:language: cpp
```

#### Output

```{only} html
![heatmap output](_static/plots/methods/heatmap.svg)
```

```{only} not html
![heatmap output](_static/plots/methods/heatmap.png)
```

`Source section`: `flash_api.md`

### `heatmap (overload 2)`

#### Signature

```cpp
Trace& heatmap(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, const HeatmapOptions& options);
```

#### Notes

2D intensity map with masking/limits/scale-guide controls.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/heatmap.cpp
:language: cpp
```

#### Output

```{only} html
![heatmap output](_static/plots/methods/heatmap.svg)
```

```{only} not html
![heatmap output](_static/plots/methods/heatmap.png)
```

`Source section`: `flash_api.md`

### `heatmap (overload 3)`

#### Signature

```cpp
Trace& heatmap(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, SeriesLike c);
```

#### Notes

2D intensity map with masking/limits/scale-guide controls.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/heatmap.cpp
:language: cpp
```

#### Output

```{only} html
![heatmap output](_static/plots/methods/heatmap.svg)
```

```{only} not html
![heatmap output](_static/plots/methods/heatmap.png)
```

`Source section`: `flash_api.md`

### `heatmap (overload 4)`

#### Signature

```cpp
Trace& heatmap(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, SeriesLike c, const HeatmapOptions& options);
```

#### Notes

2D intensity map with masking/limits/scale-guide controls.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/heatmap.cpp
:language: cpp
```

#### Output

```{only} html
![heatmap output](_static/plots/methods/heatmap.svg)
```

```{only} not html
![heatmap output](_static/plots/methods/heatmap.png)
```

`Source section`: `flash_api.md`

### `hist3d (overload 1)`

#### Signature

```cpp
Trace& hist3d(self: Pane, SeriesLike x, SeriesLike y, const Hist3DOptions& options = {});
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, options: Hist3DOptions = {});

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/hist3d.cpp
:language: cpp
```

#### Output

```{only} html
![hist3d output](_static/plots/methods/hist3d.svg)
```

```{only} not html
![hist3d output](_static/plots/methods/hist3d.png)
```

`Source section`: `flash_api.md`

### `hist3d (overload 2)`

#### Signature

```cpp
Trace& hist3d(self: Studio, SeriesLike x, SeriesLike y, const Hist3DOptions& options = {}, std::string title = {});
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, options: Hist3DOptions = {});

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/hist3d.cpp
:language: cpp
```

#### Output

```{only} html
![hist3d output](_static/plots/methods/hist3d.svg)
```

```{only} not html
![hist3d output](_static/plots/methods/hist3d.png)
```

`Source section`: `flash_api.md`

### `histplot (single form)`

#### Signature

```cpp
Trace& histplot(self: Pane, SeriesLike values, int bins = 12);
```

#### Notes

Univariate histogram.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/histplot.cpp
:language: cpp
```

#### Output

```{only} html
![histplot output](_static/plots/methods/histplot.svg)
```

```{only} not html
![histplot output](_static/plots/methods/histplot.png)
```

`Source section`: `flash_api.md`

### `isoshell (overload 1)`

#### Signature

```cpp
Trace& isoshell(self: Pane, SeriesLike values, int level_count = 4);
```

#### Notes

Isosurface-style shell extraction from volumetric grids.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/isoshell.cpp
:language: cpp
```

#### Output

```{only} html
![isoshell output](_static/plots/methods/isoshell.svg)
```

```{only} not html
![isoshell output](_static/plots/methods/isoshell.png)
```

`Source section`: `flash_api.md`

### `isoshell (overload 2)`

#### Signature

```cpp
Trace& isoshell(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, SeriesLike values, int level_count = 4);
```

#### Notes

Isosurface-style shell extraction from volumetric grids.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/isoshell.cpp
:language: cpp
```

#### Output

```{only} html
![isoshell output](_static/plots/methods/isoshell.svg)
```

```{only} not html
![isoshell output](_static/plots/methods/isoshell.png)
```

`Source section`: `flash_api.md`

### `isoshell (overload 3)`

#### Signature

```cpp
Trace& isoshell(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, SeriesLike values, SeriesLike levels);
```

#### Notes

Isosurface-style shell extraction from volumetric grids.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/isoshell.cpp
:language: cpp
```

#### Output

```{only} html
![isoshell output](_static/plots/methods/isoshell.svg)
```

```{only} not html
![isoshell output](_static/plots/methods/isoshell.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 1)`

#### Signature

```cpp
Studio& jointplot(self: Studio, SeriesLike x, SeriesLike y, std::string_view kind = "scatter");
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 2)`

#### Signature

```cpp
Studio& jointplot(self: Studio, SeriesLike x, SeriesLike y, const JointPlotOptions& options);
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 3)`

#### Signature

```cpp
Studio& jointplot(self: Studio, const Dataset& data, std::string_view x_name, std::string_view y_name, std::string_view kind = "scatter");
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 4)`

#### Signature

```cpp
Studio& jointplot(self: Studio, const Dataset& data, std::string_view x_name, std::string_view y_name, const JointPlotOptions& options);
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 5)`

#### Signature

```cpp
Studio& jointplot(self: Studio, const Series& matrix, std::string_view x_name, std::string_view y_name, const MatrixFrameOptions& frame_options, std::string_view kind = "scatter");
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 6)`

#### Signature

```cpp
Studio& jointplot(self: Studio, const Series& matrix, std::string_view x_name, std::string_view y_name, const MatrixFrameOptions& frame_options, const JointPlotOptions& options);
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 7)`

#### Signature

```cpp
Studio& jointplot(self: Studio, const Series& matrix, std::string_view x_name, std::string_view y_name, pluss::msharp::RangeView::Of<std::string_view> feature_names, Axis feature_axis, std::string_view kind = "scatter");
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 8)`

#### Signature

```cpp
Studio& jointplot(self: Studio, const Series& matrix, std::string_view x_name, std::string_view y_name, pluss::msharp::RangeView::Of<std::string_view> feature_names, Axis feature_axis, const JointPlotOptions& options);
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 9)`

#### Signature

```cpp
Studio& jointplot(self: Studio, const Series& matrix, std::string_view x_name, std::string_view y_name, std::string_view kind = "scatter");
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 10)`

#### Signature

```cpp
Studio& jointplot(self: Studio, const Series& matrix, std::string_view x_name, std::string_view y_name, const JointPlotOptions& options);
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 11)`

#### Signature

```cpp
Studio& jointplot(self: Studio, const Series& matrix, const MatrixJointOptions& matrix_options, std::string_view kind = "scatter");
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `jointplot (overload 12)`

#### Signature

```cpp
Studio& jointplot(self: Studio, const Series& matrix, const MatrixJointOptions& matrix_options, const JointPlotOptions& options);
```

#### Notes

Joint + marginals composition for bivariate diagnostics.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointplot.cpp
:language: cpp
```

#### Output

```{only} html
![jointplot output](_static/plots/methods/jointplot.svg)
```

```{only} not html
![jointplot output](_static/plots/methods/jointplot.png)
```

`Source section`: `flash_api.md`

### `kdeplot (overload 1)`

#### Signature

```cpp
Trace& kdeplot(self: Pane, SeriesLike x, SeriesLike density);
```

#### Notes

Kernel density estimate rendered as a smooth curve.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/kdeplot.cpp
:language: cpp
```

#### Output

```{only} html
![kdeplot output](_static/plots/methods/kdeplot.svg)
```

```{only} not html
![kdeplot output](_static/plots/methods/kdeplot.png)
```

`Source section`: `flash_api.md`

### `kdeplot (overload 2)`

#### Signature

```cpp
Trace& kdeplot(self: Pane, SeriesLike values, std::int64_t points = 128);
```

#### Notes

Kernel density estimate rendered as a smooth curve.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/kdeplot.cpp
:language: cpp
```

#### Output

```{only} html
![kdeplot output](_static/plots/methods/kdeplot.svg)
```

```{only} not html
![kdeplot output](_static/plots/methods/kdeplot.png)
```

`Source section`: `flash_api.md`

### `lagplot (single form)`

#### Signature

```cpp
Studio& lagplot(self: Studio, SeriesLike values, const LagPlotOptions& options = {});
```

#### Notes

Scatter of `x_t` vs `x_{t-lag}` for serial dependence.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/lagplot.cpp
:language: cpp
```

#### Output

```{only} html
![lagplot output](_static/plots/methods/lagplot.svg)
```

```{only} not html
![lagplot output](_static/plots/methods/lagplot.png)
```

`Source section`: `flash_api.md`

### `lattice (overload 1)`

#### Signature

```cpp
Trace& lattice(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike[, color]);

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/lattice.cpp
:language: cpp
```

#### Output

```{only} html
![lattice output](_static/plots/methods/lattice.svg)
```

```{only} not html
![lattice output](_static/plots/methods/lattice.png)
```

`Source section`: `flash_api.md`

### `lattice (overload 2)`

#### Signature

```cpp
Trace& lattice(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, SeriesLike c);
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike[, color]);

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/lattice.cpp
:language: cpp
```

#### Output

```{only} html
![lattice output](_static/plots/methods/lattice.svg)
```

```{only} not html
![lattice output](_static/plots/methods/lattice.png)
```

`Source section`: `flash_api.md`

### `lattice (overload 3)`

#### Signature

```cpp
Trace& lattice(self: Studio, SeriesLike x, SeriesLike y, SeriesLike z, std::string title = {});
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike[, color]);

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/lattice.cpp
:language: cpp
```

#### Output

```{only} html
![lattice output](_static/plots/methods/lattice.svg)
```

```{only} not html
![lattice output](_static/plots/methods/lattice.png)
```

`Source section`: `flash_api.md`

### `lineplot (overload 1)`

#### Signature

```cpp
Trace& lineplot(self: Pane, SeriesLike x, SeriesLike y);
```

#### Notes

Line chart with optional grouped hue handling.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/lineplot.cpp
:language: cpp
```

#### Output

```{only} html
![lineplot output](_static/plots/methods/lineplot.svg)
```

```{only} not html
![lineplot output](_static/plots/methods/lineplot.png)
```

`Source section`: `flash_api.md`

### `lineplot (overload 2)`

#### Signature

```cpp
Trace& lineplot(self: Pane, SeriesLike x, SeriesLike y, const Note& hue);
```

#### Notes

Line chart with optional grouped hue handling.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/lineplot.cpp
:language: cpp
```

#### Output

```{only} html
![lineplot output](_static/plots/methods/lineplot.svg)
```

```{only} not html
![lineplot output](_static/plots/methods/lineplot.png)
```

`Source section`: `flash_api.md`

### `lineplot (overload 3)`

#### Signature

```cpp
Trace& lineplot(self: Pane, SeriesLike x, SeriesLike y, SeriesLike hue);
```

#### Notes

Line chart with optional grouped hue handling.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/lineplot.cpp
:language: cpp
```

#### Output

```{only} html
![lineplot output](_static/plots/methods/lineplot.svg)
```

```{only} not html
![lineplot output](_static/plots/methods/lineplot.png)
```

`Source section`: `flash_api.md`

### `lmplot (overload 1)`

#### Signature

```cpp
Trace& lmplot(self: Studio, SeriesLike x, SeriesLike y, std::string title = {});
```

#### Notes

Regression convenience wrapper that allocates its own pane.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/lmplot.cpp
:language: cpp
```

#### Output

```{only} html
![lmplot output](_static/plots/methods/lmplot.svg)
```

```{only} not html
![lmplot output](_static/plots/methods/lmplot.png)
```

`Source section`: `flash_api.md`

### `lmplot (overload 2)`

#### Signature

```cpp
Trace& lmplot(self: Studio, SeriesLike x, SeriesLike y, const RegPlotOptions& options, std::string title = {});
```

#### Notes

Regression convenience wrapper that allocates its own pane.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/lmplot.cpp
:language: cpp
```

#### Output

```{only} html
![lmplot output](_static/plots/methods/lmplot.svg)
```

```{only} not html
![lmplot output](_static/plots/methods/lmplot.png)
```

`Source section`: `flash_api.md`

### `pacfplot (single form)`

#### Signature

```cpp
Studio& pacfplot(self: Studio, SeriesLike values, const PacfPlotOptions& options = {});
```

#### Notes

Partial autocorrelation for AR order identification.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pacfplot.cpp
:language: cpp
```

#### Output

```{only} html
![pacfplot output](_static/plots/methods/pacfplot.svg)
```

```{only} not html
![pacfplot output](_static/plots/methods/pacfplot.png)
```

`Source section`: `flash_api.md`

### `pairplot (overload 1)`

#### Signature

```cpp
Studio& pairplot(self: Studio, pluss::msharp::RangeView::Series columns, const PairPlotOptions& options = {});
```

#### Notes

Pairwise matrix of feature relationships.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pairplot.cpp
:language: cpp
```

#### Output

```{only} html
![pairplot output](_static/plots/methods/pairplot.svg)
```

```{only} not html
![pairplot output](_static/plots/methods/pairplot.png)
```

`Source section`: `flash_api.md`

### `pairplot (overload 2)`

#### Signature

```cpp
Studio& pairplot(self: Studio, const Dataset& data, const PairPlotOptions& options = {});
```

#### Notes

Pairwise matrix of feature relationships.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pairplot.cpp
:language: cpp
```

#### Output

```{only} html
![pairplot output](_static/plots/methods/pairplot.svg)
```

```{only} not html
![pairplot output](_static/plots/methods/pairplot.png)
```

`Source section`: `flash_api.md`

### `pairplot (overload 3)`

#### Signature

```cpp
Studio& pairplot(self: Studio, const Series& matrix, const MatrixFrameOptions& frame_options, const PairPlotOptions& options = {});
```

#### Notes

Pairwise matrix of feature relationships.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pairplot.cpp
:language: cpp
```

#### Output

```{only} html
![pairplot output](_static/plots/methods/pairplot.svg)
```

```{only} not html
![pairplot output](_static/plots/methods/pairplot.png)
```

`Source section`: `flash_api.md`

### `pairplot (overload 4)`

#### Signature

```cpp
Studio& pairplot(self: Studio, const Series& matrix, pluss::msharp::RangeView::Of<std::string_view> feature_names, Axis feature_axis, const PairPlotOptions& options = {});
```

#### Notes

Pairwise matrix of feature relationships.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pairplot.cpp
:language: cpp
```

#### Output

```{only} html
![pairplot output](_static/plots/methods/pairplot.svg)
```

```{only} not html
![pairplot output](_static/plots/methods/pairplot.png)
```

`Source section`: `flash_api.md`

### `pairplot (overload 5)`

#### Signature

```cpp
Studio& pairplot(self: Studio, const Series& matrix, const PairPlotOptions& options = {});
```

#### Notes

Pairwise matrix of feature relationships.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pairplot.cpp
:language: cpp
```

#### Output

```{only} html
![pairplot output](_static/plots/methods/pairplot.svg)
```

```{only} not html
![pairplot output](_static/plots/methods/pairplot.png)
```

`Source section`: `flash_api.md`

### `parallelplot (overload 1)`

#### Signature

```cpp
Studio& parallelplot(self: Studio, pluss::msharp::RangeView::Series columns, const ParallelCoordinatesOptions& options = {});
```

#### Notes

Parallel coordinates for high-dimensional row comparisons.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/parallelplot.cpp
:language: cpp
```

#### Output

```{only} html
![parallelplot output](_static/plots/methods/parallelplot.svg)
```

```{only} not html
![parallelplot output](_static/plots/methods/parallelplot.png)
```

`Source section`: `flash_api.md`

### `parallelplot (overload 2)`

#### Signature

```cpp
Studio& parallelplot(self: Studio, pluss::msharp::RangeView::Series columns, const Note& class_labels, const ParallelCoordinatesOptions& options = {});
```

#### Notes

Parallel coordinates for high-dimensional row comparisons.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/parallelplot.cpp
:language: cpp
```

#### Output

```{only} html
![parallelplot output](_static/plots/methods/parallelplot.svg)
```

```{only} not html
![parallelplot output](_static/plots/methods/parallelplot.png)
```

`Source section`: `flash_api.md`

### `parallelplot (overload 3)`

#### Signature

```cpp
Studio& parallelplot(self: Studio, const Dataset& data, const ParallelCoordinatesOptions& options = {});
```

#### Notes

Parallel coordinates for high-dimensional row comparisons.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/parallelplot.cpp
:language: cpp
```

#### Output

```{only} html
![parallelplot output](_static/plots/methods/parallelplot.svg)
```

```{only} not html
![parallelplot output](_static/plots/methods/parallelplot.png)
```

`Source section`: `flash_api.md`

### `parallelplot (overload 4)`

#### Signature

```cpp
Studio& parallelplot(self: Studio, const Series& matrix, const MatrixFrameOptions& frame_options, const ParallelCoordinatesOptions& options = {});
```

#### Notes

Parallel coordinates for high-dimensional row comparisons.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/parallelplot.cpp
:language: cpp
```

#### Output

```{only} html
![parallelplot output](_static/plots/methods/parallelplot.svg)
```

```{only} not html
![parallelplot output](_static/plots/methods/parallelplot.png)
```

`Source section`: `flash_api.md`

### `parallelplot (overload 5)`

#### Signature

```cpp
Studio& parallelplot(self: Studio, const Series& matrix, pluss::msharp::RangeView::Of<std::string_view> feature_names, Axis feature_axis, const ParallelCoordinatesOptions& options = {});
```

#### Notes

Parallel coordinates for high-dimensional row comparisons.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/parallelplot.cpp
:language: cpp
```

#### Output

```{only} html
![parallelplot output](_static/plots/methods/parallelplot.svg)
```

```{only} not html
![parallelplot output](_static/plots/methods/parallelplot.png)
```

`Source section`: `flash_api.md`

### `parallelplot (overload 6)`

#### Signature

```cpp
Studio& parallelplot(self: Studio, const Series& matrix, const ParallelCoordinatesOptions& options = {});
```

#### Notes

Parallel coordinates for high-dimensional row comparisons.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/parallelplot.cpp
:language: cpp
```

#### Output

```{only} html
![parallelplot output](_static/plots/methods/parallelplot.svg)
```

```{only} not html
![parallelplot output](_static/plots/methods/parallelplot.png)
```

`Source section`: `flash_api.md`

### `path3 (overload 1)`

#### Signature

```cpp
Trace& path3(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike);

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/path3.cpp
:language: cpp
```

#### Output

```{only} html
![path3 output](_static/plots/methods/path3.svg)
```

```{only} not html
![path3 output](_static/plots/methods/path3.png)
```

`Source section`: `flash_api.md`

### `path3 (overload 2)`

#### Signature

```cpp
Trace& path3(self: Studio, SeriesLike x, SeriesLike y, SeriesLike z, std::string title = {});
```

#### Notes

Connected 3D trajectory through ordered samples.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/path3.cpp
:language: cpp
```

#### Output

```{only} html
![path3 output](_static/plots/methods/path3.svg)
```

```{only} not html
![path3 output](_static/plots/methods/path3.png)
```

`Source section`: `flash_scene_animation_api.md`

### `path4 (overload 1)`

#### Signature

```cpp
Trace& path4(self: Pane
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike, w: SeriesLike, options: HyperPlotOptions = {});

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/path4.cpp
:language: cpp
```

#### Output

```{only} html
![path4 output](_static/plots/methods/path4.svg)
```

```{only} not html
![path4 output](_static/plots/methods/path4.png)
```

`Source section`: `flash_api.md`

### `path4 (overload 2)`

#### Signature

```cpp
Trace& path4(self: Pane|Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike, w: SeriesLike, options: HyperPlotOptions = {}, title: std::string = {});
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike, w: SeriesLike, options: HyperPlotOptions = {});

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/path4.cpp
:language: cpp
```

#### Output

```{only} html
![path4 output](_static/plots/methods/path4.svg)
```

```{only} not html
![path4 output](_static/plots/methods/path4.png)
```

`Source section`: `flash_api.md`

### `pieplot (overload 1)`

#### Signature

```cpp
Trace& pieplot(self: Pane, SeriesLike values);
```

#### Notes

Pie wedges from part-to-whole values.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pieplot.cpp
:language: cpp
```

#### Output

```{only} html
![pieplot output](_static/plots/methods/pieplot.svg)
```

```{only} not html
![pieplot output](_static/plots/methods/pieplot.png)
```

`Source section`: `flash_api.md`

### `pieplot (overload 2)`

#### Signature

```cpp
Trace& pieplot(self: Pane, SeriesLike values, pluss::msharp::RangeView::Of<std::string_view> labels);
Trace& pieplot(self: Pane, SeriesLike values, std::initializer_list<std::string_view> labels);
```

#### Notes

Pie wedges from part-to-whole values.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pieplot.cpp
:language: cpp
```

#### Output

```{only} html
![pieplot output](_static/plots/methods/pieplot.svg)
```

```{only} not html
![pieplot output](_static/plots/methods/pieplot.png)
```

`Source section`: `flash_api.md`

### `plot3d (single form)`

#### Signature

```cpp
Trace& plot3D(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

Matplotlib-style alias for 3D path plotting.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/plot3D.cpp
:language: cpp
```

#### Output

```{only} html
![plot3D output](_static/plots/methods/plot3D.svg)
```

```{only} not html
![plot3D output](_static/plots/methods/plot3D.png)
```

`Source section`: `flash_api.md`

### `plot_surface (single form)`

#### Signature

```cpp
Trace& plot_surface(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

Matplotlib-style alias for surface plotting (`sheet`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/plot_surface.cpp
:language: cpp
```

#### Output

```{only} html
![plot_surface output](_static/plots/methods/plot_surface.svg)
```

```{only} not html
![plot_surface output](_static/plots/methods/plot_surface.png)
```

`Source section`: `flash_api.md`

### `plot_wireframe (single form)`

#### Signature

```cpp
Trace& plot_wireframe(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

Matplotlib-style alias for wireframe plotting (`lattice`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/plot_wireframe.cpp
:language: cpp
```

#### Output

```{only} html
![plot_wireframe output](_static/plots/methods/plot_wireframe.svg)
```

```{only} not html
![plot_wireframe output](_static/plots/methods/plot_wireframe.png)
```

`Source section`: `flash_api.md`

### `pointplot (single form)`

#### Signature

```cpp
Trace& pointplot(self: Pane, SeriesLike x, SeriesLike y, SeriesLike err);
```

#### Notes

Mean/point trend with error whiskers.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pointplot.cpp
:language: cpp
```

#### Output

```{only} html
![pointplot output](_static/plots/methods/pointplot.svg)
```

```{only} not html
![pointplot output](_static/plots/methods/pointplot.png)
```

`Source section`: `flash_api.md`

### `qqplot (overload 1)`

#### Signature

```cpp
Trace& qqplot(self: Pane, SeriesLike values, const QqPlotOptions& options = {});
```

#### Notes

Studio, values: SeriesLike, options: QqPlotOptions = {});

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/qqplot.cpp
:language: cpp
```

#### Output

```{only} html
![qqplot output](_static/plots/methods/qqplot.svg)
```

```{only} not html
![qqplot output](_static/plots/methods/qqplot.png)
```

`Source section`: `flash_api.md`

### `qqplot (overload 2)`

#### Signature

```cpp
Trace& qqplot(self: Studio, SeriesLike values, const QqPlotOptions& options = {}, std::string title = {});
```

#### Notes

Studio, values: SeriesLike, options: QqPlotOptions = {});

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/qqplot.cpp
:language: cpp
```

#### Output

```{only} html
![qqplot output](_static/plots/methods/qqplot.svg)
```

```{only} not html
![qqplot output](_static/plots/methods/qqplot.png)
```

`Source section`: `flash_api.md`

### `radviz (overload 1)`

#### Signature

```cpp
Studio& radviz(self: Studio, pluss::msharp::RangeView::Series columns, const RadvizOptions& options = {});
```

#### Notes

Projects features to anchors on a circle for class separation.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/radviz.cpp
:language: cpp
```

#### Output

```{only} html
![radviz output](_static/plots/methods/radviz.svg)
```

```{only} not html
![radviz output](_static/plots/methods/radviz.png)
```

`Source section`: `flash_api.md`

### `radviz (overload 2)`

#### Signature

```cpp
Studio& radviz(self: Studio, pluss::msharp::RangeView::Series columns, const Note& class_labels, const RadvizOptions& options = {});
```

#### Notes

Projects features to anchors on a circle for class separation.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/radviz.cpp
:language: cpp
```

#### Output

```{only} html
![radviz output](_static/plots/methods/radviz.svg)
```

```{only} not html
![radviz output](_static/plots/methods/radviz.png)
```

`Source section`: `flash_api.md`

### `radviz (overload 3)`

#### Signature

```cpp
Studio& radviz(self: Studio, const Dataset& data, const RadvizOptions& options = {});
```

#### Notes

Projects features to anchors on a circle for class separation.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/radviz.cpp
:language: cpp
```

#### Output

```{only} html
![radviz output](_static/plots/methods/radviz.svg)
```

```{only} not html
![radviz output](_static/plots/methods/radviz.png)
```

`Source section`: `flash_api.md`

### `radviz (overload 4)`

#### Signature

```cpp
Studio& radviz(self: Studio, const Series& matrix, const MatrixFrameOptions& frame_options, const RadvizOptions& options = {});
```

#### Notes

Projects features to anchors on a circle for class separation.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/radviz.cpp
:language: cpp
```

#### Output

```{only} html
![radviz output](_static/plots/methods/radviz.svg)
```

```{only} not html
![radviz output](_static/plots/methods/radviz.png)
```

`Source section`: `flash_api.md`

### `radviz (overload 5)`

#### Signature

```cpp
Studio& radviz(self: Studio, const Series& matrix, pluss::msharp::RangeView::Of<std::string_view> feature_names, Axis feature_axis, const RadvizOptions& options = {});
```

#### Notes

Projects features to anchors on a circle for class separation.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/radviz.cpp
:language: cpp
```

#### Output

```{only} html
![radviz output](_static/plots/methods/radviz.svg)
```

```{only} not html
![radviz output](_static/plots/methods/radviz.png)
```

`Source section`: `flash_api.md`

### `radviz (overload 6)`

#### Signature

```cpp
Studio& radviz(self: Studio, const Series& matrix, const RadvizOptions& options = {});
```

#### Notes

Projects features to anchors on a circle for class separation.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/radviz.cpp
:language: cpp
```

#### Output

```{only} html
![radviz output](_static/plots/methods/radviz.svg)
```

```{only} not html
![radviz output](_static/plots/methods/radviz.png)
```

`Source section`: `flash_api.md`

### `regplot (single form)`

#### Signature

```cpp
Trace& regplot(self: Pane, SeriesLike x, SeriesLike y, const RegPlotOptions& options = {});
```

#### Notes

Scatter + fitted regression (optionally CI bands).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/regplot.cpp
:language: cpp
```

#### Output

```{only} html
![regplot output](_static/plots/methods/regplot.svg)
```

```{only} not html
![regplot output](_static/plots/methods/regplot.png)
```

`Source section`: `flash_api.md`

### `relgridplot (overload 1)`

#### Signature

```cpp
Studio& relgridplot(self: Studio, pluss::msharp::RangeView::Series xs, pluss::msharp::RangeView::Series ys, const RelGridPlotOptions& options = {});
```

#### Notes

Facet grid for relational plots (scatter/line/reg).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/relgridplot.cpp
:language: cpp
```

#### Output

```{only} html
![relgridplot output](_static/plots/methods/relgridplot.svg)
```

```{only} not html
![relgridplot output](_static/plots/methods/relgridplot.png)
```

`Source section`: `flash_api.md`

### `relgridplot (overload 2)`

#### Signature

```cpp
Studio& relgridplot(self: Studio, pluss::msharp::RangeView::Series xs, pluss::msharp::RangeView::Series ys, std::string_view kind);
```

#### Notes

Facet grid for relational plots (scatter/line/reg).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/relgridplot.cpp
:language: cpp
```

#### Output

```{only} html
![relgridplot output](_static/plots/methods/relgridplot.svg)
```

```{only} not html
![relgridplot output](_static/plots/methods/relgridplot.png)
```

`Source section`: `flash_api.md`

### `relplot (overload 1)`

#### Signature

```cpp
Trace& relplot(self: Pane, SeriesLike x, SeriesLike y, std::string_view kind = "scatter");
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike[, hue], kind = "scatter");

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/relplot.cpp
:language: cpp
```

#### Output

```{only} html
![relplot output](_static/plots/methods/relplot.svg)
```

```{only} not html
![relplot output](_static/plots/methods/relplot.png)
```

`Source section`: `flash_api.md`

### `relplot (overload 2)`

#### Signature

```cpp
Trace& relplot(self: Pane, SeriesLike x, SeriesLike y, const Note& hue, std::string_view kind = "scatter");
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike[, hue], kind = "scatter");

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/relplot.cpp
:language: cpp
```

#### Output

```{only} html
![relplot output](_static/plots/methods/relplot.svg)
```

```{only} not html
![relplot output](_static/plots/methods/relplot.png)
```

`Source section`: `flash_api.md`

### `relplot (overload 3)`

#### Signature

```cpp
Trace& relplot(self: Pane, SeriesLike x, SeriesLike y, SeriesLike hue, std::string_view kind = "scatter");
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike[, hue], kind = "scatter");

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/relplot.cpp
:language: cpp
```

#### Output

```{only} html
![relplot output](_static/plots/methods/relplot.svg)
```

```{only} not html
![relplot output](_static/plots/methods/relplot.png)
```

`Source section`: `flash_api.md`

### `relplot (overload 4)`

#### Signature

```cpp
Trace& relplot(self: Studio, SeriesLike x, SeriesLike y, std::string_view kind = "scatter", std::string title = {});
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike[, hue], kind = "scatter");

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/relplot.cpp
:language: cpp
```

#### Output

```{only} html
![relplot output](_static/plots/methods/relplot.svg)
```

```{only} not html
![relplot output](_static/plots/methods/relplot.png)
```

`Source section`: `flash_api.md`

### `relplot (overload 5)`

#### Signature

```cpp
Trace& relplot(self: Studio, SeriesLike x, SeriesLike y, const Note& hue, std::string_view kind = "scatter", std::string title = {});
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike[, hue], kind = "scatter");

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/relplot.cpp
:language: cpp
```

#### Output

```{only} html
![relplot output](_static/plots/methods/relplot.svg)
```

```{only} not html
![relplot output](_static/plots/methods/relplot.png)
```

`Source section`: `flash_api.md`

### `relplot (overload 6)`

#### Signature

```cpp
Trace& relplot(self: Studio, SeriesLike x, SeriesLike y, SeriesLike hue, std::string_view kind = "scatter", std::string title = {});
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike[, hue], kind = "scatter");

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/relplot.cpp
:language: cpp
```

#### Output

```{only} html
![relplot output](_static/plots/methods/relplot.svg)
```

```{only} not html
![relplot output](_static/plots/methods/relplot.png)
```

`Source section`: `flash_api.md`

### `resid_acfplot (single form)`

#### Signature

```cpp
Studio& resid_acfplot(self: Studio, SeriesLike x, SeriesLike y, const ResidAcfPlotOptions& options = {});
```

#### Notes

Fits regression, then runs ACF on residuals.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/resid_acfplot.cpp
:language: cpp
```

#### Output

```{only} html
![resid_acfplot output](_static/plots/methods/resid_acfplot.svg)
```

```{only} not html
![resid_acfplot output](_static/plots/methods/resid_acfplot.png)
```

`Source section`: `flash_api.md`

### `resid_histplot (overload 1)`

#### Signature

```cpp
Trace& resid_histplot(self: Pane, SeriesLike x, SeriesLike y, const ResidHistPlotOptions& options = {});
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, options: ResidHistPlotOptions = {});

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/resid_histplot.cpp
:language: cpp
```

#### Output

```{only} html
![resid_histplot output](_static/plots/methods/resid_histplot.svg)
```

```{only} not html
![resid_histplot output](_static/plots/methods/resid_histplot.png)
```

`Source section`: `flash_api.md`

### `resid_histplot (overload 2)`

#### Signature

```cpp
Trace& resid_histplot(self: Studio, SeriesLike x, SeriesLike y, const ResidHistPlotOptions& options = {}, std::string title = {});
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, options: ResidHistPlotOptions = {});

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/resid_histplot.cpp
:language: cpp
```

#### Output

```{only} html
![resid_histplot output](_static/plots/methods/resid_histplot.svg)
```

```{only} not html
![resid_histplot output](_static/plots/methods/resid_histplot.png)
```

`Source section`: `flash_api.md`

### `residplot (overload 1)`

#### Signature

```cpp
Trace& residplot(self: Pane, SeriesLike x, SeriesLike y, const RegPlotOptions& options);
```

#### Notes

Residual scatter against predictor axis.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/residplot.cpp
:language: cpp
```

#### Output

```{only} html
![residplot output](_static/plots/methods/residplot.svg)
```

```{only} not html
![residplot output](_static/plots/methods/residplot.png)
```

`Source section`: `flash_api.md`

### `residplot (overload 2)`

#### Signature

```cpp
Trace& residplot(self: Pane, SeriesLike x, SeriesLike y);
```

#### Notes

Residual scatter against predictor axis.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/residplot.cpp
:language: cpp
```

#### Output

```{only} html
![residplot output](_static/plots/methods/residplot.svg)
```

```{only} not html
![residplot output](_static/plots/methods/residplot.png)
```

`Source section`: `flash_api.md`

### `ridge3 (overload 1)`

#### Signature

```cpp
Trace& ridge3(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

3D ridge layers over a surface grid.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/ridge3.cpp
:language: cpp
```

#### Output

```{only} html
![ridge3 output](_static/plots/methods/ridge3.svg)
```

```{only} not html
![ridge3 output](_static/plots/methods/ridge3.png)
```

`Source section`: `flash_api.md`

### `ridge3 (overload 2)`

#### Signature

```cpp
Trace& ridge3(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, SeriesLike levels);
```

#### Notes

3D ridge layers over a surface grid.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/ridge3.cpp
:language: cpp
```

#### Output

```{only} html
![ridge3 output](_static/plots/methods/ridge3.svg)
```

```{only} not html
![ridge3 output](_static/plots/methods/ridge3.png)
```

`Source section`: `flash_api.md`

### `ridge3 (overload 3)`

#### Signature

```cpp
Trace& ridge3(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, int level_count);
```

#### Notes

3D ridge layers over a surface grid.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/ridge3.cpp
:language: cpp
```

#### Output

```{only} html
![ridge3 output](_static/plots/methods/ridge3.svg)
```

```{only} not html
![ridge3 output](_static/plots/methods/ridge3.png)
```

`Source section`: `flash_api.md`

### `scatter3d (single form)`

#### Signature

```cpp
Trace& scatter3D(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

Matplotlib-style alias for `cloud3`.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/scatter3D.cpp
:language: cpp
```

#### Output

```{only} html
![scatter3D output](_static/plots/methods/scatter3D.svg)
```

```{only} not html
![scatter3D output](_static/plots/methods/scatter3D.png)
```

`Source section`: `flash_api.md`

### `scatterplot (overload 1)`

#### Signature

```cpp
Trace& scatterplot(self: Pane, SeriesLike x, SeriesLike y);
```

#### Notes

2D scatter with optional hue grouping/binning.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/scatterplot.cpp
:language: cpp
```

#### Output

```{only} html
![scatterplot output](_static/plots/methods/scatterplot.svg)
```

```{only} not html
![scatterplot output](_static/plots/methods/scatterplot.png)
```

`Source section`: `flash_api.md`

### `scatterplot (overload 2)`

#### Signature

```cpp
Trace& scatterplot(self: Pane, SeriesLike x, SeriesLike y, const Note& hue);
```

#### Notes

2D scatter with optional hue grouping/binning.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/scatterplot.cpp
:language: cpp
```

#### Output

```{only} html
![scatterplot output](_static/plots/methods/scatterplot.svg)
```

```{only} not html
![scatterplot output](_static/plots/methods/scatterplot.png)
```

`Source section`: `flash_api.md`

### `scatterplot (overload 3)`

#### Signature

```cpp
Trace& scatterplot(self: Pane, SeriesLike x, SeriesLike y, SeriesLike hue);
```

#### Notes

2D scatter with optional hue grouping/binning.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/scatterplot.cpp
:language: cpp
```

#### Output

```{only} html
![scatterplot output](_static/plots/methods/scatterplot.svg)
```

```{only} not html
![scatterplot output](_static/plots/methods/scatterplot.png)
```

`Source section`: `flash_api.md`

### `scene3d (single form)`

#### Signature

```cpp
Pane& scene3d(self: Pane, options: Scene3DOptions);
```

#### Notes

Applies camera, lighting, theme, and preset orchestration for 3D panes.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/scene3d.cpp
:language: cpp
```

#### Output

```{only} html
![scene3d output](_static/plots/methods/scene3d.svg)
```

```{only} not html
![scene3d output](_static/plots/methods/scene3d.png)
```

`Source section`: `flash_api.md`

### `sheet (overload 1)`

#### Signature

```cpp
Trace& sheet(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike[, color]);

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/sheet.cpp
:language: cpp
```

#### Output

```{only} html
![sheet output](_static/plots/methods/sheet.svg)
```

```{only} not html
![sheet output](_static/plots/methods/sheet.png)
```

`Source section`: `flash_api.md`

### `sheet (overload 2)`

#### Signature

```cpp
Trace& sheet(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, SeriesLike c);
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike[, color]);

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/sheet.cpp
:language: cpp
```

#### Output

```{only} html
![sheet output](_static/plots/methods/sheet.svg)
```

```{only} not html
![sheet output](_static/plots/methods/sheet.png)
```

`Source section`: `flash_api.md`

### `sheet (overload 3)`

#### Signature

```cpp
Trace& sheet(self: Studio, SeriesLike x, SeriesLike y, SeriesLike z, std::string title = {});
```

#### Notes

Studio, x: SeriesLike, y: SeriesLike, z: SeriesLike[, color]);

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/sheet.cpp
:language: cpp
```

#### Output

```{only} html
![sheet output](_static/plots/methods/sheet.svg)
```

```{only} not html
![sheet output](_static/plots/methods/sheet.png)
```

`Source section`: `flash_api.md`

### `spire3 (overload 1)`

#### Signature

```cpp
Trace& spire3(self: Pane, SeriesLike z);
```

#### Notes

3D spire/column rendering from grid-like data.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/spire3.cpp
:language: cpp
```

#### Output

```{only} html
![spire3 output](_static/plots/methods/spire3.svg)
```

```{only} not html
![spire3 output](_static/plots/methods/spire3.png)
```

`Source section`: `flash_api.md`

### `spire3 (overload 2)`

#### Signature

```cpp
Trace& spire3(self: Pane, SeriesLike y, SeriesLike z);
```

#### Notes

3D spire/column rendering from grid-like data.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/spire3.cpp
:language: cpp
```

#### Output

```{only} html
![spire3 output](_static/plots/methods/spire3.svg)
```

```{only} not html
![spire3 output](_static/plots/methods/spire3.png)
```

`Source section`: `flash_api.md`

### `spire3 (overload 3)`

#### Signature

```cpp
Trace& spire3(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z);
```

#### Notes

3D spire/column rendering from grid-like data.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/spire3.cpp
:language: cpp
```

#### Output

```{only} html
![spire3 output](_static/plots/methods/spire3.svg)
```

```{only} not html
![spire3 output](_static/plots/methods/spire3.png)
```

`Source section`: `flash_api.md`

### `spire3 (overload 4)`

#### Signature

```cpp
Trace& spire3(self: Pane, SeriesLike x, SeriesLike y, SeriesLike z, SeriesLike c);
```

#### Notes

3D spire/column rendering from grid-like data.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/spire3.cpp
:language: cpp
```

#### Output

```{only} html
![spire3 output](_static/plots/methods/spire3.svg)
```

```{only} not html
![spire3 output](_static/plots/methods/spire3.png)
```

`Source section`: `flash_api.md`

### `stepplot (single form)`

#### Signature

```cpp
Trace& stepplot(self: Pane, SeriesLike x, SeriesLike y);
```

#### Notes

Converts line input into right-step segments.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/stepplot.cpp
:language: cpp
```

#### Output

```{only} html
![stepplot output](_static/plots/methods/stepplot.svg)
```

```{only} not html
![stepplot output](_static/plots/methods/stepplot.png)
```

`Source section`: `flash_api.md`

### `stripplot (overload 1)`

#### Signature

```cpp
Trace& stripplot(self: Pane, const Note& categories, const Series& values);
```

#### Notes

Raw points per category with minimal aggregation.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/stripplot.cpp
:language: cpp
```

#### Output

```{only} html
![stripplot output](_static/plots/methods/stripplot.svg)
```

```{only} not html
![stripplot output](_static/plots/methods/stripplot.png)
```

`Source section`: `flash_api.md`

### `stripplot (overload 2)`

#### Signature

```cpp
Trace& stripplot(self: Pane, SeriesLike group, SeriesLike values);
```

#### Notes

Raw points per category with minimal aggregation.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/stripplot.cpp
:language: cpp
```

#### Output

```{only} html
![stripplot output](_static/plots/methods/stripplot.svg)
```

```{only} not html
![stripplot output](_static/plots/methods/stripplot.png)
```

`Source section`: `flash_api.md`

### `summaryplot (single form)`

#### Signature

```cpp
Studio& summaryplot(self: Studio, pluss::msharp::RangeView::Series columns, pluss::msharp::RangeView::Of<std::string_view> labels, const SummaryOptions& options = {});
```

#### Notes

Statistical summary tables rendered into a figure layout.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/summaryplot.cpp
:language: cpp
```

#### Output

```{only} html
![summaryplot output](_static/plots/methods/summaryplot.svg)
```

```{only} not html
![summaryplot output](_static/plots/methods/summaryplot.png)
```

`Source section`: `flash_api.md`

### `swarmplot (single form)`

#### Signature

```cpp
Trace& swarmplot(self: Pane, SeriesLike group, SeriesLike values);
```

#### Notes

Alias of stripplot for dense categorical point clouds.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/swarmplot.cpp
:language: cpp
```

#### Output

```{only} html
![swarmplot output](_static/plots/methods/swarmplot.svg)
```

```{only} not html
![swarmplot output](_static/plots/methods/swarmplot.png)
```

`Source section`: `flash_api.md`

### `triplot (overload 1)`

#### Signature

```cpp
Studio& triplot(self: Studio, pluss::msharp::RangeView::Series columns, const TriplotOptions& options = {});
```

#### Notes

Enumerates 3-feature combinations into small-multiple 3D panels.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/triplot.cpp
:language: cpp
```

#### Output

```{only} html
![triplot output](_static/plots/methods/triplot.svg)
```

```{only} not html
![triplot output](_static/plots/methods/triplot.png)
```

`Source section`: `flash_api.md`

### `triplot (overload 2)`

#### Signature

```cpp
Studio& triplot(self: Studio, const Dataset& data, const TriplotOptions& options = {});
```

#### Notes

Enumerates 3-feature combinations into small-multiple 3D panels.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/triplot.cpp
:language: cpp
```

#### Output

```{only} html
![triplot output](_static/plots/methods/triplot.svg)
```

```{only} not html
![triplot output](_static/plots/methods/triplot.png)
```

`Source section`: `flash_api.md`

### `triplot (overload 3)`

#### Signature

```cpp
Studio& triplot(self: Studio, const Series& matrix, const MatrixFrameOptions& frame_options, const TriplotOptions& options = {});
```

#### Notes

Enumerates 3-feature combinations into small-multiple 3D panels.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/triplot.cpp
:language: cpp
```

#### Output

```{only} html
![triplot output](_static/plots/methods/triplot.svg)
```

```{only} not html
![triplot output](_static/plots/methods/triplot.png)
```

`Source section`: `flash_api.md`

### `triplot (overload 4)`

#### Signature

```cpp
Studio& triplot(self: Studio, const Series& matrix, pluss::msharp::RangeView::Of<std::string_view> feature_names, Axis feature_axis, const TriplotOptions& options = {});
```

#### Notes

Enumerates 3-feature combinations into small-multiple 3D panels.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/triplot.cpp
:language: cpp
```

#### Output

```{only} html
![triplot output](_static/plots/methods/triplot.svg)
```

```{only} not html
![triplot output](_static/plots/methods/triplot.png)
```

`Source section`: `flash_api.md`

### `triplot (overload 5)`

#### Signature

```cpp
Studio& triplot(self: Studio, const Series& matrix, const TriplotOptions& options = {});
```

#### Notes

Enumerates 3-feature combinations into small-multiple 3D panels.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/triplot.cpp
:language: cpp
```

#### Output

```{only} html
![triplot output](_static/plots/methods/triplot.svg)
```

```{only} not html
![triplot output](_static/plots/methods/triplot.png)
```

`Source section`: `flash_api.md`

### `violinplot (overload 1)`

#### Signature

```cpp
Trace& violinplot(self: Pane, SeriesLike values);
```

#### Notes

Density-shape distribution view, optionally grouped.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/violinplot.cpp
:language: cpp
```

#### Output

```{only} html
![violinplot output](_static/plots/methods/violinplot.svg)
```

```{only} not html
![violinplot output](_static/plots/methods/violinplot.png)
```

`Source section`: `flash_api.md`

### `violinplot (overload 2)`

#### Signature

```cpp
Trace& violinplot(self: Pane, SeriesLike values, const ViolinOptions& options);
```

#### Notes

Density-shape distribution view, optionally grouped.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/violinplot.cpp
:language: cpp
```

#### Output

```{only} html
![violinplot output](_static/plots/methods/violinplot.svg)
```

```{only} not html
![violinplot output](_static/plots/methods/violinplot.png)
```

`Source section`: `flash_api.md`

### `violinplot (overload 3)`

#### Signature

```cpp
Trace& violinplot(self: Pane, const Note& categories, const Series& values);
```

#### Notes

Density-shape distribution view, optionally grouped.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/violinplot.cpp
:language: cpp
```

#### Output

```{only} html
![violinplot output](_static/plots/methods/violinplot.svg)
```

```{only} not html
![violinplot output](_static/plots/methods/violinplot.png)
```

`Source section`: `flash_api.md`

### `violinplot (overload 4)`

#### Signature

```cpp
Trace& violinplot(self: Pane, const Note& categories, const Series& values, const ViolinOptions& options);
```

#### Notes

Density-shape distribution view, optionally grouped.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/violinplot.cpp
:language: cpp
```

#### Output

```{only} html
![violinplot output](_static/plots/methods/violinplot.svg)
```

```{only} not html
![violinplot output](_static/plots/methods/violinplot.png)
```

`Source section`: `flash_api.md`

### `violinplot (overload 5)`

#### Signature

```cpp
Trace& violinplot(self: Pane, SeriesLike group, SeriesLike values);
```

#### Notes

Density-shape distribution view, optionally grouped.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/violinplot.cpp
:language: cpp
```

#### Output

```{only} html
![violinplot output](_static/plots/methods/violinplot.svg)
```

```{only} not html
![violinplot output](_static/plots/methods/violinplot.png)
```

`Source section`: `flash_api.md`

### `violinplot (overload 6)`

#### Signature

```cpp
Trace& violinplot(self: Pane, SeriesLike group, SeriesLike values, const ViolinOptions& options);
```

#### Notes

Density-shape distribution view, optionally grouped.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/violinplot.cpp
:language: cpp
```

#### Output

```{only} html
![violinplot output](_static/plots/methods/violinplot.svg)
```

```{only} not html
![violinplot output](_static/plots/methods/violinplot.png)
```

`Source section`: `flash_api.md`

### `whiskplot (overload 1)`

#### Signature

```cpp
Trace& whiskplot(self: Pane, SeriesLike x, SeriesLike y, SeriesLike err);
```

#### Notes

Error-whisker plotting with symmetric or explicit bounds.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/whiskplot.cpp
:language: cpp
```

#### Output

```{only} html
![whiskplot output](_static/plots/methods/whiskplot.svg)
```

```{only} not html
![whiskplot output](_static/plots/methods/whiskplot.png)
```

`Source section`: `flash_api.md`

### `whiskplot (overload 2)`

#### Signature

```cpp
Trace& whiskplot(self: Pane, SeriesLike x, SeriesLike y, SeriesLike yneg, SeriesLike ypos, SeriesLike xneg, SeriesLike xpos);
```

#### Notes

Error-whisker plotting with symmetric or explicit bounds.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/whiskplot.cpp
:language: cpp
```

#### Output

```{only} html
![whiskplot output](_static/plots/methods/whiskplot.svg)
```

```{only} not html
![whiskplot output](_static/plots/methods/whiskplot.png)
```

`Source section`: `flash_api.md`

### `rugplot (single form)`

#### Signature

```cpp
Trace& rugplot(self: Pane, SeriesLike values, double height = 0.08);
```

#### Notes

Draws small tick marks along an axis to visualize sample concentration.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/rugplot.cpp
:language: cpp
```

#### Output

```{only} html
![rugplot output](_static/plots/methods/rugplot.svg)
```

```{only} not html
![rugplot output](_static/plots/methods/rugplot.png)
```

`Source section`: `flash_api.md`

### `facetgrid (single form)`

#### Signature

```cpp
Studio& FacetGrid(self: Studio, std::int64_t rows, std::int64_t cols, double gx = 18.0, double gy = 18.0);
```

#### Notes

Allocates a faceted 2D grid layout. You then populate panes manually.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/facetgrid.cpp
:language: cpp
```

#### Output

```{only} html
![facetgrid output](_static/plots/methods/facetgrid.svg)
```

```{only} not html
![facetgrid output](_static/plots/methods/facetgrid.png)
```

`Source section`: `flash_api.md`

### `pairgrid (single form)`

#### Signature

```cpp
Studio& PairGrid(self: Studio, std::int64_t dimensions, double gx = 14.0, double gy = 14.0);
```

#### Notes

Creates an NxN grid intended for pairwise matrix views.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pairgrid.cpp
:language: cpp
```

#### Output

```{only} html
![pairgrid output](_static/plots/methods/pairgrid.svg)
```

```{only} not html
![pairgrid output](_static/plots/methods/pairgrid.png)
```

`Source section`: `flash_api.md`

### `jointgrid (single form)`

#### Signature

```cpp
Studio& JointGrid(self: Studio, double gx = 14.0, double gy = 14.0);
```

#### Notes

Creates the standard joint-layout scaffold (joint pane + top/right marginals).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointgrid.cpp
:language: cpp
```

#### Output

```{only} html
![jointgrid output](_static/plots/methods/jointgrid.svg)
```

```{only} not html
![jointgrid output](_static/plots/methods/jointgrid.png)
```

`Source section`: `flash_api.md`

### `facet_map_plot (single form)`

#### Signature

```cpp
requires std::invocable<Mapper, Pane&, std::int64_t, std::int64_t> inline Studio& facet_map_plot(self: Studio, std::int64_t rows, std::int64_t cols, SeriesLike mapper, double gx = 18.0, double gy = 18.0);
```

#### Notes

Runs a user mapper callback over each facet cell for custom per-pane drawing.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/facet_map_plot.cpp
:language: cpp
```

#### Output

```{only} html
![facet_map_plot output](_static/plots/methods/facet_map_plot.svg)
```

```{only} not html
![facet_map_plot output](_static/plots/methods/facet_map_plot.png)
```

`Source section`: `flash_api.md`

### `pairgrid_map_plot (single form)`

#### Signature

```cpp
requires std::invocable<Mapper, Pane&, std::int64_t, std::int64_t, bool> inline Studio& pairgrid_map_plot(self: Studio, std::int64_t dimensions, SeriesLike mapper, double gx = 14.0, double gy = 14.0);
```

#### Notes

Runs a callback over all pair-grid cells and flags diagonal cells for specialized rendering.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pairgrid_map_plot.cpp
:language: cpp
```

#### Output

```{only} html
![pairgrid_map_plot output](_static/plots/methods/pairgrid_map_plot.svg)
```

```{only} not html
![pairgrid_map_plot output](_static/plots/methods/pairgrid_map_plot.png)
```

`Source section`: `flash_api.md`

### `jointgrid_map_plot (single form)`

#### Signature

```cpp
requires std::invocable<JointMapper, Pane&> && std::invocable<MarginalMapper, Pane&, bool> inline Studio& jointgrid_map_plot(self: Studio, SeriesLike joint_mapper, SeriesLike marginal_mapper, double gx = 14.0, double gy = 14.0);
```

#### Notes

Runs separate callbacks for the central joint pane and marginal panes in a joint-grid layout.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointgrid_map_plot.cpp
:language: cpp
```

#### Output

```{only} html
![jointgrid_map_plot output](_static/plots/methods/jointgrid_map_plot.svg)
```

```{only} not html
![jointgrid_map_plot output](_static/plots/methods/jointgrid_map_plot.png)
```

`Source section`: `flash_api.md`

### `color_palette (single form)`

#### Signature

```cpp
inline PaletteRamp color_palette(std::string_view name = "ocean");
```

#### Notes

Returns a named palette ramp (`ocean`, `thermal`, `lagoon`, `meadow`, `neutral`, plus seaborn-like names).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/color_palette.cpp
:language: cpp
```

#### Output

```{only} html
![color_palette output](_static/plots/methods/color_palette.svg)
```

```{only} not html
![color_palette output](_static/plots/methods/color_palette.png)
```

`Source section`: `flash_theme_api.md`

### `despine (single form)`

#### Signature

```cpp
Pane& despine(self: Pane, enable: bool = true) noexcept;
```

#### Notes

Keeps the plotting content while removing pane-frame emphasis for cleaner minimalist charts.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/despine.cpp
:language: cpp
```

#### Output

```{only} html
![despine output](_static/plots/methods/despine.svg)
```

```{only} not html
![despine output](_static/plots/methods/despine.png)
```

`Source section`: `flash_theme_api.md`

### `horizontal_grid (single form)`

#### Signature

```cpp
Pane& horizontal_grid(self: Pane, enable: bool = true) noexcept;
```

#### Notes

Applies y-major grid only (with x-grid suppressed) for time-series readability.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/horizontal_grid.cpp
:language: cpp
```

#### Output

```{only} html
![horizontal_grid output](_static/plots/methods/horizontal_grid.svg)
```

```{only} not html
![horizontal_grid output](_static/plots/methods/horizontal_grid.png)
```

`Source section`: `flash_theme_api.md`

### `load_dataset (single form)`

#### Signature

```cpp
inline std::optional<Dataset> load_dataset(std::string_view name);
```

#### Notes

Loads built-in dataset bundles (currently `tips`, `flights`) for quick exploratory plotting flows.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/load_dataset.cpp
:language: cpp
```

#### Output

```{only} html
![load_dataset output](_static/plots/methods/load_dataset.svg)
```

```{only} not html
![load_dataset output](_static/plots/methods/load_dataset.png)
```

`Source section`: `flash_theme_api.md`

### `plotting_context (single form)`

#### Signature

```cpp
Studio& plotting_context(self: Studio, width: double, height: double, tick_font_scale: double = 1.0, axis_label_font_scale: double = 1.0);
```

#### Notes

Alias of `set_context(...)`; applies figure extent and tick/label scaling.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/plotting_context.cpp
:language: cpp
```

#### Output

```{only} html
![plotting_context output](_static/plots/methods/plotting_context.svg)
```

```{only} not html
![plotting_context output](_static/plots/methods/plotting_context.png)
```

`Source section`: `flash_theme_api.md`

### `set_context (overload 1)`

#### Signature

```cpp
Studio& set_context(self: Studio, width: double, height: double, tick_font_scale: double = 1.0, axis_label_font_scale: double = 1.0);
```

#### Notes

Primary context entry point. Use explicit dimensions or semantic context presets (`Paper`, `Notebook`, `Talk`, `Poster`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/set_context.cpp
:language: cpp
```

#### Output

```{only} html
![set_context output](_static/plots/methods/set_context.svg)
```

```{only} not html
![set_context output](_static/plots/methods/set_context.png)
```

`Source section`: `flash_theme_api.md`

### `set_context (overload 2)`

#### Signature

```cpp
Studio& set_context(self: Studio, context: FlashContext, width: std::optional<double> = std::nullopt, height: std::optional<double> = std::nullopt);
```

#### Notes

Primary context entry point. Use explicit dimensions or semantic context presets (`Paper`, `Notebook`, `Talk`, `Poster`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/set_context.cpp
:language: cpp
```

#### Output

```{only} html
![set_context output](_static/plots/methods/set_context.svg)
```

```{only} not html
![set_context output](_static/plots/methods/set_context.png)
```

`Source section`: `flash_theme_api.md`

### `set_style (overload 1)`

#### Signature

```cpp
Pane& set_style(self: Pane, style: GridStyle = GridStyle::Major);
```

#### Notes

Low-level style primitive for a single pane (`Darkgrid`, `Lightgrid`, `Sandgrid`, `Dark`, `Light`, `Sand`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/set_style.cpp
:language: cpp
```

#### Output

```{only} html
![set_style output](_static/plots/methods/set_style.svg)
```

```{only} not html
![set_style output](_static/plots/methods/set_style.png)
```

`Source section`: `flash_theme_api.md`

### `set_style (overload 2)`

#### Signature

```cpp
Pane& set_style(self: Pane, style: FlashStyle);
```

#### Notes

Low-level style primitive for a single pane (`Darkgrid`, `Lightgrid`, `Sandgrid`, `Dark`, `Light`, `Sand`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/set_style.cpp
:language: cpp
```

#### Output

```{only} html
![set_style output](_static/plots/methods/set_style.svg)
```

```{only} not html
![set_style output](_static/plots/methods/set_style.png)
```

`Source section`: `flash_theme_api.md`

### `tight_layout (single form)`

#### Signature

```cpp
Studio& tight_layout(self: Studio, padding: double = 6.0);
```

#### Notes

Compacts pane spacing after content has been placed in a mosaic.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/tight_layout.cpp
:language: cpp
```

#### Output

```{only} html
![tight_layout output](_static/plots/methods/tight_layout.svg)
```

```{only} not html
![tight_layout output](_static/plots/methods/tight_layout.png)
```

`Source section`: `flash_theme_api.md`

### `export4_frames (single form)`

#### Signature

```cpp
std::int64_t export4_frames(x: SeriesLike, y: SeriesLike, z: SeriesLike, w: SeriesLike, options: HyperFrameExportOptions = {});
```

#### Notes

Exports a sweep of static frames (`png`/`svg`/`pdf`) for offline animation workflows.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/export4_frames.cpp
:language: cpp
```

#### Output

```{only} html
![export4_frames output](_static/plots/methods/export4_frames.svg)
```

```{only} not html
![export4_frames output](_static/plots/methods/export4_frames.png)
```

`Source section`: `flash_scene_animation_api.md`

### `trace3d (overload 1)`

#### Signature

```cpp
Trace& trace3d(self: Trace, options: Trace3DOptions);
```

#### Notes

Post-processes a 3D trace with shading/material/lighting presets.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/trace3d.cpp
:language: cpp
```

#### Output

```{only} html
![trace3d output](_static/plots/methods/trace3d.svg)
```

```{only} not html
![trace3d output](_static/plots/methods/trace3d.png)
```

`Source section`: `flash_scene_animation_api.md`

### `trace3d (overload 2)`

#### Signature

```cpp
Trace& trace_preset3d(self: Trace, preset: TracePreset3D);
```

#### Notes

Post-processes a 3D trace with shading/material/lighting presets.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/trace3d.cpp
:language: cpp
```

#### Output

```{only} html
![trace3d output](_static/plots/methods/trace3d.svg)
```

```{only} not html
![trace3d output](_static/plots/methods/trace3d.png)
```

`Source section`: `flash_scene_animation_api.md`


## Expanded Helper Overload Catalog

This appendix adds helper/style/scene-preset overload entries from the new helper pages so the overload catalog stays comprehensive.

### `flash::axes_style (expanded helper form)`

#### Signature

```cpp
Pane& axes_style(self: Pane, style: GridStyle = GridStyle::Major);
Pane& axes_style(self: Pane, style: FlashStyle);
```

#### Notes

Alias helper for `set_style(...)` when you want axis-grid style changes without changing plotting calls.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/axes_style.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::blend_palette (expanded helper form)`

#### Signature

```cpp
PaletteRamp blend_palette(colors: std::vector<Rgba>, name: std::string = "blend");
```

#### Notes

Builds a custom palette ramp from explicit color stops.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/blend_palette.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::camera3d (expanded helper form)`

#### Signature

```cpp
Pane& camera3d(self: Pane, options: Camera3DOptions);
Pane& camera3d(self: Pane, azimuth_deg: double, elevation_deg: double, projection: ProjectionKind = ProjectionKind::Orthographic, view_angle_deg: double = 30.0);
```

#### Notes

Sets camera orientation/projection on a 3D pane.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/camera3d.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_scene_presets_api.md`

### `flash::camera_preset3d (expanded helper form)`

#### Signature

```cpp
Pane& camera_preset3d(self: Pane, preset: CameraPreset3D);
```

#### Notes

Applies a named camera setup (`Spatial`, `TopDown`, `Front`, `Side`, `Isometric`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/camera_preset3d.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_scene_presets_api.md`

### `flash::catplot_kind (expanded helper form)`

#### Signature

```cpp
CatPlotKind catplot_kind(kind: std::string_view);
```

#### Notes

Parses a categorical plot kind string (`strip`, `swarm`, `box`, `violin`, `bar`, `count`) into the typed enum used by `CatPlotOptions`.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/catplot_kind.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::crayon_palette (expanded helper form)`

#### Signature

```cpp
PaletteRamp crayon_palette();
```

#### Notes

Returns a fixed high-contrast crayon-like palette ramp.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/crayon_palette.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::cubehelix_palette (expanded helper form)`

#### Signature

```cpp
PaletteRamp cubehelix_palette(n: std::int64_t = 6);
```

#### Notes

Returns a cubehelix-style palette ramp with controllable stop count.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/cubehelix_palette.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::dark_palette (expanded helper form)`

#### Signature

```cpp
PaletteRamp dark_palette(color: Rgba, n: std::int64_t = 6);
```

#### Notes

Builds a ramp from black toward the supplied anchor color.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/dark_palette.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::disgrid_kind (expanded helper form)`

#### Signature

```cpp
DisGridKind disgrid_kind(kind: std::string_view);
```

#### Notes

Parses distribution-grid mode (`hist`, `density`, `ecdf`, `rug`) into `DisGridKind`.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/disgrid_kind.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::diverging_palette (expanded helper form)`

#### Signature

```cpp
PaletteRamp diverging_palette(left: Rgba, right: Rgba, n: std::int64_t = 7);
```

#### Notes

Creates a left-white-right diverging palette for signed fields.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/diverging_palette.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::flash_palette (expanded helper form)`

#### Signature

```cpp
FlashPalette flash_palette_kind(name: std::string_view);
PaletteRamp flash_palette(palette: FlashPalette = FlashPalette::Deep);
PaletteRamp flash_palette(name: std::string_view);
```

#### Notes

Maps style names (`deep`, `muted`, `pastel`, `bright`, `dark`, `colorblind`) to palette enums and ramps.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/flash_palette_kind.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::flash_save_png (expanded helper form)`

#### Signature

```cpp
void save_png(self: Studio, path: std::string_view, dpi: std::int64_t = 144);
```

#### Notes

Writes a raster PNG artifact from the current studio scene.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/flash_save_png.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::flash_set (expanded helper form)`

#### Signature

```cpp
Studio& set(self: Studio, theme: Theme = Theme::paper());
```

#### Notes

Applies a base theme to the studio with a short helper call.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/flash_set.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::flash_show (expanded helper form)`

#### Signature

```cpp
bool show(self: Studio, title: std::string_view = {}, width: std::int64_t = 1280, height: std::int64_t = 820, launch: bool = true);
```

#### Notes

Displays a studio in the HTML viewer (in docs example, `launch=false` to keep capture deterministic).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/flash_show.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::flash_theme (expanded helper form)`

#### Signature

```cpp
Theme flash_theme(style: FlashStyle = FlashStyle::Lightgrid, palette: FlashPalette = FlashPalette::Deep);
```

#### Notes

Constructs a theme object from flash style + palette presets.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/flash_theme.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::get_dataset_names (expanded helper form)`

#### Signature

```cpp
Note get_dataset_names();
```

#### Notes

Returns names of built-in demo datasets available via `load_dataset(...)`.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/get_dataset_names.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::hls_palette (expanded helper form)`

#### Signature

```cpp
PaletteRamp hls_palette(n: std::int64_t = 6, saturation: double = 0.68, lightness: double = 0.52);
```

#### Notes

Programmatic HLS wheel palette builder.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/hls_palette.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::husl_palette (expanded helper form)`

#### Signature

```cpp
PaletteRamp husl_palette(n: std::int64_t = 6);
```

#### Notes

HUSL-like palette shortcut.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/husl_palette.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::light_palette (expanded helper form)`

#### Signature

```cpp
PaletteRamp light_palette(color: Rgba, n: std::int64_t = 6);
```

#### Notes

Builds a ramp from white toward the supplied anchor color.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/light_palette.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::lighting3d (expanded helper form)`

#### Signature

```cpp
Pane& lighting3d(self: Pane, options: Lighting3DOptions);
```

#### Notes

Configures directional/light-rig composition for 3D traces.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/lighting3d.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_scene_presets_api.md`

### `flash::make_dataset (expanded helper form)`

#### Signature

```cpp
Dataset make_dataset(numeric_values: std::vector<Series>, numeric_names: std::vector<std::string> = {});
Dataset make_dataset(matrix: Series, feature_names: std::vector<std::string> = {}, feature_axis: Axis = Axis::Cols);
```

#### Notes

Creates a typed flash dataset from either explicit feature vectors or a feature matrix.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/make_dataset.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::move_legend (expanded helper form)`

#### Signature

```cpp
Pane& move_legend(self: Pane, place: KeyPlace) noexcept;
Studio& move_legend(self: Studio, dock: KeyDock) noexcept;
```

#### Notes

Repositions legend per-pane or globally per-studio.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/move_legend.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::mpl_palette (expanded helper form)`

#### Signature

```cpp
PaletteRamp mpl_palette(name: std::string_view = "viridis");
```

#### Notes

Matplotlib-style palette compatibility helper.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/mpl_palette.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::relgrid_kind (expanded helper form)`

#### Signature

```cpp
RelGridKind relgrid_kind(kind: std::string_view);
```

#### Notes

Parses relation-grid mode (`scatter`, `line`, `reg`) into `RelGridKind`.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/relgrid_kind.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::scene_preset3d (expanded helper form)`

#### Signature

```cpp
Pane& scene_preset3d(self: Pane, preset: ScenePreset3D);
```

#### Notes

Applies coordinated camera/theme/lighting defaults (`Survey`, `Presentation`, `Studio`, `Topology`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/scene_preset3d.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_scene_presets_api.md`

### `flash::theme_preset3d (expanded helper form)`

#### Signature

```cpp
Pane& theme_preset3d(self: Pane, preset: ThemePreset3D);
```

#### Notes

Applies a 3D-focused theme preset (`Paper`, `Ocean`, `Slate`, `Mist`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/theme_preset3d.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_scene_presets_api.md`

### `flash::tickless (expanded helper form)`

#### Signature

```cpp
Pane& tickless(self: Pane, enable: bool = true);
```

#### Notes

Suppresses axis framing while keeping data traces visible.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/tickless.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`

### `flash::trace_preset3d (expanded helper form)`

#### Signature

```cpp
Trace& trace_preset3d(self: Trace, preset: TracePreset3D);
```

#### Notes

Applies trace-level 3D material/shading profile (`Path`, `Cloud`, `Surface`, `Wireframe`, `Spire`, `Volume`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/trace_preset3d.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_scene_presets_api.md`

### `flash::xkcd_palette (expanded helper form)`

#### Signature

```cpp
PaletteRamp xkcd_palette();
```

#### Notes

Returns a fixed playful palette ramp.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/xkcd_palette.cpp
:language: cpp
```

#### Output

Output is visualized by running the example snippet.

`Source section`: `flash_helpers_expanded_api.md`


## Canonical Name Coverage Addendum

### `contour3D`

#### Signature

```cpp
Trace& contour3D(self: Pane, x: SeriesLike, y: SeriesLike, z: SeriesLike, level_count: int = 8);
```

#### Notes

3D contour-like ridges (alias over `ridge3`).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/contour3D.cpp
:language: cpp
```

#### Output

```{only} html
![contour3D output](_static/plots/methods/contour3D.svg)
```

```{only} not html
![contour3D output](_static/plots/methods/contour3D.png)
```

_Source section: `flash_api.md`._

### `plot3D`

#### Signature

```cpp
Trace& plot3D(self: Pane, x: SeriesLike, y: SeriesLike, z: SeriesLike);
```

#### Notes

Matplotlib-style alias for 3D path plotting.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/plot3D.cpp
:language: cpp
```

#### Output

```{only} html
![plot3D output](_static/plots/methods/plot3D.svg)
```

```{only} not html
![plot3D output](_static/plots/methods/plot3D.png)
```

_Source section: `flash_api.md`._

### `scatter3D`

#### Signature

```cpp
Trace& scatter3D(self: Pane, x: SeriesLike, y: SeriesLike, z: SeriesLike);
```

#### Notes

Matplotlib-style alias for `cloud3`.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/scatter3D.cpp
:language: cpp
```

#### Output

```{only} html
![scatter3D output](_static/plots/methods/scatter3D.svg)
```

```{only} not html
![scatter3D output](_static/plots/methods/scatter3D.png)
```

_Source section: `flash_api.md`._

### `FacetGrid`

#### Signature

```cpp
Studio& FacetGrid(self: Studio, int64_t rows, int64_t cols, double gx = 18.0, double gy = 18.0);
```

#### Notes

Allocates a faceted 2D grid layout. You then populate panes manually.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/facetgrid.cpp
:language: cpp
```

#### Output

```{only} html
![facetgrid output](_static/plots/methods/facetgrid.svg)
```

```{only} not html
![facetgrid output](_static/plots/methods/facetgrid.png)
```

_Source section: `flash_api.md`._

### `PairGrid`

#### Signature

```cpp
Studio& PairGrid(self: Studio, int64_t dimensions, double gx = 14.0, double gy = 14.0);
```

#### Notes

Creates an NxN grid intended for pairwise matrix views.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/pairgrid.cpp
:language: cpp
```

#### Output

```{only} html
![pairgrid output](_static/plots/methods/pairgrid.svg)
```

```{only} not html
![pairgrid output](_static/plots/methods/pairgrid.png)
```

_Source section: `flash_api.md`._

### `JointGrid`

#### Signature

```cpp
Studio& JointGrid(self: Studio, double gx = 14.0, double gy = 14.0);
```

#### Notes

Creates the standard joint-layout scaffold (joint pane + top/right marginals).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/jointgrid.cpp
:language: cpp
```

#### Output

```{only} html
![jointgrid output](_static/plots/methods/jointgrid.svg)
```

```{only} not html
![jointgrid output](_static/plots/methods/jointgrid.png)
```

_Source section: `flash_api.md`._

### `flash::animate3`

#### Signature

```cpp
bool animate3(x: SeriesLike, y: SeriesLike, z: SeriesLike, options: Animation3Options = {});
```

#### Notes

Interactive webview animation (not a static render call). For doc previews, use `path3`/`cloud3` outputs with the same source data.

#### Example

```cpp
pluss::pl::flash::Animation3Options options;
options.title = "3D simulation";
options.sweep_speed_deg_per_sec = 18.0;
options.snapshot_prefix = "flash3d";

const bool opened = pluss::pl::flash::animate3(x, y, z, options);
```

#### Output

```{only} html
![animate3 preview](_static/plots/methods/path3.svg)
```

```{only} not html
![animate3 preview](_static/plots/methods/path3.png)
```

_Source section: `flash_scene_animation_api.md`._

### `flash::animate4`

#### Signature

```cpp
bool animate4(x: SeriesLike, y: SeriesLike, z: SeriesLike, w: SeriesLike, options: HyperAnimationOptions = {});
```

#### Notes

Interactive geometric 4D animation in a webview with live sweep controls; use `export4_frames` for deterministic artifacts.

#### Example

```cpp
pluss::pl::flash::HyperAnimationOptions options;
options.title = "Geometric 4D simulation";
options.sweep_plane = pluss::pl::flash::HyperAnimateSweepPlane::XW;
options.sweep_speed_deg_per_sec = 22.0;

const bool opened = pluss::pl::flash::animate4(x4, y4, z4, w4, options);
```

#### Output

```{only} html
![animate4 preview](_static/plots/methods/path4.svg)
```

```{only} not html
![animate4 preview](_static/plots/methods/path4.png)
```

_Source section: `flash_scene_animation_api.md`._


## Dataset Accessor Coverage Addendum

### `Dataset::numeric`

#### Signature

```cpp
std::optional<Series> Dataset::numeric(self, std::string_view name) const;
```

#### Notes

Returns the numeric feature column with the requested name when present. If the dataset does not include
numeric names (or the name is unknown), the result is `std::nullopt`.

#### Example

```cpp
auto ds = flash::make_dataset(
    std::vector<Series>{
        pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series({10.0, 20.0, 30.0})
    },
    {"id", "sales"});

auto sales = ds.numeric("sales");
```

#### Output

```text
sales.has_value() == true
sales->numel() == 3
```

### `Dataset::note`

#### Signature

```cpp
std::optional<Note> Dataset::note(self, std::string_view name) const;
```

#### Notes

Returns the note (categorical/text) feature column with the requested name when present. If the dataset does not
include note names (or the name is unknown), the result is `std::nullopt`.

#### Example

```cpp
flash::Dataset ds;
ds.note_names = pluss::make_note({"city"});
ds.note_values = {pluss::make_note({"Accra", "Tema", "Kumasi"})};

auto city = ds.note("city");
```

#### Output

```text
city.has_value() == true
city->numel() == 3
```



### `flash::set_theme`

#### Signature

```cpp
Pane& flash::set_theme(self: Pane, theme: Theme);
Studio& flash::set_theme(self: Studio, theme: Theme);
```

#### Notes

Applies theme tokens (background, accents, surface palette, foreground) to an existing pane or entire studio.
Use the pane form when you want local overrides; use the studio form when you want global consistency.

#### Example

Pane form:

```{literalinclude} ../../examples/docs/snippets/keys/set_theme_pane.cpp
:language: cpp
```

Studio form:

```{literalinclude} ../../examples/docs/snippets/keys/set_theme_studio.cpp
:language: cpp
```

#### Output

```{only} html
![set_theme (pane) output](_static/plots/methods/set_theme_pane.svg)
![set_theme (studio) output](_static/plots/methods/set_theme_studio.svg)
```

```{only} not html
![set_theme (pane) output](_static/plots/methods/set_theme_pane.png)
![set_theme (studio) output](_static/plots/methods/set_theme_studio.png)
```
