    Studio fig("triplot", Theme::paper());
    TriplotOptions opts;
    opts.max_panels = 6;
    opts.max_points = 140;
    opts.kind = TriplotKind::Cloud;
    triplot(fig, matrix, {"f1", "f2", "f3", "f4", "f5"}, Axis::Cols, opts);
    save(fig);
