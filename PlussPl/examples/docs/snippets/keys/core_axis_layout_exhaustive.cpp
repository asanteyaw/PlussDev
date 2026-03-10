    Studio fig("core_axis_layout_exhaustive", Theme::paper());
    fig.size(1220.0, 780.0).mosaic(1, 1, 12.0, 12.0);
    auto& pane = fig.pane2("axis/layout exhaustive");
    const auto xs = linspace(1.0, 120.0, 180, ScalarType::Float64);
    const auto ys = xs.mul(0.08).sin().mul(xs.mul(0.02).add(0.6));

    pane.title("axis/layout exhaustive");
    pane.scale_x(ScalePolicy::log10()).scale_y(ScalePolicy::symlog(10.0, 0.2));
    pane.x_limits({1.0, 120.0}).y_limits({-2.0, 2.0}).color_limits({-1.0, 1.0});
    pane.x_limit_method(LimitMethod::TickAligned)
        .y_limit_method(LimitMethod::Padded)
        .color_limit_method(LimitMethod::Tight);
    pane.x_tick_format(TickFormat::Scientific).y_tick_format(TickFormat::Fixed3);
    pane.x_auto_tick_decimals(2).y_auto_tick_decimals(3);
    pane.clear_x_auto_tick_decimals().clear_y_auto_tick_decimals();
    pane.x_auto_tick_decimals(1).y_auto_tick_decimals(2);
    pane.tick_font_scale(1.08).axis_label_font_scale(1.10);

    pane.x_tick_rotation(18.0).y_tick_rotation(0.0);
    pane.x_tick_label_gap(7.0).y_tick_label_gap(6.0);
    pane.x_label_gap(16.0).y_label_gap(15.0);
    pane.x_label_rotation(0.0).y_label_rotation(90.0);
    pane.clear_x_tick_rotation().clear_y_tick_rotation();
    pane.clear_x_tick_label_gap().clear_y_tick_label_gap();
    pane.clear_x_label_gap().clear_y_label_gap();
    pane.clear_x_label_rotation().clear_y_label_rotation();
    pane.x_tick_rotation(16.0).x_tick_label_gap(6.0).x_label_gap(14.0);
    pane.y_tick_rotation(0.0).y_tick_label_gap(6.0).y_label_gap(14.0);

    pane.x_tick_labels(true).y_tick_labels(true);
    pane.auto_x().auto_y().auto_color();
    pane.x_label("time").y_label("response");
    pane.x_ticks(make_series({1.0, 10.0, 50.0, 120.0}), {"t1", "t10", "t50", "t120"});
    pane.y_ticks(make_series({-1.0, 0.0, 1.0}), {"low", "mid", "high"});
    pane.clear_x_ticks().clear_y_ticks();
    pane.x_categories({"Q1", "Q2", "Q3", "Q4"});
    pane.y_categories({"vlow", "low", "mid", "high"}, 0.0, 1.0);

    pane.legend(KeyPlace::NorthEast)
        .legend_frame(KeyFrame::Framed)
        .legend_shape(KeyShape::Vertical)
        .grid(GridStyle::MajorMinor)
        .x_major_grid(true)
        .y_major_grid(true)
        .x_minor_grid(true)
        .y_minor_grid(true)
        .scale_guide(false)
        .caption("axis controls")
        .rule_x(10.0, "x rule")
        .rule_y(0.0, "y rule")
        .band_x({2.0, 3.0}, "x band")
        .band_y({-0.2, 0.2}, "y band")
        .pin("marker", 12.0, 0.8)
        .align_x("grp_xy")
        .align_y("grp_xy")
        .align_xy("grp_xy")
        .parallel_xy_labels(true)
        .visible(true)
        .axes_visible(true)
        .hide()
        .show()
        .clear_x_align()
        .clear_y_align()
        .clear_align()
        .place(0, 0, 1, 1);

    pane.trend(xs, ys).label("trend");
    save(fig);
