    Studio fig("cloud3", Theme::paper());
    auto& pane = fig.pane3("cloud3");
    cloud3(pane,
           std::array<double, 6>{0.2, 0.5, 0.9, 1.1, 1.5, 1.9},
           std::array<double, 6>{0.1, 0.4, 0.85, 1.0, 1.35, 1.8},
           std::array<double, 6>{0.3, 0.7, 0.9, 1.25, 1.52, 1.96},
           std::array<double, 6>{10.0, 12.0, 14.0, 16.0, 18.0, 20.0},
           std::array<double, 6>{0.1, 0.22, 0.4, 0.58, 0.73, 0.9})
        .label("cloud3");
    save(fig);
