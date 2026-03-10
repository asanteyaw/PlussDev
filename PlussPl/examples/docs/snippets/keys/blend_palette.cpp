    Studio fig("blend_palette", Theme::paper());
    auto& pane = fig.pane2("blend_palette");
    const auto gx = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(blend_palette(
        {Rgba::from_u8(42, 95, 153), Rgba::from_u8(242, 178, 109), Rgba::from_u8(181, 80, 54)},
        "blend_demo"));
    save(fig);
