    Studio fig("core_studio_context_scales", Theme::paper());
    fig.context_scales(1.22, 1.28).size(1120.0, 720.0);
    auto& pane = fig.pane2("context_scales");
    pane.x_label("x").y_label("y").grid(GridStyle::Major);
    pane.trend(x, y).label("scaled text");
    save(fig);
