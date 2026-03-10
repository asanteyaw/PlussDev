    Studio fig("scatter3D", Theme::paper());
    auto& pane = fig.pane3("scatter3D");
    scatter3D(
        pane,
        std::array<double, 6>{0.2, 0.5, 0.9, 1.1, 1.5, 1.9},
        std::array<double, 6>{0.1, 0.4, 0.85, 1.0, 1.35, 1.8},
        std::array<double, 6>{0.3, 0.7, 0.9, 1.25, 1.52, 1.96})
        .label("scatter3D");
    save(fig);
