    Studio fig("bar3d", Theme::paper());
    auto& pane = fig.pane3("bar3d");
    auto zs = make_series(
        std::vector<double>{0.4,0.8,1.2,0.9, 0.7,1.1,1.6,1.3, 0.9,1.4,2.0,1.7, 0.6,1.0,1.5,1.2},
        {4,4});
    bar3d(pane, std::array<double, 4>{1,2,3,4}, std::array<double, 4>{1,2,3,4}, zs).label("bar3d");
    save(fig);
