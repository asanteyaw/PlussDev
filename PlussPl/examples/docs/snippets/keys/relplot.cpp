    Studio fig("relplot", Theme::paper());
    auto& pane = fig.pane2("relplot");
    relplot(pane, x, y, "line").label("relplot");
    save(fig);
