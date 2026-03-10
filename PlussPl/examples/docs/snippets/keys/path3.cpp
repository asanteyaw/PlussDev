    Studio fig("path3", Theme::paper());
    auto& pane = fig.pane3("path3");
    path3(pane,
          std::array<double, 6>{0.0, 0.4, 0.8, 1.2, 1.6, 2.0},
          std::array<double, 6>{0.0, 0.3, 0.9, 1.1, 1.4, 2.0},
          std::array<double, 6>{0.2, 0.5, 0.7, 1.1, 1.35, 1.8})
        .label("path3");
    save(fig);
