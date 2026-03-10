    Studio fig("residplot", Theme::paper());
    auto& pane = fig.pane2("residplot");
    residplot(pane, x, y);
    save(fig);
