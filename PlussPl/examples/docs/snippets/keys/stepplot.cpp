    Studio fig("stepplot", Theme::paper());
    auto& pane = fig.pane2("stepplot");
    stepplot(pane, x, y).label("stepplot");
    save(fig);
