# Seaborn -> PlussPl Parity Map

| Seaborn | PlussPl |
|---|---|
| `sns.relplot` | `Studio::mosaic(...)` + `Pane::trend(...)` / `Pane::cloud(...)` |
| `sns.scatterplot` | `Pane::cloud(...)` |
| `sns.lineplot` | `Pane::trend(...)` |
| `sns.displot` | Composed: `Pane::bucket(...)`, `Pane::quartet(...)`, `Pane::violin(...)`, `Pane::fieldmap(...)` |
| `sns.histplot` | `Pane::bucket(...)` |
| `sns.kdeplot` | Composed (precompute KDE externally, then `Pane::trend(...)` or `Pane::violin(...)`) |
| `sns.ecdfplot` | Composed (precompute ECDF externally, then `Pane::trend(...)`) |
| `sns.rugplot` | `Pane::spike(...)` (short baseline spikes) |
| `sns.catplot` | `Studio::mosaic(...)` + categorical traces (`columns`, `quartet`, `violin`, `cloud`) |
| `sns.stripplot` | `Pane::cloud(...)` + categorical axis (`x_categories` / `y_categories`) |
| `sns.swarmplot` | Composed (`Pane::cloud(...)` + external swarm/jitter packing) |
| `sns.boxplot` | `Pane::quartet(...)` |
| `sns.violinplot` | `Pane::violin(...)` |
| `sns.boxenplot` | No first-class equivalent yet |
| `sns.pointplot` | `Pane::trend(...)` + `Pane::whisk(...)` |
| `sns.barplot` | `Pane::columns(...)` |
| `sns.countplot` | `Pane::columns(...)` (with pre-aggregated counts) or `Pane::bucket(...)` |
| `sns.regplot` | Composed (`Pane::cloud(...)` + fitted `Pane::trend(...)`) |
| `sns.lmplot` | `Studio::mosaic(...)` + composed regplot per pane |
| `sns.residplot` | `Pane::cloud(...)` (residuals precomputed externally) |
| `sns.heatmap` | `Pane::fieldmap(...)` |
| `sns.clustermap` | Composed (external clustering/reordering + `Pane::fieldmap(...)`) |
| `sns.FacetGrid` | `Studio::mosaic(...)` + pane placement |
| `sns.PairGrid` | Composed `Studio::mosaic(...)` matrix |
| `sns.JointGrid` | Composed `Studio::mosaic(...)` with joint + marginal panes |
| `sns.pairplot` | Composed PairGrid with `cloud`/`bucket`/`trend` |
| `sns.jointplot` | Composed JointGrid with `cloud`/`trend` + marginals |
| `sns.color_palette` | `PaletteRamp` (custom stops) |
| `sns.hls_palette` | `PaletteRamp` (manual/custom) |
| `sns.husl_palette` | `PaletteRamp` (manual/custom) |
| `sns.cubehelix_palette` | `PaletteRamp` (manual/custom) |
| `sns.light_palette` | `PaletteRamp` (manual/custom) |
| `sns.dark_palette` | `PaletteRamp` (manual/custom) |
| `sns.diverging_palette` | `PaletteRamp` (manual/custom) |
| `sns.blend_palette` | `PaletteRamp` (manual/custom) |
| `sns.mpl_palette` | `PaletteRamp` (manual/custom) |
| `sns.xkcd_palette` | `PaletteRamp` (manual/custom) |
| `sns.crayon_palette` | `PaletteRamp` (manual/custom) |
| `sns.set` | `Studio(..., Theme ...)` + pane/style mutators |
| `sns.set_theme` | `Studio(..., Theme ...)` |
| `sns.set_style` | `Theme` + pane grid/key/axis style settings |
| `sns.axes_style` | Per-pane style via `Pane` axis/grid settings |
| `sns.set_context` | `Studio::size(...)` + font/tick sizing defaults |
| `sns.plotting_context` | `Studio` size/layout configuration |
| `sns.despine` | No first-class equivalent yet |
| `sns.move_legend` | `Pane::key(...)` and `Studio::trace_key(...)` |
| `sns.load_dataset` | No built-in dataset loader |
| `sns.get_dataset_names` | No built-in dataset registry |
