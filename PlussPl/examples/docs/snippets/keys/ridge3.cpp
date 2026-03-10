    Studio fig("ridge3", Theme::paper());
    auto& pane = fig.pane3("ridge3");
    const auto gx = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    ridge3(pane, gx, gy, gz, 8).label("ridge3");
    save(fig);
