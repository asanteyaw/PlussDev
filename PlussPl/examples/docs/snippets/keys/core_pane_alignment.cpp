    Studio fig("core_pane_alignment", Theme::paper());
    fig.mosaic(1, 2, 12.0, 12.0);
    auto& left = fig.pane2("left");
    left.place(0, 0).align_xy("shared_axes");
    left.trend(x, y).label("left");

    auto& right = fig.pane2("right");
    right.place(0, 1).align_xy("shared_axes");
    right.trend(x, y.mul(0.4).add(0.3)).label("right");
    save(fig);
