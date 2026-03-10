    Studio fig("radviz", Theme::paper());
    RadvizOptions opts;
    opts.max_points = 120;
    radviz(fig, matrix, {"f1", "f2", "f3", "f4", "f5"}, Axis::Cols, opts);
    save(fig);
