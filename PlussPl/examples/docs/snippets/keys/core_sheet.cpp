    Studio fig("core_sheet", Theme::paper());
    auto& pane = fig.pane3("sheet");
    const auto gx = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    pane.sheet(gx, gy, gz).label("sheet");
    save(fig);
