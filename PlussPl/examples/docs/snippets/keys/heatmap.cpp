    Studio fig("heatmap", Theme::paper());
    auto& pane = fig.pane2("heatmap");
    const auto gx = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    save(fig);
