    Studio fig("core_axis_layout_exhaustive_3d", Theme::paper());
    auto& pane = fig.pane3("axis/layout exhaustive 3d");
    const auto gx = linspace(-2.4, 2.4, 22, ScalarType::Float64);
    const auto gy = linspace(-2.4, 2.4, 22, ScalarType::Float64);
    const auto gz = make_grid_surface(22, 22);

    pane.title("axis/layout exhaustive 3d");
    pane.scale_z(ScalePolicy::linear());
    pane.z_limits({-2.0, 2.0}).color_limits({-1.0, 1.0});
    pane.z_limit_method(LimitMethod::Padded).color_limit_method(LimitMethod::TickAligned);
    pane.z_tick_format(TickFormat::Engineering);
    pane.z_auto_tick_decimals(2);
    pane.clear_z_auto_tick_decimals();
    pane.z_auto_tick_decimals(1);

    pane.z_tick_rotation(0.0)
        .z_tick_label_gap(6.0)
        .z_label_gap(16.0)
        .z_label_rotation(90.0)
        .clear_z_tick_rotation()
        .clear_z_tick_label_gap()
        .clear_z_label_gap()
        .clear_z_label_rotation()
        .z_tick_rotation(0.0)
        .z_tick_label_gap(6.0)
        .z_label_gap(14.0);

    pane.z_tick_labels(true).z_label("z");
    pane.z_ticks(make_series({-1.0, 0.0, 1.0}), {"low", "mid", "high"});
    pane.clear_z_ticks();
    pane.z_categories({"layer1", "layer2", "layer3"}, 0.0, 1.0);
    pane.z_major_grid(true).z_minor_grid(true).scale_guide(true);
    pane.rule_z(0.0, "slice").band_z({-0.2, 0.2}, "z band");
    pane.pin("3d pin", Vec3d{0.2, 0.2, 0.2});
    pane.align_z("grp3").align_xyz("grp3");
    pane.visible(true).axes_visible(true).auto_z().place(0, 0);
    pane.sheet(gx, gy, gz).label("sheet");
    save(fig);
