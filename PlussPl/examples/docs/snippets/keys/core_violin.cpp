    Studio fig("core_violin", Theme::paper());
    auto& pane = fig.pane2("violin");
    ViolinOptions opts;
    opts.inner = ViolinInner::Quartile;
    opts.scale = ViolinScale::Width;
    opts.side = ViolinSide::Full;
    pane.violin(make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0}), opts).label("violin");
    save(fig);
