    Studio fig("contour3D", Theme::paper());
    auto& pane = fig.pane3("contour3D");
    const auto gx = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    contour3D(pane, gx, gy, gz, 10).label("contour3D");
    save(fig);
