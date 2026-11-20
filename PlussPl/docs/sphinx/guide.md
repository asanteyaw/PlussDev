<!-- generated from guide.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# PlussPl by example

PlussPl draws figures: a `Studio` holds panes, a pane holds traces, and the `flash::` layer gives a one-call function for each common plot. Each page here builds real figures from the bundled datasets in complete programs that print their numbers and save their figures; the figure under each program is the file that the program wrote. The reference says what each call does; these pages show how to combine the calls into a figure that answers a question.

1. **[Figures, panes and traces](guide/figures_and_panes.md).** The structure of a figure, a dashboard, a report-quality figure, export formats, and the mistakes that are refused.
2. **[Relationships and trends](guide/relationships_and_trends.md).** Scatter, fit, residuals, lines, steps, areas, error bars and bands.
3. **[Distributions and groups](guide/distributions_and_groups.md).** Histograms, densities, box and violin plots, counts and shares.
4. **[Matrices and many variables](guide/matrices_and_many_variables.md).** Heat maps, clustered maps and the multivariate views.
5. **[Grids and linear models](guide/grids_and_models.md).** Small multiples, mappers, ready-made grids and linear-model figures.
6. **[Time-series diagnostics](guide/time_series_diagnostics.md).** Lag, autocorrelation, cross-correlation and the bootstrap.
7. **[Fields and polar plots](guide/fields_and_polar.md).** Field maps, contours, bands and dials.
8. **[Three dimensions](guide/three_dimensions.md).** Surfaces, clouds, paths, densities, cameras, lights and themes.
9. **[Four dimensions and animation](guide/four_dimensions_and_animation.md).** 4D paths and clouds, rotations, frame export and the live viewers.
10. **[Colour and style](guide/colour_and_style.md).** Palettes, themes, contexts and your own data.
11. **[The flash style system](guide/flash_styles.md).** Six styles, six palettes and four contexts, set from one options struct.
12. **[Flash from a dataset](guide/flash_datasets.md).** Joint, pair and triplots by column name, from bundled datasets, your tables and your matrices.
13. **[Categories and matrices from a dataset](guide/flash_categories_matrices.md).** Every catplot kind and option, heat maps with range, labels and masks, and clustered maps.

## Every plot, and where it is used

The table lists every plot-drawing function and the page whose programs use it. The first block is the native traces that panes create; the second is the `flash::` functions.

| Plot | Layer | Used in |
|---|---|---|
| `trend` | native trace | [Colour and style](guide/colour_and_style.md), [Figures, panes and traces](guide/figures_and_panes.md), [Grids and linear models](guide/grids_and_models.md), [Relationships and trends](guide/relationships_and_trends.md) |
| `cloud` | native trace | [Figures, panes and traces](guide/figures_and_panes.md), [Grids and linear models](guide/grids_and_models.md) |
| `fieldmap` | native trace | [Colour and style](guide/colour_and_style.md), [Fields and polar plots](guide/fields_and_polar.md) |
| `ribbon` | native trace | [Relationships and trends](guide/relationships_and_trends.md) |
| `spike` | native trace | [Relationships and trends](guide/relationships_and_trends.md) |
| `wedge` | native trace | [Distributions and groups](guide/distributions_and_groups.md) |
| `whisk` | native trace | [Relationships and trends](guide/relationships_and_trends.md) |
| `columns` | native trace | [Figures, panes and traces](guide/figures_and_panes.md) |
| `columns_h` | native trace | [Distributions and groups](guide/distributions_and_groups.md) |
| `bucket` | native trace | [Distributions and groups](guide/distributions_and_groups.md), [Figures, panes and traces](guide/figures_and_panes.md), [Grids and linear models](guide/grids_and_models.md) |
| `quartet` | native trace | [Distributions and groups](guide/distributions_and_groups.md) |
| `violin` | native trace | [Distributions and groups](guide/distributions_and_groups.md) |
| `radial` | native trace | [Fields and polar plots](guide/fields_and_polar.md) |
| `contour` | native trace | [Fields and polar plots](guide/fields_and_polar.md) |
| `isoband` | native trace | [Fields and polar plots](guide/fields_and_polar.md) |
| `cloud3` | native trace | [Three dimensions](guide/three_dimensions.md) |
| `path3` | native trace | [Three dimensions](guide/three_dimensions.md) |
| `spire3` | native trace | [Three dimensions](guide/three_dimensions.md) |
| `ridge3` | native trace | [Three dimensions](guide/three_dimensions.md) |
| `isoshell` | native trace | [Three dimensions](guide/three_dimensions.md) |
| `sheet` | native trace | [Figures, panes and traces](guide/figures_and_panes.md), [Three dimensions](guide/three_dimensions.md) |
| `lattice` | native trace | [Three dimensions](guide/three_dimensions.md) |
| [`flash::Dataset`](api/flash/Dataset.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::FacetGrid`](api/flash/FacetGrid.md) | flash | [Grids and linear models](guide/grids_and_models.md) |
| [`flash::JointGrid`](api/flash/JointGrid.md) | flash | [Grids and linear models](guide/grids_and_models.md) |
| [`flash::PairGrid`](api/flash/PairGrid.md) | flash | [Grids and linear models](guide/grids_and_models.md) |
| [`flash::acfplot`](api/flash/acfplot.md) | flash | [Time-series diagnostics](guide/time_series_diagnostics.md) |
| [`flash::actplot`](api/flash/actplot.md) | flash | [Time-series diagnostics](guide/time_series_diagnostics.md) |
| [`flash::andrews_curves`](api/flash/andrews_curves.md) | flash | [Matrices and many variables](guide/matrices_and_many_variables.md) |
| [`flash::animate3`](api/flash/animate3.md) | flash | [Four dimensions and animation](guide/four_dimensions_and_animation.md) |
| [`flash::animate4`](api/flash/animate4.md) | flash | [Four dimensions and animation](guide/four_dimensions_and_animation.md) |
| [`flash::areaplot`](api/flash/areaplot.md) | flash | [Relationships and trends](guide/relationships_and_trends.md) |
| [`flash::bar3d`](api/flash/bar3d.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::barplot`](api/flash/barplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::blend_palette`](api/flash/blend_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::bootstrapplot`](api/flash/bootstrapplot.md) | flash | [Time-series diagnostics](guide/time_series_diagnostics.md) |
| [`flash::boxenplot`](api/flash/boxenplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::boxplot`](api/flash/boxplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::camera3d`](api/flash/camera3d.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::camera_preset3d`](api/flash/camera_preset3d.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::catgridplot`](api/flash/catgridplot.md) | flash | [Grids and linear models](guide/grids_and_models.md) |
| [`flash::catplot`](api/flash/catplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::ccfplot`](api/flash/ccfplot.md) | flash | [Time-series diagnostics](guide/time_series_diagnostics.md) |
| [`flash::cloud3`](api/flash/cloud3.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::cloud4`](api/flash/cloud4.md) | flash | [Four dimensions and animation](guide/four_dimensions_and_animation.md) |
| [`flash::clustermap`](api/flash/clustermap.md) | flash | [Matrices and many variables](guide/matrices_and_many_variables.md) |
| [`flash::color_palette`](api/flash/color_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::contour3D`](api/flash/contour3D.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::countplot`](api/flash/countplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::crayon_palette`](api/flash/crayon_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::cubehelix_palette`](api/flash/cubehelix_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::dark_palette`](api/flash/dark_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::despine`](api/flash/despine.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::disgridplot`](api/flash/disgridplot.md) | flash | [Grids and linear models](guide/grids_and_models.md) |
| [`flash::displot`](api/flash/displot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::diverging_palette`](api/flash/diverging_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::ecdfplot`](api/flash/ecdfplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::export4_frames`](api/flash/export4_frames.md) | flash | [Four dimensions and animation](guide/four_dimensions_and_animation.md) |
| [`flash::facet_map_plot`](api/flash/facet_map_plot.md) | flash | [Grids and linear models](guide/grids_and_models.md) |
| [`flash::flash_palette`](api/flash/flash_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::flash_theme`](api/flash/flash_theme.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::get_dataset_names`](api/flash/get_dataset_names.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::heatmap`](api/flash/heatmap.md) | flash | [Figures, panes and traces](guide/figures_and_panes.md), [Matrices and many variables](guide/matrices_and_many_variables.md) |
| [`flash::hist3d`](api/flash/hist3d.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::histplot`](api/flash/histplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::hls_palette`](api/flash/hls_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::horizontal_grid`](api/flash/horizontal_grid.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::husl_palette`](api/flash/husl_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::hyper_animation_sequence`](api/flash/hyper_animation_sequence.md) | flash | [Four dimensions and animation](guide/four_dimensions_and_animation.md) |
| [`flash::isoshell`](api/flash/isoshell.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::jointgrid_map_plot`](api/flash/jointgrid_map_plot.md) | flash | [Grids and linear models](guide/grids_and_models.md) |
| [`flash::jointplot`](api/flash/jointplot.md) | flash | [Matrices and many variables](guide/matrices_and_many_variables.md) |
| [`flash::kdeplot`](api/flash/kdeplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::lagplot`](api/flash/lagplot.md) | flash | [Time-series diagnostics](guide/time_series_diagnostics.md) |
| [`flash::lattice`](api/flash/lattice.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::legend_frame`](api/flash/legend_frame.md) | flash | [Figures, panes and traces](guide/figures_and_panes.md) |
| [`flash::light_palette`](api/flash/light_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::lighting3d`](api/flash/lighting3d.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::lineplot`](api/flash/lineplot.md) | flash | [Relationships and trends](guide/relationships_and_trends.md) |
| [`flash::lmplot`](api/flash/lmplot.md) | flash | [Grids and linear models](guide/grids_and_models.md) |
| [`flash::load_dataset`](api/flash/load_dataset.md) | flash | [Distributions and groups](guide/distributions_and_groups.md), [Fields and polar plots](guide/fields_and_polar.md), [Figures, panes and traces](guide/figures_and_panes.md), [Grids and linear models](guide/grids_and_models.md), [Matrices and many variables](guide/matrices_and_many_variables.md), [Relationships and trends](guide/relationships_and_trends.md), [Three dimensions](guide/three_dimensions.md), [Time-series diagnostics](guide/time_series_diagnostics.md) |
| [`flash::move_legend`](api/flash/move_legend.md) | flash | [Figures, panes and traces](guide/figures_and_panes.md) |
| [`flash::mpl_palette`](api/flash/mpl_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::pacfplot`](api/flash/pacfplot.md) | flash | [Time-series diagnostics](guide/time_series_diagnostics.md) |
| [`flash::pairgrid_map_plot`](api/flash/pairgrid_map_plot.md) | flash | [Grids and linear models](guide/grids_and_models.md) |
| [`flash::pairplot`](api/flash/pairplot.md) | flash | [Matrices and many variables](guide/matrices_and_many_variables.md) |
| [`flash::parallelplot`](api/flash/parallelplot.md) | flash | [Matrices and many variables](guide/matrices_and_many_variables.md) |
| [`flash::path3`](api/flash/path3.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::path4`](api/flash/path4.md) | flash | [Four dimensions and animation](guide/four_dimensions_and_animation.md) |
| [`flash::pieplot`](api/flash/pieplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::plot3D`](api/flash/plot3D.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::plot_surface`](api/flash/plot_surface.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::plot_wireframe`](api/flash/plot_wireframe.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::pointplot`](api/flash/pointplot.md) | flash | [Relationships and trends](guide/relationships_and_trends.md) |
| [`flash::qqplot`](api/flash/qqplot.md) | flash | [Relationships and trends](guide/relationships_and_trends.md) |
| [`flash::radviz`](api/flash/radviz.md) | flash | [Matrices and many variables](guide/matrices_and_many_variables.md) |
| [`flash::regplot`](api/flash/regplot.md) | flash | [Relationships and trends](guide/relationships_and_trends.md) |
| [`flash::relgridplot`](api/flash/relgridplot.md) | flash | [Grids and linear models](guide/grids_and_models.md) |
| [`flash::relplot`](api/flash/relplot.md) | flash | [Relationships and trends](guide/relationships_and_trends.md) |
| [`flash::resid_acfplot`](api/flash/resid_acfplot.md) | flash | [Time-series diagnostics](guide/time_series_diagnostics.md) |
| [`flash::resid_histplot`](api/flash/resid_histplot.md) | flash | [Relationships and trends](guide/relationships_and_trends.md) |
| [`flash::residplot`](api/flash/residplot.md) | flash | [Relationships and trends](guide/relationships_and_trends.md) |
| [`flash::ridge3`](api/flash/ridge3.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::rugplot`](api/flash/rugplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::save_png`](api/flash/save_png.md) | flash | [Figures, panes and traces](guide/figures_and_panes.md) |
| [`flash::scatter3D`](api/flash/scatter3D.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::scatterplot`](api/flash/scatterplot.md) | flash | [Relationships and trends](guide/relationships_and_trends.md) |
| [`flash::scene3d`](api/flash/scene3d.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::scene_preset3d`](api/flash/scene_preset3d.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::set_context`](api/flash/set_context.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::set_style`](api/flash/set_style.md) | flash | [Figures, panes and traces](guide/figures_and_panes.md) |
| [`flash::set_theme`](api/flash/set_theme.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::sheet`](api/flash/sheet.md) | flash | [Figures, panes and traces](guide/figures_and_panes.md), [Three dimensions](guide/three_dimensions.md) |
| [`flash::show`](api/flash/show.md) | flash | [Figures, panes and traces](guide/figures_and_panes.md) |
| [`flash::spire3`](api/flash/spire3.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::stepplot`](api/flash/stepplot.md) | flash | [Relationships and trends](guide/relationships_and_trends.md) |
| [`flash::stripplot`](api/flash/stripplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::summaryplot`](api/flash/summaryplot.md) | flash | [Matrices and many variables](guide/matrices_and_many_variables.md) |
| [`flash::swarmplot`](api/flash/swarmplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::theme_preset3d`](api/flash/theme_preset3d.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::tickless`](api/flash/tickless.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::tight_layout`](api/flash/tight_layout.md) | flash | [Colour and style](guide/colour_and_style.md) |
| [`flash::trace3d`](api/flash/trace3d.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::trace_preset3d`](api/flash/trace_preset3d.md) | flash | [Three dimensions](guide/three_dimensions.md) |
| [`flash::triplot`](api/flash/triplot.md) | flash | [Matrices and many variables](guide/matrices_and_many_variables.md) |
| [`flash::violinplot`](api/flash/violinplot.md) | flash | [Distributions and groups](guide/distributions_and_groups.md) |
| [`flash::whiskplot`](api/flash/whiskplot.md) | flash | [Relationships and trends](guide/relationships_and_trends.md) |
| [`flash::xkcd_palette`](api/flash/xkcd_palette.md) | flash | [Colour and style](guide/colour_and_style.md) |

```{toctree}
:maxdepth: 1
:hidden:

guide/figures_and_panes
guide/relationships_and_trends
guide/distributions_and_groups
guide/matrices_and_many_variables
guide/grids_and_models
guide/time_series_diagnostics
guide/fields_and_polar
guide/three_dimensions
guide/four_dimensions_and_animation
guide/colour_and_style
guide/flash_styles
guide/flash_datasets
guide/flash_categories_matrices
```
