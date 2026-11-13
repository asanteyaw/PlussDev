#pragma once

#include "pluss/pl/studio.h"

#include <cmath>
#include <cstddef>

namespace pluss::pl {

// Two values of one quantity: the relaxed layout and the compact one (see Studio::tight_layout).
struct StyleSpacing {
  double relaxed;
  double compact;
  [[nodiscard]] constexpr double of(bool tight) const noexcept { return tight ? compact : relaxed; }
};

// The coefficients of the shading of a surface for one material.
struct StyleMaterial {
  double ambient;         // light a surface always has
  double diffuse;         // share of the light that scatters
  double specular;        // strength of the highlight
  double exponent;        // sharpness of the highlight
  double specular_color;  // brightness of the highlight colour
};

// The look of a figure as numbers: every size, spacing, stroke, colour and proportion that is a matter of taste, each with
// its default. A figure draws with these defaults unless Studio::style gives it another set, so every figure of a program
// can be restyled by one call and nothing is changed by editing the library.
//
// Rules the values follow:
//  * a size is in points (pt) or in em, a multiple of the font size it sits beside, so it follows the font and its scale;
//  * a quantity that depends on the content (a gap that depends on the tick labels, a wrap width that depends on the
//    figure) is not here: it is worked out when the figure is drawn;
//  * a value marked "guard" only keeps the drawing sane (a visible dot, a line that does not vanish) and is never a way to
//    make a distance look right.
struct StyleOptions {
  // ---- Fonts (pt). The font scales of a pane or a studio multiply these. ------------------------------------
  double tick_font_pt{10.0}; // size of the tick labels
  double axis_label_font_pt{11.0}; // size of the axis labels
  double headline_font_pt{18.0}; // size of the figure's headline
  double caption_font_pt{12.0}; // size of the shared captions of aligned panes
  double cue_label_font_pt{10.0}; // size of the labels of rules, bands and pins

  // ---- Planar axes (em of the tick font, or of the axis-label font where stated). ----------------------------
  double tick_length_em{0.45}; // tick mark length
  double tick_text_gap_em{0.74}; // from the end of a tick to its text
  double rotated_tick_text_gap_base_em{0.28}; // the gap of a rotated tick label, before the rotation gain
  double rotated_tick_text_gap_gain_em{0.16}; // added in proportion to the rotation (0 at 0 deg, full at 90)
  double bordered_virtual_tick_factor{1.0}; // clearance kept for a tick on a pane with axes
  double borderless_virtual_tick_factor{0.35}; // and on a pane without them
  double y_axis_optical_comp_em{0.18}; // y tick labels are end-anchored and look farther away
  double y_axis_optical_comp_rotation_attenuation{0.6}; // how much of the optical shift of y tick labels is kept when they are rotated
  double axis_label_gap_x_em{0.85}; // of the axis-label font: axis label from the tick text
  double axis_label_gap_y_em{0.75}; // gap between the y axis label and the tick text, in em of the axis-label font
  double axis_label_gap_x_no_axes_em{0.55}; // gap between the x axis label and the tick text on a pane without axes
  double axis_label_gap_y_no_axes_em{0.45}; // gap between the y axis label and the tick text on a pane without axes

  // ---- 3D axes (em of the tick font, or of the axis-label font where stated). -------------------------------
  double spatial_tick_length_em{0.6}; // length of a tick mark on the x and y axes of a 3D pane
  double spatial_z_tick_length_em{0.55}; // length of a tick mark on the z axis of a 3D pane
  double spatial_tick_text_gap_em{1.4}; // gap from a tick to its text on a 3D pane
  double spatial_axis_label_base_gap_em{2.1}; // of the axis-label font
  double text_ascent_em{0.8}; // a line of text above its baseline
  double text_descent_em{0.2}; // and below it
  double estimated_glyph_width_em{0.56}; // average advance of a glyph, to size a label before it is drawn
  double tick_text_clearance_em{0.35}; // clearance kept below the tick labels of a planar axis
  double axis_label_baseline_em{0.85}; // from the top of an axis label to its baseline

  // ---- Headline (em of the headline font). -------------------------------------------------------------------
  double headline_line_height_em{1.22}; // distance between the lines of a wrapped headline
  double headline_side_margin_em{1.5}; // room kept free at each side of the headline when it wraps
  double headline_first_baseline_em{1.78}; // first baseline from the top of the figure
  double headline_min_wrap_width_em{6.0}; // never wrap into a column narrower than this

  // ---- Shared captions and cues (em of their own font). ------------------------------------------------------
  double caption_below_plot_em{3.0}; // x caption below the plot area
  double caption_left_of_ticks_em{1.1}; // y caption (turned) left of the tick labels
  double caption_above_left_x_em{0.67}; // z caption offsets from the plot corner
  double caption_above_left_y_em{0.83}; // height of a shared z caption above its plot
  double cue_label_from_edge_em{1.4}; // a cue label from the edge of the plot
  double cue_label_beside_line_em{0.4}; // and beside its line
  double cue_label_inset_em{1.0}; // distance of a rule or band label from the edge of the plot
  double pin_mark_em{0.25}; // half the side of the square at a pin anchor
  double pin_text_offset_x_em{0.4}; // horizontal gap between a pin's leader and its text
  double pin_text_offset_y_em{0.2}; // vertical gap between a pin's leader and its text
  double cue_band_stroke_pt{0.7}; // width of the outline of a band
  double pin_leader_stroke_pt{0.9}; // width of a pin's leader line
  double pin_mark_stroke_pt{0.5}; // width of the outline of a pin's mark

  // ---- Marks. -------------------------------------------------------------------------------------------------
  double marker_radius_per_sqrt_size{0.31}; // radius of a cloud marker = this * sqrt(size), uncapped
  double marker3d_radius_per_sqrt_size{0.33}; // radius of a 3D marker per square root of its size

  // ---- Guards (documented; each is a validity, safety or resource limit, not a layout choice). ---------------
  double min_font_pt{1.0}; // a font stays positive-sized
  double min_marker_radius_pt{0.95}; // a marker never shrinks below a visible dot
  double min_marker3d_radius_pt{2.0}; // smallest radius of a 3D marker (guard)

  // ---- Layout spacing (pt). A pair is {relaxed, compact}: the compact value is used when tight_layout is on. ----

  StyleSpacing pane_top_inset{58.0, 44.0}; // room above the plot area of a pane
  StyleSpacing pane_left_inset{38.0, 28.0}; // room left of the plot area, before the tick labels are added
  StyleSpacing pane_right_inset{28.0, 20.0}; // room right of the plot area
  StyleSpacing pane_bottom_inset{56.0, 42.0}; // room below the plot area, before the tick labels are added
  StyleSpacing pane_outside_legend_right_inset{212.0, 176.0}; // room right of the plot for a legend placed outside it
  StyleSpacing bare_left_inset{16.0, 12.0}; // a pane with no axis text at all
  StyleSpacing bare_bottom_inset{22.0, 18.0}; // room below a pane that has no axis text
  StyleSpacing bare_right_inset{16.0, 12.0}; // room right of a pane that has no axis text
  double empty_axis_label_pad{6.0}; // reserved beside an axis that has no label

  double reference_figure_width_pt{1280.0}; // width of the figure the edge margins were designed for; margins are shares of the real width
  double reference_figure_height_pt{820.0}; // height of the figure the edge margins were designed for
  StyleSpacing figure_edge_share_x{28.0 / reference_figure_width_pt, 18.0 / reference_figure_width_pt}; // of the width: left and right
  StyleSpacing figure_edge_share_y{28.0 / reference_figure_height_pt, 18.0 / reference_figure_height_pt}; // of the height: top and bottom
  StyleSpacing figure_caption_column_em{26.0 / caption_font_pt, 16.0 / caption_font_pt}; // a y caption's column on the left
  StyleSpacing figure_caption_row_em{28.0 / caption_font_pt, 22.0 / caption_font_pt}; // an x caption's row at the bottom
  StyleSpacing figure_headline_clearance_em{30.0 / headline_font_pt, 22.0 / headline_font_pt}; // the headline above the panes

  double mosaic_gutter_share{0.03}; // automatic gutter between cells of a mosaic, as a share of a cell
  double min_tight_gutter_pt{6.0}; // guard: a tight gutter never closes up
  double tight_gutter_factor{0.62}; // a tight gutter is this share of the requested one

  StyleSpacing volume_near_pad{12.0, 8.0}; // room kept around a 3D plot on the side nearer the viewer
  StyleSpacing volume_far_pad{14.0, 10.0}; // room kept around a 3D plot on the far side
  double volume_left_label_share{0.6}; // share of the left margin of a 3D pane given to its labels
  double volume_right_label_share{0.65}; // share of the right margin of a 3D pane given to its labels
  double volume_bottom_tick_share{0.55}; // share of the bottom margin of a 3D pane given to tick labels
  double volume_bottom_label_share{0.8}; // share of the bottom margin of a 3D pane given to axis labels
  double volume_top_tick_share{0.45}; // share of the top margin of a 3D pane given to tick labels
  double volume_title_em{1.1}; // of the axis-label font

  // ---- Legend / key box (pt, at the key font). ---------------------------------------------------------------
  double key_font_pt{11.0}; // size of the text of a legend
  double key_pad_top_pt{10.0}; // padding above the first entry of a legend box
  double key_pad_bottom_pt{10.0}; // padding below the last entry
  double key_pad_left_pt{10.0}; // padding left of the entries
  double key_pad_right_pt{10.0}; // padding right of the entries
  double key_icon_span_pt{24.0}; // the line or swatch that introduces an entry
  double key_label_gap_pt{8.0}; // from the icon to the label text
  double key_title_height_pt{18.0}; // height of a legend's title line
  double key_title_gap_pt{2.0}; // gap between a legend's title and its entries
  double key_line_height_pt{12.0}; // distance between the lines of a wrapped legend label
  double key_row_pad_pt{4.0}; // space between two entries
  double key_row_min_height_pt{16.0}; // least height of an entry row
  double key_column_gap_pt{18.0}; // gap between the columns of a legend
  double key_min_label_width_pt{24.0}; // guard: a label column never collapses
  double key_title_baseline_pt{13.0}; // title baseline below the top padding
  double key_first_baseline_pt{10.0}; // first text line baseline below its row top
  double key_swatch_half_height_pt{5.0}; // half the height of the sample of a trace in a legend
  double key_swatch_stroke_pt{0.6}; // width of the outline of a legend sample
  double key_framed_stroke_pt{0.7}; // width of the border of a framed legend
  double key_soft_stroke_pt{0.6}; // width of the border of a soft legend
  double key_framed_stroke_alpha{0.72}; // opacity of the border of a framed legend
  double key_soft_fill_alpha{0.94}; // opacity of the background of a soft legend
  double key_soft_stroke_alpha{0.28}; // opacity of the border of a soft legend
  double key_soft_fill_white_mix{0.08}; // share of white mixed into the background of a soft legend
  double key_soft_stroke_background_mix{0.78}; // share of the page colour mixed into the border of a soft legend
  double key_inset_pt{8.0}; // an inset legend from the edges of the plot area
  double key_outside_gap_pt{22.0}; // an outside legend from the plot area
  double key_min_width_pt{126.0}; // narrowest legend box
  double key_min_width_outside_pt{168.0}; // narrowest legend box placed outside the plot
  double key_max_width_outside_pt{280.0}; // widest legend box placed outside the plot
  double key_max_width_plot_share{0.52}; // of the plot width
  double key_max_width_floor_pt{160.0}; // guard: never narrower than this
  double key_max_height_floor_pt{48.0}; // guard
  double legend_overlap_sample_step_pt{4.0}; // spacing of the samples along a line when Auto looks for the emptiest corner
  double key_max_height_margin_pt{16.0}; // left free inside the plot height
  double key_target_ratio_horizontal{1.7}; // width to height a horizontal legend aims at
  double key_target_ratio_vertical{0.72}; // width to height a vertical legend aims at
  double key_target_ratio_auto{1.0}; // width to height an automatic legend aims at
  int key_width_samples{10}; // number of widths tried when fitting a legend box
  std::size_t key_max_label_lines{4}; // readability policy: a legend label is cut (visibly, with "...") after this many lines
  double key_truncation_score{10.0}; // a width that cuts a label is scored worse than any width that does not
  double key_overflow_base_score{5.0}; // penalty for a legend label that does not fit, when choosing a width
  double key_overflow_gain_score{8.0}; // extra penalty for each further line a label needs
  double key_area_weight{0.03}; // weight of the box area when choosing a legend width
  double dock_min_width_east_pt{168.0}; // narrowest legend docked to the east edge
  double dock_min_width_south_pt{220.0}; // narrowest legend docked to the south edge
  double dock_east_width_share{0.24}; // of the figure width
  double dock_east_width_min_pt{220.0}; // guard: the east dock is never narrower than this; above it, it is a share of the figure
  double dock_east_height_floor_pt{64.0}; // least height of an east dock (guard)
  double dock_east_height_margin_pt{62.0}; // room left above and below an east dock
  double dock_east_height_margin_with_headline_pt{92.0}; // the same when the figure has a headline
  double dock_east_right_inset_pt{28.0}; // distance of an east dock from the right edge
  double dock_east_plot_gap_pt{28.0}; // gap between the plots and an east dock
  double dock_south_plot_gap_pt{30.0}; // kept between the panes and a south dock        // kept between the panes and an east dock
  double dock_east_top_pt{28.0}; // distance of an east dock from the top edge
  double dock_east_top_with_headline_pt{58.0}; // the same when the figure has a headline
  double dock_bottom_inset_pt{22.0}; // distance of the plots from the bottom edge when a dock is at the south
  double dock_south_side_margin_pt{108.0}; // room kept at each side of a south dock
  double dock_south_height_share{0.26}; // share of the figure height a south dock may take
  double dock_south_height_floor_pt{52.0}; // least height of a south dock (guard)
  double dock_south_bottom_pt{18.0}; // distance of a south dock from the bottom edge
  double dock_south_side_inset_pt{22.0}; // distance of a south dock from the side edges

  // ---- Planar axes, grids and polar rings: strokes (pt) and colours. -------------------------------------------------
  double grid_minor_stroke_pt{0.5}; // width of a minor grid line
  double grid_major_stroke_pt{0.8}; // width of a major grid line
  double axis_line_stroke_pt{1.0}; // width of the axis lines
  double axis_close_stroke_pt{0.8}; // the closing lines of a boxed axis
  double tick_stroke_pt{0.9}; // width of the tick marks
  double tick_color_foreground_share{0.7}; // tick marks are the foreground at this brightness
  double axis_color_foreground_share{0.9}; // share of the foreground colour used for the axes
  double dark_background_luma{0.5}; // a background darker than this uses the dark grid colours
  double luma_red{0.2126}; // weight of red in the brightness of a colour
  double luma_green{0.7152}; // weight of green in the brightness of a colour
  double luma_blue{0.0722}; // weight of blue in the brightness of a colour
  int polar_ring_count{4}; // number of rings of a polar pane
  int polar_ring_segments{96}; // segments used to draw a ring
  int polar_spoke_count{12}; // number of spokes of a polar pane
  double polar_outer_ring_stroke_pt{1.0}; // width of the outer ring
  double polar_ring_stroke_pt{0.6}; // width of the inner rings
  double polar_spoke_stroke_pt{0.6}; // width of the spokes

  Rgba polar_ring_color = Rgba::from_u8(195, 203, 214); // colour of the rings
  Rgba polar_spoke_color = Rgba::from_u8(215, 221, 230); // colour of the spokes
  Rgba plot_face_dark_grid = Rgba::from_u8(239, 242, 246); // plot background of the dark-grid style
  Rgba plot_face_sand = Rgba::from_u8(250, 244, 225); // plot background of the sand style
  Rgba plot_face_styled = Rgba::from_u8(231, 236, 242); // plot background of the other styled themes
  Rgba minor_grid_dark_grid = Rgba::from_u8(184, 192, 204, 186); // minor grid colour of the dark-grid style
  Rgba minor_grid_sand = Rgba::from_u8(222, 212, 182); // minor grid colour of the sand style
  Rgba minor_grid_dark_background = Rgba::from_u8(73, 86, 104, 170); // minor grid colour on a dark page
  Rgba minor_grid_light = Rgba::from_u8(234, 238, 244); // minor grid colour on a light page
  Rgba major_grid_dark_grid = Rgba::from_u8(151, 163, 181, 210); // major grid colour of the dark-grid style
  Rgba major_grid_sand = Rgba::from_u8(194, 178, 139); // major grid colour of the sand style
  Rgba major_grid_dark_background = Rgba::from_u8(96, 111, 131, 190); // major grid colour on a dark page
  Rgba major_grid_light = Rgba::from_u8(224, 229, 236); // major grid colour on a light page

  // ---- Planar traces (pt unless stated). ---------------------------------------------------------------------------
  double trace_line_stroke_pt{0.9}; // thin lines of a mark: whiskers, caps, leaders, outlines
  double trace_thin_line_stroke_pt{0.8}; // width of thin lines such as stems and outlines
  double trace_fine_line_stroke_pt{0.75}; // width of the finest lines of a trace
  double trace_marker_outline_stroke_pt{0.8}; // outline of a marker drawn over a mark
  double trace_box_min_median_stroke_pt{1.2}; // the median of a box is never thinner than this
  double trace_quartet_min_median_stroke_pt{1.25}; // least width of the median line of a box
  double error_cap_half_width_pt{4.0}; // half the width of the cap of an error bar
  double marker_fill_alpha{0.82}; // translucency of a cloud marker fill
  double marker_min_fill_alpha{0.18}; // guard: a marker never turns invisible
  double marker_steps_per_radius{1.8}; // polygon steps of a marker per point of radius
  int marker_min_steps{6}; // guard: a marker is at least a hexagon
  double outline_shade{0.55}; // outline of a filled mark: the fill at this brightness

  // ---- Scale guide (the colour bar beside a pane), pt. ---------------------------------------------------------------
  double scale_guide_bar_gap_axes_pt{22.0}; // from the plot area when the pane has axes
  double scale_guide_bar_gap_bare_pt{7.0}; // and when it has none
  double scale_guide_bar_width_pt{14.0}; // width of the colour bar
  double scale_guide_tick_length_pt{5.0}; // length of the ticks of the colour bar
  double scale_guide_tick_label_pad_axes_pt{8.0}; // gap between the bar and its tick labels on a pane with axes
  double scale_guide_tick_label_pad_bare_pt{6.0}; // the same on a pane without axes
  StyleSpacing scale_guide_label_allowance_axes{40.0, 34.0}; // room for the tick labels, {relaxed, compact}
  StyleSpacing scale_guide_label_allowance_bare{28.0, 22.0}; // room kept right of the bar for its labels on a pane without axes
  int scale_guide_slices{64}; // slices of the gradient bar
  int scale_guide_tick_count{5}; // number of ticks on the colour bar
  double scale_guide_stroke_pt{0.8}; // width of the outline of the colour bar
  double scale_guide_font_pt{9.5}; // size of the labels of the colour bar
  double scale_guide_label_x_pt{8.0}; // distance of the bar's labels from the bar
  double scale_guide_label_baseline_pt{4.0}; // vertical offset of the bar's labels from their ticks

  // ---- Pane caption block and row titles. ----------------------------------------------------------------------------
  double pane_caption_font_pt{10.0}; // size of a pane's own caption
  double pane_caption_color_share{0.85}; // share of the foreground colour used for a pane caption
  double pane_caption_inset_x_pt{10.0}; // single line: from the pane's left edge
  double pane_caption_from_bottom_pt{14.0}; // single line: above the pane's bottom edge
  double pane_caption_line_gap_pt{2.0}; // space between the lines of a pane caption
  double pane_caption_block_inset_x_pt{8.0}; // several lines: inside the plot area
  double pane_caption_block_top_pt{16.0}; // gap between the plot and a pane caption
  double pane_caption_block_bottom_margin_pt{8.0}; // room kept below a pane caption
  double row_title_font_pt{12.5}; // size of a row title
  double row_title_below_edge_pt{7.0}; // min_y without a headline: from the top, beyond the font
  double row_title_below_headline_pt{54.0}; // min_y with a headline
  double row_title_below_headline_font_share{0.45}; // clearance of a row title below the headline, as a share of the headline font
  double row_title_above_row_em{0.7}; // the title baseline above the row, in em of the font
  double row_title_above_row_pt{4.0}; // gap between a row title and its row

  // ---- Pane title, 3D grids and 3D cues. ----------------------------------------------------------------------------
  double pane_title_font_pt{14.0}; // size of a pane's title
  double pane_title_baseline_em{1.5714285714285714}; // 22pt at the 14pt title font: from the top of the pane
  Rgba volume_grid_major_color = Rgba::from_u8(190, 198, 208, 148); // colour of the major grid lines on the faces of a 3D plot
  Rgba volume_grid_minor_color = Rgba::from_u8(205, 212, 221, 122); // colour of the minor grid lines on the faces of a 3D plot
  double volume_grid_minor_stroke_pt{0.38}; // minor grid lines on the back walls
  double volume_grid_minor_floor_stroke_pt{0.32}; // width of the minor grid on the floor of a 3D plot
  double volume_grid_major_stroke_pt{0.65}; // width of the major grid on the walls of a 3D plot
  double volume_grid_major_floor_stroke_pt{0.55}; // width of the major grid on the floor of a 3D plot
  double volume_band_stroke_pt{0.6}; // width of the outline of a band in a 3D plot
  double volume_cue_label_offset_x_em{0.6}; // of the cue font
  double volume_cue_label_offset_y_em{0.4}; // height of a cue label above its line in a 3D plot

  // ---- Per-kind marks (pt unless stated; shares are of the quantity named). -----------------------------------------
  // Fieldmap: the value written in a cell, and what is done for a field with no spread.
  double field_label_font_pt{10.0}; // size of the value written in a cell of a labelled heat map
  double field_label_luma_threshold{0.55}; // a lighter cell gets the dark label, a darker one the light label
  Rgba field_label_on_light = Rgba::from_u8(24, 23, 21); // colour of a cell label on a light cell
  Rgba field_label_on_dark = Rgba::from_u8(248, 247, 244); // colour of a cell label on a dark cell
  double degenerate_range_half_width{0.5}; // a range with no spread is widened by this much on each side
  double spike_head_radius_pt{3.0}; // radius of the dot at the end of a stem
  int spike_head_segments{12}; // segments used to draw the dot of a stem
  double pie_radius_share{0.45}; // of the smaller side of the plot area
  double pie_explode_share{0.18}; // of the radius, at an explode of 1
  int pie_min_steps{8}; // polygon steps of a slice: at least this many
  double pie_steps_per_full_turn{48.0}; // and this many for a slice of the whole circle
  double pie_label_radius_factor{1.15}; // slice labels sit this far out, in radii
  double pie_label_font_pt{10.0}; // size of the labels of a pie
  double ribbon_palette_sample{0.58}; // a single-band ribbon
  double columns_palette_sample{0.62}; // position in the palette of the colour of bars
  double bucket_palette_sample{0.55}; // position in the palette of the colour of histogram bars
  double columns_outline_stroke_pt{0.8}; // width of the outline of bars
  double bucket_outline_stroke_pt{0.8}; // width of the outline of histogram bars
  double filled_outline_stroke_pt{0.9}; // ribbon and wedge outlines
  double whisk_centre_radius_pt{2.7}; // radius of the dot at the centre of an error bar
  int whisk_centre_segments{12}; // segments used to draw it
  double quartet_palette_sample{0.42}; // position in the palette of the colour of a box
  double violin_palette_sample{0.42}; // position in the palette of the colour of a violin
  double quartet_whisker_cap_share{0.45}; // of half the box width
  double quartet_mean_radius_pt{2.5}; // radius of the mean marker of a box
  int quartet_mean_segments{10}; // segments used to draw it
  double quartet_outline_shade{0.58}; // how much darker than its fill the outline of a box is
  double quartet_emphasis_shade{0.82}; // the median line of a box, darker than the rest
  double violin_point_radius_pt{1.9}; // radius of the sample points of a violin
  int violin_point_segments{10}; // segments used to draw them
  double violin_point_stroke_pt{0.65}; // width of the outline of the sample points
  std::size_t violin_stick_budget{280}; // readability policy: sticks are thinned to about this many
  double contour_label_min_length_pt{24.0}; // a level is labelled only if its longest segment is this long
  double contour_label_font_pt{10.0}; // size of the level labels of contour lines
  double isoband_outline_stroke_pt{0.18}; // width of the outline of a filled band
  double isoband_outline_shade{0.65}; // how much darker than its fill the outline of a band is

  // ---- 3D surfaces: materials and lighting. ----------------------------------------------------------------------------
  // Coefficients of the Phong-style shading of a surface for each named material profile.
  StyleMaterial material_dull{0.3, 0.8, 0.0, 10.0, 1.0}; // coefficients of the dull material
  StyleMaterial material_shiny{0.3, 0.6, 0.9, 20.0, 1.0}; // coefficients of the shiny material
  StyleMaterial material_metal{0.3, 0.3, 1.0, 25.0, 0.5}; // coefficients of the metal material
  StyleMaterial material_default = material_shiny; // the material a trace has when none is chosen
  double local_light_distance{2.2}; // a Local light: its distance from the scene centre, in scene units
  double surface_min_intensity{0.15}; // guard: a lit surface never goes fully black
  double surface_max_intensity{1.5}; // guard: nor blows out
  double smooth_surface_white_mix{0.08}; // smooth shading lifts the colour toward white by this share
  double transparent_surface_alpha{0.56}; // opacity of a transparent surface
  double facet_outline_shade{0.45}; // faceted surfaces: outline brightness
  double facet_outline_stroke_pt{0.45}; // width of the outline of the facets of a surface
  double volume_contour_label_min_length_pt{28.0}; // shortest contour line that gets a label in a 3D plot
  double volume_contour_label_font_pt{10.0}; // size of the level labels in a 3D plot
  double spire_outline_stroke_pt{0.8}; // width of the outline of 3D bars


  // ---- Axis ranges and tick steps. -----------------------------------------------------------------------------------------
  double limit_pad_share{0.06}; // the Padded limit method widens a range by this share of its span on each side
  double empty_linear_range_half_width{1.0}; // a linear axis with no finite data spans minus to plus this
  double empty_log_range_low{0.1}; // a log axis with no positive data spans from this
  double empty_log_range_high{10.0}; // to this
  double log_floor_share{1.0e-3}; // data that reaches zero or below on a log axis is cut at this share of the largest value
  double log_floor_min{1.0e-6}; // guard: and never below this
  double nice_step_one_below{1.5}; // a tick step is 1, 2, 5 or 10 times a power of ten: up to this mantissa it is 1
  double nice_step_two_below{3.5}; // up to this it is 2
  double nice_step_five_below{7.5}; // up to this it is 5, above it 10
  int minor_tick_limit{96}; // guard: most minor ticks drawn in one decade of a log axis
  int minor_tick_limit_total{128}; // guard: most minor ticks drawn on one axis
  double tick_text_centering_em{0.35}; // a tick label is centred on its tick by moving its baseline down this much of its font

  // ---- 3D camera. -----------------------------------------------------------------------------------------------------------
  double camera_reference_distance{3.2}; // distance of the eye from the scene at the reference field of view, in scene units
  double camera_reference_half_view_deg{15.0}; // half the field of view that distance belongs to; wider angles bring the eye closer
  double camera_min_distance{1.1}; // guard: the eye never gets closer than this
  double camera_up_flip_threshold{0.95}; // looking this close to straight up or down, the view turns about y instead of z

  // ---- Density estimate of a violin (Silverman's rule, as in the usual statistical packages). -----------------------------------
  int violin_resolution{120}; // points the outline of a violin is drawn with
  double violin_bandwidth_factor{0.9}; // bandwidth = this * sigma * n^(-1/5)
  double violin_iqr_to_sigma{1.34}; // the spread of a normal distribution in interquartile ranges
  double violin_fallback_bandwidth_span_divisor{22.0}; // a sample with no spread gets a bandwidth of its range over this
  double violin_min_bandwidth_span_divisor{64.0}; // guard: a bandwidth is never narrower than the range over this

  // ---- Text and mathematics (sizes in em of the font the text sits in). -------------------------------------------------------
  double math_script_scale{0.72}; // size of a superscript or subscript relative to the line
  double math_superscript_rise_em{0.33}; // a superscript is raised by this
  double math_subscript_drop_em{0.20}; // a subscript is lowered by this
  double math_delimiter_shift_share{0.36}; // a stretched delimiter is lowered by this share of how much it grew
  double math_delimiter_scale_plain{1.24}; // a stretched delimiter is this much larger than the text
  double math_delimiter_scale_script{1.40}; // when scripts are near
  double math_delimiter_scale_fraction{1.56}; // when a fraction is near
  double math_numerator_rise_em{0.60}; // a stacked numerator is raised by this
  double math_denominator_rise_em{-0.54}; // a stacked denominator is lowered by this
  double math_fraction_side_padding_em{0.08}; // room kept at each side of a fraction bar
  double math_fraction_min_width_em{0.74}; // narrowest fraction bar
  double math_fraction_bar_offset_em{0.03}; // height of the bar above the baseline
  double math_fraction_bar_width_em{0.055}; // thickness of the bar
  double math_fraction_trailing_gap_em{0.10}; // gap after a fraction
  double fallback_glyph_width_units{600.0}; // width, in thousandths of an em, of a glyph the font tables do not list


  // ---- Further sizes and policies. ------------------------------------------------------------------------------------------
  int tick_count_limit{12}; // guard: automatic ticks of an axis are not generated past this many (a readability cap)
  double harmonize_scientific_above{1.0e4}; // ticks of one axis are written alike; above this magnitude they are left in scientific form
  int minor_tick_divisions{5}; // minor ticks of a linear axis divide each major interval into this many parts
  double default_marker_size{18.0}; // area of a cloud marker when no size is given
  int marker_default_steps{14}; // corners of the polygon a marker is drawn with when the caller does not say
  double quartet_whisker_iqr_factor{1.5}; // a box's whiskers reach the furthest value within this many interquartile ranges of the box
  double quartet_inner_line_share{0.92}; // width of the median line of a box, as a share of the box
  double quartet_box_stroke_pt{0.8}; // width of the outline of a box
  double key_swatch_palette_sample{0.65}; // position in the palette of the sample of a filled trace in a legend
  double key_swatch_outline_shade{0.55}; // how much darker than its fill the outline of that sample is
  double filled_outline_shade{0.55}; // how much darker than its fill the outline of a filled trace is
  int key_preview_max_chars{10}; // characters of a hidden entry's label shown in the "more" line of a long legend
  double key_more_stroke_pt{1.2}; // width of the mark of the "more" line of a long legend
  double key_more_mix{0.25}; // share of the page colour mixed into the mark of the "more" line
  double volume_cue_mark_half_size_pt{2.5}; // half the size of the square mark of a pin on a 3D plot
  double field_cell_stroke_pt{0.4}; // width of the outline of a cell of a heat map
  double default_text_pt{12.0}; // size of text that is given none
  double default_stroke_pt{0.8}; // width of a line that is given none
  double math_fraction_min_bar_pt{0.6}; // guard: the bar of a stacked fraction is never thinner than this

  // ---- 3D axes and tick formatting. ---------------------------------------------------------------------------------------
  double volume_axis_color_share{0.58}; // axis lines: the foreground at this brightness
  double volume_tick_color_share{0.72}; // share of the foreground colour used for the ticks of a 3D plot
  double volume_axis_stroke_pt{1.25}; // width of the axis lines of a 3D plot
  double volume_tick_stroke_pt{1.16}; // width of the tick marks of a 3D plot
  double volume_unparallel_label_below_pt{58.0}; // axis labels kept upright (not parallel to their axis): below the axis midpoint
  double volume_unparallel_y_label_left_pt{24.0}; // distance of an upright y label left of a 3D plot
  double tick_align_middle_threshold{0.35}; // a tick pointing mostly up or down is centred, otherwise left- or right-aligned
  double tick_snap_to_zero{1.0e-12}; // a tick closer to zero than this, relative to the span, is written as 0
  int tick_plain_decimals{6}; // decimals written for plain tick format
  int tick_scientific_decimals{2}; // decimals written for scientific tick format
  double auto_scientific_above{1.0e6}; // automatic ticks above this magnitude are written in scientific notation
  double auto_scientific_below{1.0e-3}; // automatic ticks below this magnitude are written in scientific notation
  int auto_tick_decimals_min{1};  // automatic ticks show at least this many decimals when the step is fractional (readability policy)
  int auto_tick_decimals_max{3};  // and at most this many: finer steps are written in scientific notation instead

  // ---- The y-axis label of a planar pane. ----------------------------------------------------------------------------------
  double uncentered_y_label_clearance_pt{8.0}; // an upright y label above the plot: at least this far
  double uncentered_y_label_clearance_em{0.95}; // or this many ems of the tick font
  double centered_y_label_baseline_em{0.75}; // of the axis-label font: a turned label's offset beyond the tick labels
  double near_vertical_tolerance_deg{20.0}; // a label within this of upright-vertical counts as vertical
  double y_label_fit_margin_pt{10.0}; // kept free along the axis when a vertical label is fitted
  double y_label_fit_floor_pt{12.0}; // guard: the room a vertical label is fitted to is at least this
  double y_label_fit_min_font_pt{7.0}; // readability policy: a long label shrinks no further than this, then ends in "..."
  double key_min_wrap_chars{4.0}; // guard: a key label wraps to at least this many characters per line
  double y_label_min_chars{4.0}; // guard
  double y_label_max_chars{2048.0}; // guard

  // ---- Stacked panes and sub-pane grids. --------------------------------------------------------------------------------
  StyleSpacing stacked_pane_gap{18.0, 12.0}; // between panes stacked without a mosaic
  double subpane_gutter_pt{12.0}; // between the sub-panes of one parent (PanePlacement's default)
  double min_tight_subpane_gutter_pt{4.0}; // guard

};

// The name of the first value that is not a finite number, or null when every value is usable. Studio::style refuses a
// set that has one.
[[nodiscard]] inline const char* first_non_finite_style_value(const StyleOptions& o) noexcept {
  if (!std::isfinite(o.tick_font_pt)) return "tick_font_pt";
  if (!std::isfinite(o.axis_label_font_pt)) return "axis_label_font_pt";
  if (!std::isfinite(o.headline_font_pt)) return "headline_font_pt";
  if (!std::isfinite(o.caption_font_pt)) return "caption_font_pt";
  if (!std::isfinite(o.cue_label_font_pt)) return "cue_label_font_pt";
  if (!std::isfinite(o.tick_length_em)) return "tick_length_em";
  if (!std::isfinite(o.tick_text_gap_em)) return "tick_text_gap_em";
  if (!std::isfinite(o.rotated_tick_text_gap_base_em)) return "rotated_tick_text_gap_base_em";
  if (!std::isfinite(o.rotated_tick_text_gap_gain_em)) return "rotated_tick_text_gap_gain_em";
  if (!std::isfinite(o.bordered_virtual_tick_factor)) return "bordered_virtual_tick_factor";
  if (!std::isfinite(o.borderless_virtual_tick_factor)) return "borderless_virtual_tick_factor";
  if (!std::isfinite(o.y_axis_optical_comp_em)) return "y_axis_optical_comp_em";
  if (!std::isfinite(o.y_axis_optical_comp_rotation_attenuation)) return "y_axis_optical_comp_rotation_attenuation";
  if (!std::isfinite(o.axis_label_gap_x_em)) return "axis_label_gap_x_em";
  if (!std::isfinite(o.axis_label_gap_y_em)) return "axis_label_gap_y_em";
  if (!std::isfinite(o.axis_label_gap_x_no_axes_em)) return "axis_label_gap_x_no_axes_em";
  if (!std::isfinite(o.axis_label_gap_y_no_axes_em)) return "axis_label_gap_y_no_axes_em";
  if (!std::isfinite(o.spatial_tick_length_em)) return "spatial_tick_length_em";
  if (!std::isfinite(o.spatial_z_tick_length_em)) return "spatial_z_tick_length_em";
  if (!std::isfinite(o.spatial_tick_text_gap_em)) return "spatial_tick_text_gap_em";
  if (!std::isfinite(o.spatial_axis_label_base_gap_em)) return "spatial_axis_label_base_gap_em";
  if (!std::isfinite(o.text_ascent_em)) return "text_ascent_em";
  if (!std::isfinite(o.text_descent_em)) return "text_descent_em";
  if (!std::isfinite(o.estimated_glyph_width_em)) return "estimated_glyph_width_em";
  if (!std::isfinite(o.tick_text_clearance_em)) return "tick_text_clearance_em";
  if (!std::isfinite(o.axis_label_baseline_em)) return "axis_label_baseline_em";
  if (!std::isfinite(o.headline_line_height_em)) return "headline_line_height_em";
  if (!std::isfinite(o.headline_side_margin_em)) return "headline_side_margin_em";
  if (!std::isfinite(o.headline_first_baseline_em)) return "headline_first_baseline_em";
  if (!std::isfinite(o.headline_min_wrap_width_em)) return "headline_min_wrap_width_em";
  if (!std::isfinite(o.caption_below_plot_em)) return "caption_below_plot_em";
  if (!std::isfinite(o.caption_left_of_ticks_em)) return "caption_left_of_ticks_em";
  if (!std::isfinite(o.caption_above_left_x_em)) return "caption_above_left_x_em";
  if (!std::isfinite(o.caption_above_left_y_em)) return "caption_above_left_y_em";
  if (!std::isfinite(o.cue_label_from_edge_em)) return "cue_label_from_edge_em";
  if (!std::isfinite(o.cue_label_beside_line_em)) return "cue_label_beside_line_em";
  if (!std::isfinite(o.cue_label_inset_em)) return "cue_label_inset_em";
  if (!std::isfinite(o.pin_mark_em)) return "pin_mark_em";
  if (!std::isfinite(o.pin_text_offset_x_em)) return "pin_text_offset_x_em";
  if (!std::isfinite(o.pin_text_offset_y_em)) return "pin_text_offset_y_em";
  if (!std::isfinite(o.cue_band_stroke_pt)) return "cue_band_stroke_pt";
  if (!std::isfinite(o.pin_leader_stroke_pt)) return "pin_leader_stroke_pt";
  if (!std::isfinite(o.pin_mark_stroke_pt)) return "pin_mark_stroke_pt";
  if (!std::isfinite(o.marker_radius_per_sqrt_size)) return "marker_radius_per_sqrt_size";
  if (!std::isfinite(o.marker3d_radius_per_sqrt_size)) return "marker3d_radius_per_sqrt_size";
  if (!std::isfinite(o.min_font_pt)) return "min_font_pt";
  if (!std::isfinite(o.min_marker_radius_pt)) return "min_marker_radius_pt";
  if (!std::isfinite(o.min_marker3d_radius_pt)) return "min_marker3d_radius_pt";
  if (!std::isfinite(o.pane_top_inset.relaxed) || !std::isfinite(o.pane_top_inset.compact)) return "pane_top_inset";
  if (!std::isfinite(o.pane_left_inset.relaxed) || !std::isfinite(o.pane_left_inset.compact)) return "pane_left_inset";
  if (!std::isfinite(o.pane_right_inset.relaxed) || !std::isfinite(o.pane_right_inset.compact)) return "pane_right_inset";
  if (!std::isfinite(o.pane_bottom_inset.relaxed) || !std::isfinite(o.pane_bottom_inset.compact)) return "pane_bottom_inset";
  if (!std::isfinite(o.pane_outside_legend_right_inset.relaxed) || !std::isfinite(o.pane_outside_legend_right_inset.compact)) return "pane_outside_legend_right_inset";
  if (!std::isfinite(o.bare_left_inset.relaxed) || !std::isfinite(o.bare_left_inset.compact)) return "bare_left_inset";
  if (!std::isfinite(o.bare_bottom_inset.relaxed) || !std::isfinite(o.bare_bottom_inset.compact)) return "bare_bottom_inset";
  if (!std::isfinite(o.bare_right_inset.relaxed) || !std::isfinite(o.bare_right_inset.compact)) return "bare_right_inset";
  if (!std::isfinite(o.empty_axis_label_pad)) return "empty_axis_label_pad";
  if (!std::isfinite(o.reference_figure_width_pt)) return "reference_figure_width_pt";
  if (!std::isfinite(o.reference_figure_height_pt)) return "reference_figure_height_pt";
  if (!std::isfinite(o.figure_edge_share_x.relaxed) || !std::isfinite(o.figure_edge_share_x.compact)) return "figure_edge_share_x";
  if (!std::isfinite(o.figure_edge_share_y.relaxed) || !std::isfinite(o.figure_edge_share_y.compact)) return "figure_edge_share_y";
  if (!std::isfinite(o.figure_caption_column_em.relaxed) || !std::isfinite(o.figure_caption_column_em.compact)) return "figure_caption_column_em";
  if (!std::isfinite(o.figure_caption_row_em.relaxed) || !std::isfinite(o.figure_caption_row_em.compact)) return "figure_caption_row_em";
  if (!std::isfinite(o.figure_headline_clearance_em.relaxed) || !std::isfinite(o.figure_headline_clearance_em.compact)) return "figure_headline_clearance_em";
  if (!std::isfinite(o.mosaic_gutter_share)) return "mosaic_gutter_share";
  if (!std::isfinite(o.min_tight_gutter_pt)) return "min_tight_gutter_pt";
  if (!std::isfinite(o.tight_gutter_factor)) return "tight_gutter_factor";
  if (!std::isfinite(o.volume_near_pad.relaxed) || !std::isfinite(o.volume_near_pad.compact)) return "volume_near_pad";
  if (!std::isfinite(o.volume_far_pad.relaxed) || !std::isfinite(o.volume_far_pad.compact)) return "volume_far_pad";
  if (!std::isfinite(o.volume_left_label_share)) return "volume_left_label_share";
  if (!std::isfinite(o.volume_right_label_share)) return "volume_right_label_share";
  if (!std::isfinite(o.volume_bottom_tick_share)) return "volume_bottom_tick_share";
  if (!std::isfinite(o.volume_bottom_label_share)) return "volume_bottom_label_share";
  if (!std::isfinite(o.volume_top_tick_share)) return "volume_top_tick_share";
  if (!std::isfinite(o.volume_title_em)) return "volume_title_em";
  if (!std::isfinite(o.key_font_pt)) return "key_font_pt";
  if (!std::isfinite(o.key_pad_top_pt)) return "key_pad_top_pt";
  if (!std::isfinite(o.key_pad_bottom_pt)) return "key_pad_bottom_pt";
  if (!std::isfinite(o.key_pad_left_pt)) return "key_pad_left_pt";
  if (!std::isfinite(o.key_pad_right_pt)) return "key_pad_right_pt";
  if (!std::isfinite(o.key_icon_span_pt)) return "key_icon_span_pt";
  if (!std::isfinite(o.key_label_gap_pt)) return "key_label_gap_pt";
  if (!std::isfinite(o.key_title_height_pt)) return "key_title_height_pt";
  if (!std::isfinite(o.key_title_gap_pt)) return "key_title_gap_pt";
  if (!std::isfinite(o.key_line_height_pt)) return "key_line_height_pt";
  if (!std::isfinite(o.key_row_pad_pt)) return "key_row_pad_pt";
  if (!std::isfinite(o.key_row_min_height_pt)) return "key_row_min_height_pt";
  if (!std::isfinite(o.key_column_gap_pt)) return "key_column_gap_pt";
  if (!std::isfinite(o.key_min_label_width_pt)) return "key_min_label_width_pt";
  if (!std::isfinite(o.key_title_baseline_pt)) return "key_title_baseline_pt";
  if (!std::isfinite(o.key_first_baseline_pt)) return "key_first_baseline_pt";
  if (!std::isfinite(o.key_swatch_half_height_pt)) return "key_swatch_half_height_pt";
  if (!std::isfinite(o.key_swatch_stroke_pt)) return "key_swatch_stroke_pt";
  if (!std::isfinite(o.key_framed_stroke_pt)) return "key_framed_stroke_pt";
  if (!std::isfinite(o.key_soft_stroke_pt)) return "key_soft_stroke_pt";
  if (!std::isfinite(o.key_framed_stroke_alpha)) return "key_framed_stroke_alpha";
  if (!std::isfinite(o.key_soft_fill_alpha)) return "key_soft_fill_alpha";
  if (!std::isfinite(o.key_soft_stroke_alpha)) return "key_soft_stroke_alpha";
  if (!std::isfinite(o.key_soft_fill_white_mix)) return "key_soft_fill_white_mix";
  if (!std::isfinite(o.key_soft_stroke_background_mix)) return "key_soft_stroke_background_mix";
  if (!std::isfinite(o.key_inset_pt)) return "key_inset_pt";
  if (!std::isfinite(o.key_outside_gap_pt)) return "key_outside_gap_pt";
  if (!std::isfinite(o.key_min_width_pt)) return "key_min_width_pt";
  if (!std::isfinite(o.key_min_width_outside_pt)) return "key_min_width_outside_pt";
  if (!std::isfinite(o.key_max_width_outside_pt)) return "key_max_width_outside_pt";
  if (!std::isfinite(o.key_max_width_plot_share)) return "key_max_width_plot_share";
  if (!std::isfinite(o.key_max_width_floor_pt)) return "key_max_width_floor_pt";
  if (!std::isfinite(o.key_max_height_floor_pt)) return "key_max_height_floor_pt";
  if (!std::isfinite(o.legend_overlap_sample_step_pt)) return "legend_overlap_sample_step_pt";
  if (!std::isfinite(o.key_max_height_margin_pt)) return "key_max_height_margin_pt";
  if (!std::isfinite(o.key_target_ratio_horizontal)) return "key_target_ratio_horizontal";
  if (!std::isfinite(o.key_target_ratio_vertical)) return "key_target_ratio_vertical";
  if (!std::isfinite(o.key_target_ratio_auto)) return "key_target_ratio_auto";
  if (!std::isfinite(o.key_truncation_score)) return "key_truncation_score";
  if (!std::isfinite(o.key_overflow_base_score)) return "key_overflow_base_score";
  if (!std::isfinite(o.key_overflow_gain_score)) return "key_overflow_gain_score";
  if (!std::isfinite(o.key_area_weight)) return "key_area_weight";
  if (!std::isfinite(o.dock_min_width_east_pt)) return "dock_min_width_east_pt";
  if (!std::isfinite(o.dock_min_width_south_pt)) return "dock_min_width_south_pt";
  if (!std::isfinite(o.dock_east_width_share)) return "dock_east_width_share";
  if (!std::isfinite(o.dock_east_width_min_pt)) return "dock_east_width_min_pt";
  if (!std::isfinite(o.dock_east_height_floor_pt)) return "dock_east_height_floor_pt";
  if (!std::isfinite(o.dock_east_height_margin_pt)) return "dock_east_height_margin_pt";
  if (!std::isfinite(o.dock_east_height_margin_with_headline_pt)) return "dock_east_height_margin_with_headline_pt";
  if (!std::isfinite(o.dock_east_right_inset_pt)) return "dock_east_right_inset_pt";
  if (!std::isfinite(o.dock_east_plot_gap_pt)) return "dock_east_plot_gap_pt";
  if (!std::isfinite(o.dock_south_plot_gap_pt)) return "dock_south_plot_gap_pt";
  if (!std::isfinite(o.dock_east_top_pt)) return "dock_east_top_pt";
  if (!std::isfinite(o.dock_east_top_with_headline_pt)) return "dock_east_top_with_headline_pt";
  if (!std::isfinite(o.dock_bottom_inset_pt)) return "dock_bottom_inset_pt";
  if (!std::isfinite(o.dock_south_side_margin_pt)) return "dock_south_side_margin_pt";
  if (!std::isfinite(o.dock_south_height_share)) return "dock_south_height_share";
  if (!std::isfinite(o.dock_south_height_floor_pt)) return "dock_south_height_floor_pt";
  if (!std::isfinite(o.dock_south_bottom_pt)) return "dock_south_bottom_pt";
  if (!std::isfinite(o.dock_south_side_inset_pt)) return "dock_south_side_inset_pt";
  if (!std::isfinite(o.grid_minor_stroke_pt)) return "grid_minor_stroke_pt";
  if (!std::isfinite(o.grid_major_stroke_pt)) return "grid_major_stroke_pt";
  if (!std::isfinite(o.axis_line_stroke_pt)) return "axis_line_stroke_pt";
  if (!std::isfinite(o.axis_close_stroke_pt)) return "axis_close_stroke_pt";
  if (!std::isfinite(o.tick_stroke_pt)) return "tick_stroke_pt";
  if (!std::isfinite(o.tick_color_foreground_share)) return "tick_color_foreground_share";
  if (!std::isfinite(o.axis_color_foreground_share)) return "axis_color_foreground_share";
  if (!std::isfinite(o.dark_background_luma)) return "dark_background_luma";
  if (!std::isfinite(o.luma_red)) return "luma_red";
  if (!std::isfinite(o.luma_green)) return "luma_green";
  if (!std::isfinite(o.luma_blue)) return "luma_blue";
  if (!std::isfinite(o.polar_outer_ring_stroke_pt)) return "polar_outer_ring_stroke_pt";
  if (!std::isfinite(o.polar_ring_stroke_pt)) return "polar_ring_stroke_pt";
  if (!std::isfinite(o.polar_spoke_stroke_pt)) return "polar_spoke_stroke_pt";
  if (!std::isfinite(o.polar_ring_color.red) || !std::isfinite(o.polar_ring_color.green) || !std::isfinite(o.polar_ring_color.blue) || !std::isfinite(o.polar_ring_color.alpha)) return "polar_ring_color";
  if (!std::isfinite(o.polar_spoke_color.red) || !std::isfinite(o.polar_spoke_color.green) || !std::isfinite(o.polar_spoke_color.blue) || !std::isfinite(o.polar_spoke_color.alpha)) return "polar_spoke_color";
  if (!std::isfinite(o.plot_face_dark_grid.red) || !std::isfinite(o.plot_face_dark_grid.green) || !std::isfinite(o.plot_face_dark_grid.blue) || !std::isfinite(o.plot_face_dark_grid.alpha)) return "plot_face_dark_grid";
  if (!std::isfinite(o.plot_face_sand.red) || !std::isfinite(o.plot_face_sand.green) || !std::isfinite(o.plot_face_sand.blue) || !std::isfinite(o.plot_face_sand.alpha)) return "plot_face_sand";
  if (!std::isfinite(o.plot_face_styled.red) || !std::isfinite(o.plot_face_styled.green) || !std::isfinite(o.plot_face_styled.blue) || !std::isfinite(o.plot_face_styled.alpha)) return "plot_face_styled";
  if (!std::isfinite(o.minor_grid_dark_grid.red) || !std::isfinite(o.minor_grid_dark_grid.green) || !std::isfinite(o.minor_grid_dark_grid.blue) || !std::isfinite(o.minor_grid_dark_grid.alpha)) return "minor_grid_dark_grid";
  if (!std::isfinite(o.minor_grid_sand.red) || !std::isfinite(o.minor_grid_sand.green) || !std::isfinite(o.minor_grid_sand.blue) || !std::isfinite(o.minor_grid_sand.alpha)) return "minor_grid_sand";
  if (!std::isfinite(o.minor_grid_dark_background.red) || !std::isfinite(o.minor_grid_dark_background.green) || !std::isfinite(o.minor_grid_dark_background.blue) || !std::isfinite(o.minor_grid_dark_background.alpha)) return "minor_grid_dark_background";
  if (!std::isfinite(o.minor_grid_light.red) || !std::isfinite(o.minor_grid_light.green) || !std::isfinite(o.minor_grid_light.blue) || !std::isfinite(o.minor_grid_light.alpha)) return "minor_grid_light";
  if (!std::isfinite(o.major_grid_dark_grid.red) || !std::isfinite(o.major_grid_dark_grid.green) || !std::isfinite(o.major_grid_dark_grid.blue) || !std::isfinite(o.major_grid_dark_grid.alpha)) return "major_grid_dark_grid";
  if (!std::isfinite(o.major_grid_sand.red) || !std::isfinite(o.major_grid_sand.green) || !std::isfinite(o.major_grid_sand.blue) || !std::isfinite(o.major_grid_sand.alpha)) return "major_grid_sand";
  if (!std::isfinite(o.major_grid_dark_background.red) || !std::isfinite(o.major_grid_dark_background.green) || !std::isfinite(o.major_grid_dark_background.blue) || !std::isfinite(o.major_grid_dark_background.alpha)) return "major_grid_dark_background";
  if (!std::isfinite(o.major_grid_light.red) || !std::isfinite(o.major_grid_light.green) || !std::isfinite(o.major_grid_light.blue) || !std::isfinite(o.major_grid_light.alpha)) return "major_grid_light";
  if (!std::isfinite(o.trace_line_stroke_pt)) return "trace_line_stroke_pt";
  if (!std::isfinite(o.trace_thin_line_stroke_pt)) return "trace_thin_line_stroke_pt";
  if (!std::isfinite(o.trace_fine_line_stroke_pt)) return "trace_fine_line_stroke_pt";
  if (!std::isfinite(o.trace_marker_outline_stroke_pt)) return "trace_marker_outline_stroke_pt";
  if (!std::isfinite(o.trace_box_min_median_stroke_pt)) return "trace_box_min_median_stroke_pt";
  if (!std::isfinite(o.trace_quartet_min_median_stroke_pt)) return "trace_quartet_min_median_stroke_pt";
  if (!std::isfinite(o.error_cap_half_width_pt)) return "error_cap_half_width_pt";
  if (!std::isfinite(o.marker_fill_alpha)) return "marker_fill_alpha";
  if (!std::isfinite(o.marker_min_fill_alpha)) return "marker_min_fill_alpha";
  if (!std::isfinite(o.marker_steps_per_radius)) return "marker_steps_per_radius";
  if (!std::isfinite(o.outline_shade)) return "outline_shade";
  if (!std::isfinite(o.scale_guide_bar_gap_axes_pt)) return "scale_guide_bar_gap_axes_pt";
  if (!std::isfinite(o.scale_guide_bar_gap_bare_pt)) return "scale_guide_bar_gap_bare_pt";
  if (!std::isfinite(o.scale_guide_bar_width_pt)) return "scale_guide_bar_width_pt";
  if (!std::isfinite(o.scale_guide_tick_length_pt)) return "scale_guide_tick_length_pt";
  if (!std::isfinite(o.scale_guide_tick_label_pad_axes_pt)) return "scale_guide_tick_label_pad_axes_pt";
  if (!std::isfinite(o.scale_guide_tick_label_pad_bare_pt)) return "scale_guide_tick_label_pad_bare_pt";
  if (!std::isfinite(o.scale_guide_label_allowance_axes.relaxed) || !std::isfinite(o.scale_guide_label_allowance_axes.compact)) return "scale_guide_label_allowance_axes";
  if (!std::isfinite(o.scale_guide_label_allowance_bare.relaxed) || !std::isfinite(o.scale_guide_label_allowance_bare.compact)) return "scale_guide_label_allowance_bare";
  if (!std::isfinite(o.scale_guide_stroke_pt)) return "scale_guide_stroke_pt";
  if (!std::isfinite(o.scale_guide_font_pt)) return "scale_guide_font_pt";
  if (!std::isfinite(o.scale_guide_label_x_pt)) return "scale_guide_label_x_pt";
  if (!std::isfinite(o.scale_guide_label_baseline_pt)) return "scale_guide_label_baseline_pt";
  if (!std::isfinite(o.pane_caption_font_pt)) return "pane_caption_font_pt";
  if (!std::isfinite(o.pane_caption_color_share)) return "pane_caption_color_share";
  if (!std::isfinite(o.pane_caption_inset_x_pt)) return "pane_caption_inset_x_pt";
  if (!std::isfinite(o.pane_caption_from_bottom_pt)) return "pane_caption_from_bottom_pt";
  if (!std::isfinite(o.pane_caption_line_gap_pt)) return "pane_caption_line_gap_pt";
  if (!std::isfinite(o.pane_caption_block_inset_x_pt)) return "pane_caption_block_inset_x_pt";
  if (!std::isfinite(o.pane_caption_block_top_pt)) return "pane_caption_block_top_pt";
  if (!std::isfinite(o.pane_caption_block_bottom_margin_pt)) return "pane_caption_block_bottom_margin_pt";
  if (!std::isfinite(o.row_title_font_pt)) return "row_title_font_pt";
  if (!std::isfinite(o.row_title_below_edge_pt)) return "row_title_below_edge_pt";
  if (!std::isfinite(o.row_title_below_headline_pt)) return "row_title_below_headline_pt";
  if (!std::isfinite(o.row_title_below_headline_font_share)) return "row_title_below_headline_font_share";
  if (!std::isfinite(o.row_title_above_row_em)) return "row_title_above_row_em";
  if (!std::isfinite(o.row_title_above_row_pt)) return "row_title_above_row_pt";
  if (!std::isfinite(o.pane_title_font_pt)) return "pane_title_font_pt";
  if (!std::isfinite(o.pane_title_baseline_em)) return "pane_title_baseline_em";
  if (!std::isfinite(o.volume_grid_major_color.red) || !std::isfinite(o.volume_grid_major_color.green) || !std::isfinite(o.volume_grid_major_color.blue) || !std::isfinite(o.volume_grid_major_color.alpha)) return "volume_grid_major_color";
  if (!std::isfinite(o.volume_grid_minor_color.red) || !std::isfinite(o.volume_grid_minor_color.green) || !std::isfinite(o.volume_grid_minor_color.blue) || !std::isfinite(o.volume_grid_minor_color.alpha)) return "volume_grid_minor_color";
  if (!std::isfinite(o.volume_grid_minor_stroke_pt)) return "volume_grid_minor_stroke_pt";
  if (!std::isfinite(o.volume_grid_minor_floor_stroke_pt)) return "volume_grid_minor_floor_stroke_pt";
  if (!std::isfinite(o.volume_grid_major_stroke_pt)) return "volume_grid_major_stroke_pt";
  if (!std::isfinite(o.volume_grid_major_floor_stroke_pt)) return "volume_grid_major_floor_stroke_pt";
  if (!std::isfinite(o.volume_band_stroke_pt)) return "volume_band_stroke_pt";
  if (!std::isfinite(o.volume_cue_label_offset_x_em)) return "volume_cue_label_offset_x_em";
  if (!std::isfinite(o.volume_cue_label_offset_y_em)) return "volume_cue_label_offset_y_em";
  if (!std::isfinite(o.field_label_font_pt)) return "field_label_font_pt";
  if (!std::isfinite(o.field_label_luma_threshold)) return "field_label_luma_threshold";
  if (!std::isfinite(o.field_label_on_light.red) || !std::isfinite(o.field_label_on_light.green) || !std::isfinite(o.field_label_on_light.blue) || !std::isfinite(o.field_label_on_light.alpha)) return "field_label_on_light";
  if (!std::isfinite(o.field_label_on_dark.red) || !std::isfinite(o.field_label_on_dark.green) || !std::isfinite(o.field_label_on_dark.blue) || !std::isfinite(o.field_label_on_dark.alpha)) return "field_label_on_dark";
  if (!std::isfinite(o.degenerate_range_half_width)) return "degenerate_range_half_width";
  if (!std::isfinite(o.spike_head_radius_pt)) return "spike_head_radius_pt";
  if (!std::isfinite(o.pie_radius_share)) return "pie_radius_share";
  if (!std::isfinite(o.pie_explode_share)) return "pie_explode_share";
  if (!std::isfinite(o.pie_steps_per_full_turn)) return "pie_steps_per_full_turn";
  if (!std::isfinite(o.pie_label_radius_factor)) return "pie_label_radius_factor";
  if (!std::isfinite(o.pie_label_font_pt)) return "pie_label_font_pt";
  if (!std::isfinite(o.ribbon_palette_sample)) return "ribbon_palette_sample";
  if (!std::isfinite(o.columns_palette_sample)) return "columns_palette_sample";
  if (!std::isfinite(o.bucket_palette_sample)) return "bucket_palette_sample";
  if (!std::isfinite(o.columns_outline_stroke_pt)) return "columns_outline_stroke_pt";
  if (!std::isfinite(o.bucket_outline_stroke_pt)) return "bucket_outline_stroke_pt";
  if (!std::isfinite(o.filled_outline_stroke_pt)) return "filled_outline_stroke_pt";
  if (!std::isfinite(o.whisk_centre_radius_pt)) return "whisk_centre_radius_pt";
  if (!std::isfinite(o.quartet_palette_sample)) return "quartet_palette_sample";
  if (!std::isfinite(o.violin_palette_sample)) return "violin_palette_sample";
  if (!std::isfinite(o.quartet_whisker_cap_share)) return "quartet_whisker_cap_share";
  if (!std::isfinite(o.quartet_mean_radius_pt)) return "quartet_mean_radius_pt";
  if (!std::isfinite(o.quartet_outline_shade)) return "quartet_outline_shade";
  if (!std::isfinite(o.quartet_emphasis_shade)) return "quartet_emphasis_shade";
  if (!std::isfinite(o.violin_point_radius_pt)) return "violin_point_radius_pt";
  if (!std::isfinite(o.violin_point_stroke_pt)) return "violin_point_stroke_pt";
  if (!std::isfinite(o.contour_label_min_length_pt)) return "contour_label_min_length_pt";
  if (!std::isfinite(o.contour_label_font_pt)) return "contour_label_font_pt";
  if (!std::isfinite(o.isoband_outline_stroke_pt)) return "isoband_outline_stroke_pt";
  if (!std::isfinite(o.isoband_outline_shade)) return "isoband_outline_shade";
  if (!std::isfinite(o.material_dull.ambient) || !std::isfinite(o.material_dull.diffuse) || !std::isfinite(o.material_dull.specular) || !std::isfinite(o.material_dull.exponent) || !std::isfinite(o.material_dull.specular_color)) return "material_dull";
  if (!std::isfinite(o.material_shiny.ambient) || !std::isfinite(o.material_shiny.diffuse) || !std::isfinite(o.material_shiny.specular) || !std::isfinite(o.material_shiny.exponent) || !std::isfinite(o.material_shiny.specular_color)) return "material_shiny";
  if (!std::isfinite(o.material_metal.ambient) || !std::isfinite(o.material_metal.diffuse) || !std::isfinite(o.material_metal.specular) || !std::isfinite(o.material_metal.exponent) || !std::isfinite(o.material_metal.specular_color)) return "material_metal";
  if (!std::isfinite(o.material_default.ambient) || !std::isfinite(o.material_default.diffuse) || !std::isfinite(o.material_default.specular) || !std::isfinite(o.material_default.exponent) || !std::isfinite(o.material_default.specular_color)) return "material_default";
  if (!std::isfinite(o.local_light_distance)) return "local_light_distance";
  if (!std::isfinite(o.surface_min_intensity)) return "surface_min_intensity";
  if (!std::isfinite(o.surface_max_intensity)) return "surface_max_intensity";
  if (!std::isfinite(o.smooth_surface_white_mix)) return "smooth_surface_white_mix";
  if (!std::isfinite(o.transparent_surface_alpha)) return "transparent_surface_alpha";
  if (!std::isfinite(o.facet_outline_shade)) return "facet_outline_shade";
  if (!std::isfinite(o.facet_outline_stroke_pt)) return "facet_outline_stroke_pt";
  if (!std::isfinite(o.volume_contour_label_min_length_pt)) return "volume_contour_label_min_length_pt";
  if (!std::isfinite(o.volume_contour_label_font_pt)) return "volume_contour_label_font_pt";
  if (!std::isfinite(o.spire_outline_stroke_pt)) return "spire_outline_stroke_pt";
  if (!std::isfinite(o.volume_axis_color_share)) return "volume_axis_color_share";
  if (!std::isfinite(o.volume_tick_color_share)) return "volume_tick_color_share";
  if (!std::isfinite(o.volume_axis_stroke_pt)) return "volume_axis_stroke_pt";
  if (!std::isfinite(o.volume_tick_stroke_pt)) return "volume_tick_stroke_pt";
  if (!std::isfinite(o.volume_unparallel_label_below_pt)) return "volume_unparallel_label_below_pt";
  if (!std::isfinite(o.volume_unparallel_y_label_left_pt)) return "volume_unparallel_y_label_left_pt";
  if (!std::isfinite(o.tick_align_middle_threshold)) return "tick_align_middle_threshold";
  if (!std::isfinite(o.tick_snap_to_zero)) return "tick_snap_to_zero";
  if (!std::isfinite(o.auto_scientific_above)) return "auto_scientific_above";
  if (!std::isfinite(o.auto_scientific_below)) return "auto_scientific_below";
  if (!std::isfinite(o.uncentered_y_label_clearance_pt)) return "uncentered_y_label_clearance_pt";
  if (!std::isfinite(o.uncentered_y_label_clearance_em)) return "uncentered_y_label_clearance_em";
  if (!std::isfinite(o.centered_y_label_baseline_em)) return "centered_y_label_baseline_em";
  if (!std::isfinite(o.near_vertical_tolerance_deg)) return "near_vertical_tolerance_deg";
  if (!std::isfinite(o.y_label_fit_margin_pt)) return "y_label_fit_margin_pt";
  if (!std::isfinite(o.y_label_fit_floor_pt)) return "y_label_fit_floor_pt";
  if (!std::isfinite(o.y_label_fit_min_font_pt)) return "y_label_fit_min_font_pt";
  if (!std::isfinite(o.key_min_wrap_chars)) return "key_min_wrap_chars";
  if (!std::isfinite(o.y_label_min_chars)) return "y_label_min_chars";
  if (!std::isfinite(o.y_label_max_chars)) return "y_label_max_chars";
  if (!std::isfinite(o.stacked_pane_gap.relaxed) || !std::isfinite(o.stacked_pane_gap.compact)) return "stacked_pane_gap";
  if (!std::isfinite(o.subpane_gutter_pt)) return "subpane_gutter_pt";
  if (!std::isfinite(o.min_tight_subpane_gutter_pt)) return "min_tight_subpane_gutter_pt";
  if (!std::isfinite(o.limit_pad_share)) return "limit_pad_share";
  if (!std::isfinite(o.empty_linear_range_half_width)) return "empty_linear_range_half_width";
  if (!std::isfinite(o.empty_log_range_low)) return "empty_log_range_low";
  if (!std::isfinite(o.empty_log_range_high)) return "empty_log_range_high";
  if (!std::isfinite(o.log_floor_share)) return "log_floor_share";
  if (!std::isfinite(o.log_floor_min)) return "log_floor_min";
  if (!std::isfinite(o.nice_step_one_below)) return "nice_step_one_below";
  if (!std::isfinite(o.nice_step_two_below)) return "nice_step_two_below";
  if (!std::isfinite(o.nice_step_five_below)) return "nice_step_five_below";
  if (!std::isfinite(o.tick_text_centering_em)) return "tick_text_centering_em";
  if (!std::isfinite(o.camera_reference_distance)) return "camera_reference_distance";
  if (!std::isfinite(o.camera_reference_half_view_deg)) return "camera_reference_half_view_deg";
  if (!std::isfinite(o.camera_min_distance)) return "camera_min_distance";
  if (!std::isfinite(o.camera_up_flip_threshold)) return "camera_up_flip_threshold";
  if (!std::isfinite(o.violin_bandwidth_factor)) return "violin_bandwidth_factor";
  if (!std::isfinite(o.violin_iqr_to_sigma)) return "violin_iqr_to_sigma";
  if (!std::isfinite(o.violin_fallback_bandwidth_span_divisor)) return "violin_fallback_bandwidth_span_divisor";
  if (!std::isfinite(o.violin_min_bandwidth_span_divisor)) return "violin_min_bandwidth_span_divisor";
  if (!std::isfinite(o.math_script_scale)) return "math_script_scale";
  if (!std::isfinite(o.math_superscript_rise_em)) return "math_superscript_rise_em";
  if (!std::isfinite(o.math_subscript_drop_em)) return "math_subscript_drop_em";
  if (!std::isfinite(o.math_delimiter_shift_share)) return "math_delimiter_shift_share";
  if (!std::isfinite(o.math_delimiter_scale_plain)) return "math_delimiter_scale_plain";
  if (!std::isfinite(o.math_delimiter_scale_script)) return "math_delimiter_scale_script";
  if (!std::isfinite(o.math_delimiter_scale_fraction)) return "math_delimiter_scale_fraction";
  if (!std::isfinite(o.math_numerator_rise_em)) return "math_numerator_rise_em";
  if (!std::isfinite(o.math_denominator_rise_em)) return "math_denominator_rise_em";
  if (!std::isfinite(o.math_fraction_side_padding_em)) return "math_fraction_side_padding_em";
  if (!std::isfinite(o.math_fraction_min_width_em)) return "math_fraction_min_width_em";
  if (!std::isfinite(o.math_fraction_bar_offset_em)) return "math_fraction_bar_offset_em";
  if (!std::isfinite(o.math_fraction_bar_width_em)) return "math_fraction_bar_width_em";
  if (!std::isfinite(o.math_fraction_trailing_gap_em)) return "math_fraction_trailing_gap_em";
  if (!std::isfinite(o.fallback_glyph_width_units)) return "fallback_glyph_width_units";
  if (!std::isfinite(o.harmonize_scientific_above)) return "harmonize_scientific_above";
  if (!std::isfinite(o.default_marker_size)) return "default_marker_size";
  if (!std::isfinite(o.quartet_whisker_iqr_factor)) return "quartet_whisker_iqr_factor";
  if (!std::isfinite(o.quartet_inner_line_share)) return "quartet_inner_line_share";
  if (!std::isfinite(o.quartet_box_stroke_pt)) return "quartet_box_stroke_pt";
  if (!std::isfinite(o.key_swatch_palette_sample)) return "key_swatch_palette_sample";
  if (!std::isfinite(o.key_swatch_outline_shade)) return "key_swatch_outline_shade";
  if (!std::isfinite(o.filled_outline_shade)) return "filled_outline_shade";
  if (!std::isfinite(o.key_more_stroke_pt)) return "key_more_stroke_pt";
  if (!std::isfinite(o.key_more_mix)) return "key_more_mix";
  if (!std::isfinite(o.volume_cue_mark_half_size_pt)) return "volume_cue_mark_half_size_pt";
  if (!std::isfinite(o.field_cell_stroke_pt)) return "field_cell_stroke_pt";
  if (!std::isfinite(o.default_text_pt)) return "default_text_pt";
  if (!std::isfinite(o.default_stroke_pt)) return "default_stroke_pt";
  if (!std::isfinite(o.math_fraction_min_bar_pt)) return "math_fraction_min_bar_pt";
  return nullptr;
}

}  // namespace pluss::pl
