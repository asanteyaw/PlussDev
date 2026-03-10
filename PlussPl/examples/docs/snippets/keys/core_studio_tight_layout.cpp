    Studio fig("core_studio_tight_layout", Theme::paper());
    fig.mosaic(1, 3, 8.0, 8.0);

    auto& p0 = fig.pane2("left pane with long title");
    p0.place(0, 0).x_label("time (days)").y_label("return");
    p0.trend(x, y).label("left");

    auto& p1 = fig.pane2("center pane");
    p1.place(0, 1).x_label("time (days)").y_label("return");
    p1.trend(x, y.mul(0.7).add(0.15)).label("center");

    auto& p2 = fig.pane2("right pane");
    p2.place(0, 2).x_label("time (days)").y_label("return");
    p2.trend(x, y.mul(0.5).add(0.25)).label("right");

    fig.tight_layout(true, 8.0);
    save(fig);
