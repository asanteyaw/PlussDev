    Studio fig("core_studio_subpane3", Theme::paper());
    fig.mosaic(1, 1, 12.0, 12.0);

    auto& parent = fig.pane3("parent3").place(0, 0);
    const std::size_t root = fig.pane_index(parent);

    const auto gx = linspace(-2.4, 2.4, 20, ScalarType::Float64);
    const auto gy = linspace(-2.4, 2.4, 20, ScalarType::Float64);
    const auto gz = make_grid_surface(20, 20);

    auto& a = fig.subpane3(root, 1, 2, 0, 0, 1, 1, "sheet");
    a.sheet(gx, gy, gz).label("sheet");
    auto& b = fig.subpane3(root, 1, 2, 0, 1, 1, 1, "lattice");
    b.lattice(gx, gy, gz).label("lattice");
    save(fig);
