    Pane defaults(PaneKind::Planar, Theme::paper(), "defaults");
    defaults.grid(GridStyle::MajorMinor)
        .legend(KeyPlace::NorthEast)
        .legend_frame(KeyFrame::Framed)
        .legend_shape(KeyShape::Vertical)
        .tick_font_scale(1.12)
        .axis_label_font_scale(1.16)
        .x_major_grid(true)
        .y_major_grid(true);

    Studio fig("core_studio_pane_style_defaults", Theme::paper());
    fig.mosaic(1, 2, 12.0, 12.0).pane_style_defaults(defaults);
    auto& left = fig.pane2("inherits defaults");
    left.place(0, 0).x_label("x").y_label("y");
    left.trend(x, y).label("defaulted");

    fig.clear_pane_style_defaults();
    auto& right = fig.pane2("after clear");
    right.place(0, 1).x_label("x").y_label("y");
    right.trend(x, y.mul(0.6).add(0.2)).label("manual");
    save(fig);
