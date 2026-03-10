    Studio fig("diverging_palette", Theme::paper());
    auto& pane = fig.pane2("diverging_palette");
    const auto gx = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(diverging_palette(Rgba::from_u8(42, 95, 153), Rgba::from_u8(181, 80, 54), 9));
    save(fig);
