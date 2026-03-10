    Studio fig("areaplot", Theme::paper());
    auto& pane = fig.pane2("areaplot");
    areaplot(pane, x, y, -0.2).label("areaplot");
    save(fig);
