    Studio fig("core_studio_subpane2", Theme::paper());
    fig.mosaic(1, 1, 12.0, 12.0);

    auto& parent = fig.pane2("parent").place(0, 0).caption("subpane2 canvas");
    parent.trend(x, y).label("parent trend");
    const std::size_t root = fig.pane_index(parent);

    auto& top = fig.subpane2(root, 2, 2, 0, 0, 1, 2, "top");
    top.trend(x, y.mul(0.7)).label("top");
    auto& bl = fig.subpane2(root, 2, 2, 1, 0, 1, 1, "bl");
    bl.columns(make_series({2.0, 4.0, 3.0})).label("bl");
    auto& br = fig.subpane2(root, 2, 2, 1, 1, 1, 1, "br");
    br.bucket(y, 14).label("br");
    save(fig);
