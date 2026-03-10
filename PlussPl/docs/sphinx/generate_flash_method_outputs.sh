#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BIN="${BUILD_DIR}/PlussPl/examples/plusspl_flash_api_capture_demo"
OUT_DIR="${ROOT_DIR}/PlussPl/docs/sphinx/_static/plots/methods"

keys=(
  acfplot actplot andrews_curves areaplot bar3d barplot bootstrapplot boxenplot boxplot
  catgridplot catplot ccfplot cloud3 cloud4 clustermap color_palette contour3D countplot
  core_axis_layout_exhaustive core_axis_layout_exhaustive_3d
  core_bucket core_cloud core_cloud3 core_columns core_columns_h core_contour
  core_fieldmap core_isoband core_isoshell core_lattice core_pane_alignment
  core_pane_axes_limits core_pane_grid_legend core_pane_guides_annotations
  core_pane_scales core_pane_tick_format core_pane_tick_rotation_gaps
  core_pane_ticks_categories core_pane_visibility core_path3 core_quartet
  core_radial core_ribbon core_ridge3 core_save_pdf core_save_png core_save_svg
  core_sheet core_spike core_spire3 core_studio_captions_row_titles
  core_studio_context_scales core_studio_mosaic core_studio_pane_style_defaults
  core_studio_subpane2 core_studio_subpane3 core_studio_theme core_studio_tight_layout core_studio_exhaustive
  core_trace_style_2d core_trace_style_3d core_trace_visibility core_trend core_violin
  core_wedge core_whisk
  despine disgridplot displot ecdfplot export4_frames facet_map_plot facetgrid heatmap hist3d histplot
  horizontal_grid isoshell jointplot kdeplot lagplot lattice lineplot lmplot load_dataset
  pacfplot pairgrid pairgrid_map_plot pairplot parallelplot path3 path4 pieplot plotting_context plot3D plot_surface
  plot_wireframe pointplot qqplot radviz regplot relgridplot relplot resid_acfplot
  resid_histplot residplot ridge3 rugplot scatter3D scatterplot scene3d set_context set_style
  set_theme_pane set_theme_studio sheet spire3 stepplot stripplot summaryplot swarmplot
  tight_layout trace3d triplot violinplot whiskplot jointgrid jointgrid_map_plot
)

mkdir -p "${OUT_DIR}"

if [[ ! -x "${BIN}" ]]; then
  echo "Building capture demo target..."
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}"
  cmake --build "${BUILD_DIR}" --target plusspl_flash_api_capture_demo -j8
fi

echo "Generating SVG outputs..."
for key in "${keys[@]}"; do
  echo "  - ${key}"
  "${BIN}" "${key}" "${OUT_DIR}/${key}.svg"
done

if command -v rsvg-convert >/dev/null 2>&1; then
  echo "Generating PNG fallbacks for PDF..."
  for svg in "${OUT_DIR}"/*.svg; do
    png="${svg%.svg}.png"
    rsvg-convert -w 1500 -h 980 "${svg}" -o "${png}"
  done
else
  echo "rsvg-convert not found; skipped PNG fallback generation."
fi

echo "Done. Outputs in: ${OUT_DIR}"
