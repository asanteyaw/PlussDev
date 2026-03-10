    Studio fig("core_trend", Theme::paper());
    auto& pane = fig.pane2("trend");
    pane.trend(x, y).label("trend");
    save(fig);
