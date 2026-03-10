    Studio fig("qqplot", Theme::paper());
    auto& pane = fig.pane2("qqplot");
    qqplot(pane, y);
    save(fig);
