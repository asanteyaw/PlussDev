    Studio fig("core_trace_style_3d", Theme::paper());
    auto& pane = fig.pane3("trace styling 3d");
    const auto gx = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gy = linspace(-2.5, 2.5, 24, ScalarType::Float64);
    const auto gz = make_grid_surface(24, 24);
    pane.sheet(gx, gy, gz)
        .label("sheet")
        .shade(ShadeMode::Smooth)
        .lighting(LightingMode::Gouraud)
        .material(MaterialProfile::Metal)
        .transparent(true)
        .annotate_levels(true);
    save(fig);
