    Studio fig("core_contour", Theme::paper());
    auto& pane = fig.pane2("contour");
    const auto gx = linspace(-2.5, 2.5, 26, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 26, ScalarType::Float64);
    const auto gz = make_grid_surface(26, 26);
    pane.contour(gx, gy, gz, 10).label("contour");
    save(fig);
