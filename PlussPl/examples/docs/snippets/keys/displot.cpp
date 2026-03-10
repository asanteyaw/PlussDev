    Studio fig("displot", Theme::paper());
    auto& pane = fig.pane2("displot");
    displot(pane, y, 18).label("displot");
    save(fig);
