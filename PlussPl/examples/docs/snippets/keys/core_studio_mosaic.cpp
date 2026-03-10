    Studio fig("core_studio_mosaic", Theme::paper());
    fig.rename("core_mosaic").headline("Mosaic + placement").size(1240.0, 860.0);
    fig.mosaic(2, 2, 14.0, 14.0);

    auto& p00 = fig.pane2("trend").place(0, 0).grid(GridStyle::Major);
    p00.trend(x, y).label("signal");

    auto& p01 = fig.pane2("columns").place(0, 1);
    p01.x_categories({"A", "B", "C", "D"});
    p01.columns(make_series({1.3, 2.1, 1.8, 2.4})).label("bars");

    auto& p10 = fig.pane2("wedge").place(1, 0);
    p10.wedge(make_series({18.0, 27.0, 21.0, 34.0}), {"NLP", "Risk", "Ops", "Infra"}).label("mix");

    auto& p11 = fig.pane2("ribbon").place(1, 1);
    p11.ribbon(x, y.mul(0.65), -0.2).label("band");

    save(fig);
