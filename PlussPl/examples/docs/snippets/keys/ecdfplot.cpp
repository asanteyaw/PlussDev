    Studio fig("ecdfplot", Theme::paper());
    auto& pane = fig.pane2("ecdfplot");
    ecdfplot(pane, y).label("ecdfplot");
    save(fig);
