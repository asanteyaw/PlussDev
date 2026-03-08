# Flash Roadmap (Seaborn-Style)

## Naming Doctrine

1. `flash` keeps explicit plot nouns (`pairplot`, `jointplot`, `regplot`, `actplot`, `lagplot`).
2. No aliases in public API (single canonical method name per feature).
3. `DataFrame` layer stays concise (`df.plot().act(...)`, `df.plot().lag(...)`) and delegates down.

## Locked Constraints

1. PlussPl theme ownership stays in core:
- Keep `Theme::paper` and `Theme::talk` at PlussPl level.
- Do not relocate/remove existing core theme APIs.

2. Flash theming is additive (not replacement):
- Add flash-specific advanced theme packs/profiles as wrappers on top of PlussPl themes/tokens.
- Flash may expose richer style presets, but final rendering still uses PlussPl theme primitives.

3. `DataFrame::plot()` routing stays performance-aware hybrid:
- Do not force every op through flash if it introduces avoidable materialization/copy overhead.
- Keep DF-side table semantics/prep where needed (pivot/group/reshape/type-aware resolution).
- Delegate to flash for reusable plotting kernels where parity and perf are both acceptable.

## Current State (High Level)

Already present in `flash`:
- Relational: `scatterplot`, `lineplot`, `relplot`
- Distribution: `histplot`, `displot`, `kdeplot`, `ecdfplot`, `rugplot`, `bootstrapplot`
- Categorical: `stripplot`, `swarmplot`, `boxplot`, `violinplot`, `boxenplot`, `pointplot`, `barplot`, `countplot`, `catplot`
- Regression: `regplot`, `lmplot`, `residplot`
- Matrix/Grid: `heatmap`, `clustermap`, `pairplot`, `jointplot`, `FacetGrid`, `PairGrid`, `JointGrid`
- Multivariate extras: `parallelplot`, `andrews_curves`, `radviz`, `lagplot`, `actplot`, `summaryplot`

What is still shallow vs Seaborn behavior:
- Many APIs are thin wrappers (good coverage, limited semantic depth/options).
- Faceting/semantic mapping is not yet as automatic as seaborn wide/long-data behavior.
- Several “advanced” variants are currently composed manually instead of first-class workflows.

## Priority Roadmap

### Phase 1: Option-Depth Parity (no new aliases)

1. `regplot` / `lmplot`
- Add options: `ci`, `n_boot`, `order`, `robust`, `lowess`.
Status:
- Implemented in flash:
  - `RegPlotOptions` now includes: `order`, `robust`, `lowess`, `lowess_frac`, `n_boot`, plus CI controls.
  - `regplot` supports polynomial fits, robust reweighting, LOWESS fits, and bootstrap CI bands.
  - `lmplot` delegates through the upgraded `regplot` options.
- Remaining depth item:
  - add explicit robust estimator family selection (e.g., Huber/Tukey choice) if needed.

2. `jointplot`
- Add options: `kind` (`scatter`, `kde`, `hex`, `reg`) with dedicated marginals.
Status:
- Implemented kinds: `Scatter`, `Reg`, `Hex`, `Kde`.
- Implemented marginals: `Hist`, `Kde`.
- Added density-grid controls for `Kde` (`kde_bins_x/y`, smoothing passes, contour levels).

3. `pairplot`
- Add options: `diag_kind`, `kind`, `corner`, `dropna`, sampling controls.
Status:
- Implemented in `PairPlotOptions`:
  - `diag_kind` (`Histogram`, `Density`)
  - `kind` (`Scatter`, `Reg`)
  - `corner` sugar (maps `Full -> Lower`)
  - vectorized `dropna` (row-wise finite mask across all columns)
  - sampling via existing `max_points`
- Remaining depth item:
  - add optional non-stride sampling strategy controls if needed.

4. `catplot`
- Add options: estimator + errorbars (`mean/median`, CI/PI), `order`, `hue_order`, dodge.
Status:
- Implemented in flash:
  - `CatPlotOptions` with `kind`, `estimator`, `errorbar`, `confidence_level`, `order`, `hue_order`, `dodge`.
  - grouped `bar`/`count` summaries with CI/PI/Std/SE support.
  - hue-aware categorical rendering with dodge offsets.
- Remaining depth item:
  - explicit percentile interval policy presets and bootstrapped errorbars.

5. `heatmap` / `clustermap`
- Add options: `mask`, `annot`, `fmt`, `vmin/vmax`, clustering method/metric + dendrogram controls.
Status:
- Implemented in flash:
  - `HeatmapOptions`: `annotate_levels`, `scale_guide`, `vmin/vmax`, `mask`.
  - `ClusterMapOptions`: row/col mean-based clustering toggles + delegated heatmap options.
- Remaining depth item:
  - full hierarchical clustering methods/metrics and dendrogram controls.

### Phase 2: Figure-Level Workflows

1. Add first-class faceted figure helpers built on `Studio::mosaic(...)`:
- `relgridplot`, `catgridplot`, `disgridplot`.
Status:
- Implemented in flash:
  - `relgridplot` with `RelGridPlotOptions` (`Scatter`/`Line`/`Reg`, shape hints, labels/titles, grid toggles).
  - `catgridplot` with `CatGridPlotOptions` (facet-level cat rendering, optional hue facets).
  - `disgridplot` with `DisGridPlotOptions` (`Hist`/`Density`/`Ecdf`/`Rug`, bins/points controls).
- Remaining depth item:
  - add automatic long-form semantic faceting (`row`/`col`/`hue`) directly from tidy data containers.

2. Add explicit “map” APIs for grids:
- `facet_map_plot`, `pairgrid_map_plot`, `jointgrid_map_plot`.
Status:
- Implemented in flash:
  - `facet_map_plot` (row/col pane mapper).
  - `pairgrid_map_plot` (row/col + diagonal-awareness mapper).
  - `jointgrid_map_plot` (joint + marginal mappers with reserved corner pane).
- Remaining depth item:
  - add optional shared-axis/linking policies and automatic legend consolidation across mapped panes.

### Phase 3: Statistical + TS Integration Hooks

1. Time-series hooks (to align with upcoming `PlussMa::Stats::time_series`):
- `acfplot`, `pacfplot`, `ccfplot` in `flash`.
- Backed by core TS stats when available (not duplicated in plotting layer).
Status:
- Implemented in flash:
  - `acfplot` with confidence bands and lag controls.
  - `pacfplot` (Durbin-Levinson recursion) with confidence bands.
  - `ccfplot` with symmetric lag support and confidence bands.
  - existing `actplot` now delegates to `acfplot` for consistent behavior.
- Remaining depth item:
  - route ACF/PACF/CCF statistic kernels to `PlussMa::Stats::time_series` once core TS estimators land.

2. Model diagnostics helpers:
- `qqplot`, `resid_histplot`, `resid_acfplot`.
Status:
- Implemented in flash:
  - `qqplot` (normal QQ with optional standardization + reference line).
  - `resid_histplot` (regression residual histogram).
  - `resid_acfplot` (regression residual ACF).
- Remaining depth item:
  - add optional QQ confidence envelopes and robust residual scaling presets.

## Delegation Contract (Important)

1. `DataFrame::plot()` should delegate to `flash` where a flash op exists.
2. `flash` should delegate to PlussPl primitives (`Pane`, `Studio`) only.
3. If no flash op exists, either:
- add a flash op first, then delegate from `DataFrame`, or
- document temporary direct `DataFrame -> PlussPl` path and retire it later.
4. Performance guardrail:
- No mandatory flash-hop refactor for hot paths unless benchmarks show neutral or better runtime/memory.

## Acceptance Criteria For Each New Flash Op

1. No alias duplicates.
2. Option struct exists (no argument sprawl).
3. Works with pluss-native types (`Series`, `Note`, `Timeline`) only.
4. Scales to large data via sampling/downsampling options.
5. Has one focused example in `PlussPl/examples` and one delegation example in `PlussMic/examples/table`.


## Doctrine
Loops(and nested loops) only when inevitable or needed for correctness. Linear passes preferred or vectorized Series/Note/Timeline ops where possible. Modernized cpp 20 semantics, when loops are needed and stl iterators and algos (they are not necessarily faster than classical loops but are cleaner and more compact) can do it, then they can be used rather than classical loops, other times you migh seek clarity so you can use traditional loops(they are more verbose but easier to understand), so you should manage it. Smart and vectorized delegations. No aliasing/unecessary bloat.