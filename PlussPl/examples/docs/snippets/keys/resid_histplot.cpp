    Studio fig("resid_histplot", Theme::paper());
    auto& pane = fig.pane2("resid_histplot");
    resid_histplot(pane, x, y);
    save(fig);
