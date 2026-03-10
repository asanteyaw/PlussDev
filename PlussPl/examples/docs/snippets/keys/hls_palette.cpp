    Studio fig("hls_palette", Theme::paper());
    auto& pane = fig.pane2("hls_palette");
    const auto gx = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(hls_palette(8, 0.72, 0.54));
    save(fig);
