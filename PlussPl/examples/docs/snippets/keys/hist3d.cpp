    Studio fig("hist3d", Theme::paper());
    auto& pane = fig.pane3("hist3d");
    hist3d(pane, x, y, Hist3DOptions{.bins_x = 28, .bins_y = 22, .as_surface = true}).label("hist3d");
    save(fig);
