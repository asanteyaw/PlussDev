    Studio fig("core_studio_captions_row_titles", Theme::paper());
    fig.mosaic(2, 1, 12.0, 12.0);
    fig.row_title(0, "Signals");
    fig.row_title(1, "Diagnostics");
    fig.clear_row_title(1);
    fig.row_title(1, "Diagnostics");
    fig.x_caption("shared_xy", "Shared X Axis");
    fig.y_caption("shared_xy", "Shared Y Axis");

    auto& top = fig.pane2("top");
    top.place(0, 0).align_xy("shared_xy");
    top.trend(x, y).label("top");

    auto& bottom = fig.pane2("bottom");
    bottom.place(1, 0).align_xy("shared_xy");
    bottom.trend(x, y.mul(0.55).add(0.22)).label("bottom");
    save(fig);
