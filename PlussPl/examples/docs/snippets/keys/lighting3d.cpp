    Studio fig("lighting3d", Theme::paper());
    auto& pane = fig.pane3("lighting3d");
    const auto gx = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    lighting3d(
        pane,
        Lighting3DOptions{
            .clear_first = true,
            .headlight = true,
            .lamp_left = true,
            .lamp_right = true,
            .sun_key = false,
        });
    sheet(pane, gx, gy, gz).label("lighting3d");
    save(fig);
