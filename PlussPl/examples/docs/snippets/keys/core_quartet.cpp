    Studio fig("core_quartet", Theme::paper());
    auto& pane = fig.pane2("quartet");
    const auto groups = make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0});
    const auto vals = make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0});
    pane.quartet(groups, vals).label("quartet");
    save(fig);
