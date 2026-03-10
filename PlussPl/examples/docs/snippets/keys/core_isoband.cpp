    Studio fig("core_isoband", Theme::paper());
    auto& pane = fig.pane2("isoband");
    const auto gx = linspace(-2.5, 2.5, 34, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 34, ScalarType::Float64);
    const auto gz = make_grid_surface(34, 34);
    pane.isoband(gx, gy, gz, 8).label("isoband");
    save(fig);
