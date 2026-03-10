    Studio fig("move_legend", Theme::paper());
    auto& pane = fig.pane2("move_legend");
    lineplot(pane, x, y).label("signal");
    lineplot(pane, x, y.mul(0.8).add(0.2)).label("baseline");
    move_legend(pane, KeyPlace::NorthEast);
    move_legend(fig, KeyDock::East);
    save(fig);
