    Studio fig("camera_preset3d", Theme::paper());
    auto& pane = fig.pane3("camera_preset3d");
    const auto gx = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    camera_preset3d(pane, CameraPreset3D::Isometric);
    sheet(pane, gx, gy, gz).label("camera_preset3d");
    save(fig);
