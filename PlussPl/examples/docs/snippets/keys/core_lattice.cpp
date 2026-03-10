    Studio fig("core_lattice", Theme::paper());
    auto& pane = fig.pane3("lattice");
    const auto gx = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    pane.lattice(gx, gy, gz).label("lattice");
    save(fig);
