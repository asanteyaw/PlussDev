# flash API Reference

This page documents `pluss::pl::flash` — the ~65-function, high-level plotting surface
built on top of `Studio`/`Pane`/`Trace`. It is the convenience-first layer most users
reach for directly: one function call per plot family, backed by rich `*Options`
structs for the toggles each family exposes.

## Using This Reference

- Use the left sidebar to open one function page at a time.
- Each function page contains its own signatures, parameter details, notes, verified
  outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant
  scroll of every function.

## Coverage

This reference is being built out family by family; the sidebar always reflects exactly
what's written and verified so far — nothing here is a placeholder or a stub.

**Complete**: relational (`scatterplot`, `lineplot`, `stepplot`, `relplot`), distribution
(`histplot`, `displot`, `kdeplot`, `ecdfplot`, `areaplot`, `rugplot`), categorical
(`stripplot`, `swarmplot`, `boxplot`, `violinplot`, `boxenplot`, `pointplot`,
`whiskplot`, `barplot`, `countplot`, `pieplot`, `catplot`), regression/residual
diagnostics (`regplot`, `lmplot`, `residplot`, `qqplot`, `resid_histplot`), matrix
(`heatmap`, `clustermap`), grid constructors/fill helpers (`FacetGrid`, `PairGrid`,
`JointGrid`, `relgridplot`, `catgridplot`, `disgridplot`, `facet_map_plot`,
`pairgrid_map_plot`, `jointgrid_map_plot`), multivariate grid views (`pairplot`,
`triplot`), and multivariate curve/glyph views (`parallelplot`, `andrews_curves`,
`radviz`), `jointplot`, `summaryplot`, and time-series diagnostics (`lagplot`,
`acfplot`, `pacfplot`, `ccfplot`, `actplot`, `resid_acfplot`, `bootstrapplot`), and the
3D native family (`path3`, `cloud3`, `sheet`, `lattice`, `spire3`, `ridge3`,
`isoshell`), the 3D convenience aliases (`plot3D`, `scatter3D`,
`plot_surface`, `plot_wireframe`, `contour3D`, `bar3d`, `hist3d`), and the 3D/4D
scene/camera/lighting/preset/animation helpers (`camera3d`, `camera_preset3d`,
`lighting3d`, `theme_preset3d`, `scene_preset3d`, `scene3d`, `trace_preset3d`,
`trace3d`, `path4`, `cloud4`, `hyper_animation_sequence`, `animate3`, `animate4`,
`export4_frames`), the 2D theming/context family (`set`/`set_theme`,
`set_style`/`axes_style`, `set_context`/`plotting_context`), the legend/grid/layout/
export helpers (`move_legend`, `legend`, `legend_frame`, `tight_layout`, `despine`,
`tickless`, `horizontal_grid`, `show`, `save_png`), the palette-generation family
(`flash_palette`, `color_palette`, `flash_theme`, `blend_palette`, `hls_palette`,
`husl_palette`, `cubehelix_palette`, `light_palette`, `dark_palette`,
`diverging_palette`, `mpl_palette`, `xkcd_palette`, `crayon_palette`), and the bundled
dataset family (`get_dataset_names`, `load_dataset`, `Dataset::from_frame`,
`Dataset::frame`) families.

Every function in `pluss::pl::flash` is now documented and verified. A number of
lower-level helpers that previously lived directly at `flash::` scope (`catplot_kind`,
`ordered_levels`, `relgrid_kind`, `disgrid_kind`, `resolve_grid_shape`,
`dataset_numeric_columns`, `triplot_feature_indices`, `triplot_triplets`,
`triplot_bin_surface`, `deg_to_rad`, `rotate_plane`, `rotate4`, `inside_slice`,
`project4`, `project4_batch`, `apply_horizontal_grid_policy`, `flash_context_values`,
`flash_palette_kind`, `flash_palette_colors`, `flash_palette_ramp`) have been moved
into `flash::detail::`, where their internal-implementation nature is unambiguous —
they were never part of this reference's public surface.

```{toctree}
:hidden:
:maxdepth: 1

api/flash/scatterplot
api/flash/lineplot
api/flash/stepplot
api/flash/relplot
api/flash/histplot
api/flash/displot
api/flash/kdeplot
api/flash/ecdfplot
api/flash/areaplot
api/flash/rugplot
api/flash/stripplot
api/flash/swarmplot
api/flash/boxplot
api/flash/violinplot
api/flash/boxenplot
api/flash/pointplot
api/flash/whiskplot
api/flash/barplot
api/flash/countplot
api/flash/pieplot
api/flash/catplot
api/flash/regplot
api/flash/lmplot
api/flash/residplot
api/flash/qqplot
api/flash/resid_histplot
api/flash/heatmap
api/flash/clustermap
api/flash/FacetGrid
api/flash/PairGrid
api/flash/JointGrid
api/flash/relgridplot
api/flash/catgridplot
api/flash/disgridplot
api/flash/facet_map_plot
api/flash/pairgrid_map_plot
api/flash/jointgrid_map_plot
api/flash/pairplot
api/flash/triplot
api/flash/parallelplot
api/flash/andrews_curves
api/flash/radviz
api/flash/jointplot
api/flash/summaryplot
api/flash/lagplot
api/flash/acfplot
api/flash/pacfplot
api/flash/ccfplot
api/flash/actplot
api/flash/resid_acfplot
api/flash/bootstrapplot
api/flash/path3
api/flash/cloud3
api/flash/sheet
api/flash/lattice
api/flash/spire3
api/flash/ridge3
api/flash/isoshell
api/flash/plot3D
api/flash/scatter3D
api/flash/plot_surface
api/flash/plot_wireframe
api/flash/contour3D
api/flash/bar3d
api/flash/hist3d
api/flash/camera3d
api/flash/camera_preset3d
api/flash/lighting3d
api/flash/theme_preset3d
api/flash/scene_preset3d
api/flash/scene3d
api/flash/trace_preset3d
api/flash/trace3d
api/flash/path4
api/flash/cloud4
api/flash/hyper_animation_sequence
api/flash/animate3
api/flash/animate4
api/flash/export4_frames
api/flash/set_theme
api/flash/set_style
api/flash/set_context
api/flash/move_legend
api/flash/legend_frame
api/flash/tight_layout
api/flash/despine
api/flash/tickless
api/flash/horizontal_grid
api/flash/show
api/flash/save_png
api/flash/flash_palette
api/flash/color_palette
api/flash/flash_theme
api/flash/blend_palette
api/flash/hls_palette
api/flash/husl_palette
api/flash/cubehelix_palette
api/flash/light_palette
api/flash/dark_palette
api/flash/diverging_palette
api/flash/mpl_palette
api/flash/xkcd_palette
api/flash/crayon_palette
api/flash/get_dataset_names
api/flash/load_dataset
api/flash/dataset_from_frame
api/flash/dataset_frame
```
