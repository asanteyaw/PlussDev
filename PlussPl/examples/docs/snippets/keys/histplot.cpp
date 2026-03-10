    Studio fig("histplot", Theme::paper());
    auto& pane = fig.pane2("histplot");
    histplot(pane, y, 20).label("histplot");
    save(fig);
