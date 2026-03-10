    Studio fig("core_ridge3", Theme::paper());
    auto& pane = fig.pane3("ridge3");
    const auto gx = linspace(-2.5, 2.5, 26, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 26, ScalarType::Float64);
    const auto gz = make_grid_surface(26, 26);
    pane.ridge3(gx, gy, gz, 9).label("ridge3");
    save(fig);
