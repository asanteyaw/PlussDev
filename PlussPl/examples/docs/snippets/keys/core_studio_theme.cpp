    Studio fig("core_studio_theme", Theme::paper());
    fig.theme(themes::ocean()).headline("Theme::ocean()");
    auto& pane = fig.pane2("theme");
    pane.grid(GridStyle::MajorMinor).x_label("time").y_label("value");
    pane.trend(x, y).label("signal");
    pane.cloud(x, y.add(0.08), y.abs().mul(12.0).add(6.0), y).label("points");
    save(fig);
