    Studio fig("trace3d", Theme::paper());
    auto& pane = fig.pane3("trace3d");
    const auto gx = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    auto& tr = sheet(pane, gx, gy, gz).label("sheet");
    trace3d(tr, Trace3DOptions{.preset = TracePreset3D::Surface});
    save(fig);
