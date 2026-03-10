    Studio fig("spire3", Theme::paper());
    auto& pane = fig.pane3("spire3");
    auto zs = make_series(
        std::vector<double>{0.4,0.8,1.2,0.9, 0.7,1.1,1.6,1.3, 0.9,1.4,2.0,1.7, 0.6,1.0,1.5,1.2},
        {4,4});
    spire3(pane, std::array<double, 4>{1,2,3,4}, std::array<double, 4>{1,2,3,4}, zs).label("spire3");
    save(fig);
