    Studio fig("regplot", Theme::paper());
    auto& pane = fig.pane2("regplot");
    regplot(pane, x, y).label("regplot");
    save(fig);
