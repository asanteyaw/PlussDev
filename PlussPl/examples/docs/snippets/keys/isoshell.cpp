    Studio fig("isoshell", Theme::paper());
    auto& pane = fig.pane3("isoshell");
    auto values = make_series(
        std::vector<double>{
            0.0, 0.1, 0.2, 0.12, 0.4, 0.75, 0.25, 0.72, 1.05,
            0.1, 0.22, 0.35, 0.24, 0.65, 1.02, 0.41, 0.98, 1.36,
            0.2, 0.33, 0.48, 0.36, 0.82, 1.2, 0.57, 1.18, 1.62,
        },
        {3, 3, 3});
    isoshell(pane,
             std::array<double, 3>{0.0, 1.0, 2.0},
             std::array<double, 3>{0.0, 1.0, 2.0},
             std::array<double, 3>{0.0, 1.0, 2.0},
             values,
             4)
        .label("isoshell");
    save(fig);
