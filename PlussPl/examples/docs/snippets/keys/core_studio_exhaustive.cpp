    Pane defaults(PaneKind::Planar, Theme::paper(), "defaults");
    defaults.grid(GridStyle::Major)
        .x_major_grid(true)
        .y_major_grid(true)
        .tick_font_scale(1.1)
        .axis_label_font_scale(1.1);

    Studio fig("core_studio_exhaustive", Theme::paper());
    fig.rename("core_studio_full")
        .headline("Studio exhaustive controls")
        .theme(themes::ocean())
        .context_scales(1.12, 1.16)
        .size(1280.0, 860.0)
        .mosaic(2, 2, 14.0, 14.0)
        .legend(KeyDock::East)
        .legend_frame(KeyFrame::Framed)
        .legend_shape(KeyShape::Vertical)
        .x_caption("xy_shared", "Shared X")
        .y_caption("xy_shared", "Shared Y")
        .z_caption("z_shared", "Shared Z")
        .row_title(0, "Top Row")
        .row_title(1, "Bottom Row")
        .clear_row_title(1)
        .row_title(1, "Bottom Row")
        .pane_style_defaults(defaults);

    const Theme paper_theme = Theme::paper();
    const Theme ocean_theme = Theme::ocean();
    const auto& paper_name = paper_theme.name();
    const auto& paper_accents = paper_theme.accents();
    const auto& paper_bg = paper_theme.background();
    const auto& paper_fg = paper_theme.foreground();
    const auto& paper_surface = paper_theme.surface_palette();
    const auto paper_accent0 = paper_theme.accent(0);
    const auto& ocean_name = ocean_theme.name();

    const PaletteRamp ramp_neutral = PaletteRamp::neutral();
    const PaletteRamp ramp_thermal = PaletteRamp::thermal();
    const PaletteRamp ramp_lagoon = PaletteRamp::lagoon();
    const PaletteRamp ramp_ocean = PaletteRamp::ocean();
    const PaletteRamp ramp_meadow = PaletteRamp::meadow();
    const auto& ramp_name = ramp_ocean.name();
    const auto& ramp_stops = ramp_ocean.stops();
    const bool ramp_empty = ramp_ocean.empty();
    const auto ramp_sample_mid = ramp_ocean.sample(0.5);

    Lens lens = Lens::spatial();
    lens.look(-45.0, 26.0)
        .spin(6.0, -2.0)
        .target_at(Vec3d{0.2, -0.1, 0.0})
        .up(Vec3d{0.0, 0.0, 1.0})
        .project(ProjectionKind::Perspective)
        .view_angle(34.0);
    const Lens planar_lens = Lens::planar();
    const auto lens_az = lens.azimuth_deg();
    const auto lens_el = lens.elevation_deg();
    const auto lens_view = lens.view_angle_deg();
    const auto lens_proj = lens.projection();
    const auto& lens_target = lens.target();
    const auto& lens_up = lens.up_vector();

    LightRig rig;
    rig.headlight().lamp_left().lamp_right().sun_key();
    rig.add(LightSpec{25.0, 35.0, LightSpace::Camera, LightStyle::Local});
    const bool rig_empty_before = rig.empty();
    const auto& rig_lights = rig.lights();
    rig.clear();
    const bool rig_empty_after = rig.empty();

    auto& p00 = fig.pane2("p00");
    p00.place(0, 0).align_xy("xy_shared");
    auto& trend_trace = p00.trend(x, y).label("trend");
    trend_trace.hide().show().shade(ShadeMode::Smooth).shade(ShadeMode::Faceted);
    const auto trace_kind = trend_trace.kind();
    const auto& trace_payload = trend_trace.payload();
    const auto& trace_label = trend_trace.label_text();
    const auto trace_width = trend_trace.stroke_width();
    const auto& trace_stroke = trend_trace.stroke_color();
    const auto& trace_palette = trend_trace.palette_ramp();
    const auto trace_shade_mode = trend_trace.shade_mode();
    const auto trace_lighting = trend_trace.lighting_mode();
    const auto trace_material = trend_trace.material_profile();
    const bool trace_visible = trend_trace.visible();
    const auto parent = fig.pane_index(p00);

    auto& p01 = fig.pane2("p01");
    p01.place(0, 1).align_xy("xy_shared");
    p01.columns(make_series({2.0, 3.0, 4.0, 3.2})).label("bars");

    auto& p10 = fig.pane3("p10");
    p10.place(1, 0).align_z("z_shared");
    const auto parent3 = fig.pane_index(p10);
    const auto gx = linspace(-2.5, 2.5, 18, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 18, ScalarType::Float64);
    const auto gz = make_grid_surface(18, 18);
    p10.sheet(gx, gy, gz).label("sheet");

    auto& p11 = fig.pane3("p11");
    p11.place(1, 1).align_z("z_shared");
    p11.lattice(gx, gy, gz).label("lattice");

    auto& nested = fig.subpane2(parent, 1, 2, 0, 0, 1, 1, "nested");
    nested.columns(make_series({1.0, 2.0, 1.5})).label("nested");

    auto& nested3 = fig.subpane3(parent3, 1, 1, 0, 0, 1, 1, "nested3");
    nested3.cloud3(
        std::array<double, 6>{0.2, 0.5, 0.9, 1.1, 1.5, 1.9},
        std::array<double, 6>{0.1, 0.4, 0.85, 1.0, 1.35, 1.8},
        std::array<double, 6>{0.3, 0.7, 0.9, 1.25, 1.52, 1.96})
        .label("nested3");

    const auto& fig_name = fig.name();
    const auto& fig_headline = fig.headline_text();
    const auto& fig_theme = fig.theme_tokens();
    const auto& fig_panes = fig.panes();
    const auto fig_pane_count = fig.pane_count();
    const auto& fig_extent = fig.extent();
    const auto& fig_mosaic = fig.mosaic_layout();
    const auto fig_tight = fig.tight_layout_enabled();
    const auto fig_tight_padding = fig.tight_layout_padding();
    const auto fig_tick_scale = fig.default_tick_font_scale();
    const auto fig_axis_scale = fig.default_axis_label_font_scale();
    const auto fig_legend_dock = fig.legend_dock();
    const auto& fig_x_captions = fig.x_captions();
    const auto& fig_y_captions = fig.y_captions();
    const auto& fig_z_captions = fig.z_captions();
    const auto& fig_row_titles = fig.row_titles();
    const auto fig_svg = fig.to_svg();
    const auto fig_pdf = fig.to_pdf();
    fig.mosaic(2, 2, 14.0, 14.0);
    fig.row_title(0, "Top Row");

    Pane scratch(PaneKind::Planar, Theme::paper(), "scratch");
    scratch.scale_x(ScalePolicy::linear())
        .x_limits({-0.2, 12.2})
        .x_tick_format(TickFormat::Auto)
        .x_tick_rotation(0.0)
        .x_ticks(make_series({0.0, 6.0, 12.0}), {"start", "mid", "end"})
        .grid(GridStyle::Major);

    const auto pane_kind = p00.kind();
    const auto& pane_title = p00.title_text();
    const auto& pane_theme = p00.theme_tokens();
    const auto& pane_x_axis = p00.x_axis();
    const auto& pane_y_axis = p00.y_axis();
    const auto& pane_z_axis = p00.z_axis();
    const auto& pane_color_axis = p00.color_axis();
    const auto pane_legend = p00.legend_place();
    const auto pane_grid_style = p00.grid_style();
    const auto& pane_lens = p00.lens();
    const auto& pane_lights = p00.lights();
    const auto& pane_traces = p00.traces();
    const auto pane_trace_count = p00.trace_count();
    const auto pane_scale_guide = p00.scale_guide_enabled();
    const auto& pane_caption = p00.caption_text();
    const auto& pane_rules = p00.rules();
    const auto& pane_bands = p00.bands();
    const auto& pane_pins = p00.pins();
    const auto& pane_x_align = p00.x_align_group();
    const auto& pane_y_align = p00.y_align_group();
    const auto& pane_z_align = p00.z_align_group();
    const auto& pane_place = p00.placement();
    const auto pane_visible = p00.visible();

    (void)paper_name;
    (void)paper_accents;
    (void)paper_bg;
    (void)paper_fg;
    (void)paper_surface;
    (void)paper_accent0;
    (void)ocean_name;
    (void)ramp_neutral;
    (void)ramp_thermal;
    (void)ramp_lagoon;
    (void)ramp_ocean;
    (void)ramp_meadow;
    (void)ramp_name;
    (void)ramp_stops;
    (void)ramp_empty;
    (void)ramp_sample_mid;
    (void)planar_lens;
    (void)lens_az;
    (void)lens_el;
    (void)lens_view;
    (void)lens_proj;
    (void)lens_target;
    (void)lens_up;
    (void)rig_empty_before;
    (void)rig_lights;
    (void)rig_empty_after;
    (void)trace_kind;
    (void)trace_payload;
    (void)trace_label;
    (void)trace_width;
    (void)trace_stroke;
    (void)trace_palette;
    (void)trace_shade_mode;
    (void)trace_lighting;
    (void)trace_material;
    (void)trace_visible;
    (void)fig_name;
    (void)fig_headline;
    (void)fig_theme;
    (void)fig_panes;
    (void)fig_pane_count;
    (void)fig_extent;
    (void)fig_mosaic;
    (void)fig_tight;
    (void)fig_tight_padding;
    (void)fig_tick_scale;
    (void)fig_axis_scale;
    (void)fig_legend_dock;
    (void)fig_x_captions;
    (void)fig_y_captions;
    (void)fig_z_captions;
    (void)fig_row_titles;
    (void)fig_svg;
    (void)fig_pdf;
    (void)pane_kind;
    (void)pane_title;
    (void)pane_theme;
    (void)pane_x_axis;
    (void)pane_y_axis;
    (void)pane_z_axis;
    (void)pane_color_axis;
    (void)pane_legend;
    (void)pane_grid_style;
    (void)pane_lens;
    (void)pane_lights;
    (void)pane_traces;
    (void)pane_trace_count;
    (void)pane_scale_guide;
    (void)pane_caption;
    (void)pane_rules;
    (void)pane_bands;
    (void)pane_pins;
    (void)pane_x_align;
    (void)pane_y_align;
    (void)pane_z_align;
    (void)pane_place;
    (void)pane_visible;

    fig.clear_pane_style_defaults().tight_layout(true, 8.0);
    const bool shown = fig.show("core_studio_exhaustive", 900, 620, false);
    (void)shown;
    save(fig);
