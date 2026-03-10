    Studio fig("core_fieldmap", Theme::paper());
    auto& pane = fig.pane2("fieldmap");
    const auto gx = linspace(-3.0, 3.0, 42, ScalarType::Float64);
    const auto gy = linspace(-3.0, 3.0, 42, ScalarType::Float64);
    const auto gz = make_grid_surface(42, 42);
    pane.scale_guide(true);
    pane.fieldmap(gx, gy, gz).label("fieldmap");
    save(fig);
