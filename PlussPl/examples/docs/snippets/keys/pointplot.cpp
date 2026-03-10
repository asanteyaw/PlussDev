    Studio fig("pointplot", Theme::paper());
    auto& pane = fig.pane2("pointplot");
    auto px = make_series({1.0, 2.0, 3.0, 4.0});
    auto py = make_series({3.2, 3.9, 4.1, 4.8});
    auto pe = make_series({0.15, 0.2, 0.17, 0.25});
    pointplot(pane, px, py, pe).label("pointplot");
    save(fig);
