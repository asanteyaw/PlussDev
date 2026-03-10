    Studio fig("core_pane_visibility", Theme::paper());
    fig.mosaic(1, 2, 12.0, 12.0);
    auto& left = fig.pane2("visible");
    left.place(0, 0).show().axes_visible(true);
    left.trend(x, y).label("visible");

    auto& right = fig.pane2("axes hidden");
    right.place(0, 1).visible(true).axes_visible(false);
    right.trend(x, y.mul(0.55).add(0.2)).label("axes off");
    save(fig);
