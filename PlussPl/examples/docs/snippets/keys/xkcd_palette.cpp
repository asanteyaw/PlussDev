    Studio fig("xkcd_palette", Theme::paper());
    auto& pane = fig.pane2("xkcd_palette");
    const auto gx = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    trace.palette(xkcd_palette());
    save(fig);
