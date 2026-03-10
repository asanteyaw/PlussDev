    Studio fig("clustermap", Theme::paper());
    auto& pane = fig.pane2("clustermap");
    const auto gx = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gy = linspace(-3.0, 3.0, 40, ScalarType::Float64);
    const auto gz = make_grid_surface(40, 40);
    clustermap(pane, gx, gy, gz).label("clustermap");
    save(fig);
