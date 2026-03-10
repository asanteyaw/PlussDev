    Studio fig("flash_palette_kind", Theme::paper());
    auto& pane = fig.pane2("flash_palette_kind");
    const auto gx = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    auto& trace = heatmap(pane, gx, gy, gz, HeatmapOptions{.scale_guide = true});
    const auto palette_kind = flash_palette_kind("colorblind");
    trace.palette(flash_palette(palette_kind));
    save(fig);
