    Studio fig("plot_surface", Theme::paper());
    auto& pane = fig.pane3("plot_surface");
    const auto gx = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    plot_surface(pane, gx, gy, gz).label("plot_surface");
    save(fig);
