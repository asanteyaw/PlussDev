    Studio fig("kdeplot", Theme::paper());
    auto& pane = fig.pane2("kdeplot");
    kdeplot(pane, y, 220).label("kdeplot");
    save(fig);
