    Studio fig("parallelplot", Theme::paper());
    ParallelCoordinatesOptions opts;
    opts.max_points = 120;
    parallelplot(fig, matrix, {"f1", "f2", "f3", "f4", "f5"}, Axis::Cols, opts);
    save(fig);
