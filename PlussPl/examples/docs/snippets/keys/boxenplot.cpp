    Studio fig("boxenplot", Theme::paper());
    auto& pane = fig.pane2("boxenplot");
    boxenplot(pane, y).label("boxenplot");
    save(fig);
