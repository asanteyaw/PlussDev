    Studio fig("rugplot", Theme::paper());
    auto& pane = fig.pane2("rugplot");
    rugplot(pane, y, 0.09).label("rugplot");
    save(fig);
