    Studio fig("camera3d", Theme::paper());
    auto& pane = fig.pane3("camera3d");
    const auto gx = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    camera3d(
        pane,
        Camera3DOptions{
            .azimuth_deg = -28.0,
            .elevation_deg = 22.0,
            .view_angle_deg = 26.0,
            .projection = ProjectionKind::Perspective,
        });
    sheet(pane, gx, gy, gz).label("camera3d");
    save(fig);
