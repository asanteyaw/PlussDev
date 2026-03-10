    Studio fig("andrews_curves", Theme::paper());
    AndrewsCurvesOptions opts;
    opts.max_curves = 120;
    andrews_curves(fig, matrix, {"f1", "f2", "f3", "f4", "f5"}, Axis::Cols, opts);
    save(fig);
