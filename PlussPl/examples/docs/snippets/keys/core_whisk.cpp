    Studio fig("core_whisk", Theme::paper());
    auto& pane = fig.pane2("whisk");
    const auto px = make_series({1.0, 2.0, 3.0, 4.0});
    const auto py = make_series({3.2, 3.9, 4.1, 4.8});
    const auto pe = make_series({0.15, 0.2, 0.17, 0.25});
    pane.whisk(px, py, pe).label("whisk");
    save(fig);
