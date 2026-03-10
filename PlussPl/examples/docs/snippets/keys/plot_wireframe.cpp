    Studio fig("plot_wireframe", Theme::paper());
    auto& pane = fig.pane3("plot_wireframe");
    const auto gx = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    plot_wireframe(pane, gx, gy, gz).label("plot_wireframe");
    save(fig);
