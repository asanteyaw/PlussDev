    Studio fig("core_pane_ticks_categories", Theme::paper());
    auto& pane = fig.pane2("ticks + categories");
    pane.x_ticks(make_series({1.0, 2.0, 3.0, 4.0}), {"Q1", "Q2", "Q3", "Q4"});
    pane.y_categories({"low", "mid", "high"}, 0.0, 1.0);
    pane.columns(make_series({0.0, 1.0, 2.0, 1.0})).label("band");
    save(fig);
