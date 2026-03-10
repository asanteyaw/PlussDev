    Studio fig("disgrid_kind", Theme::paper());
    DisGridPlotOptions options;
    options.kind = disgrid_kind("ecdf");
    disgridplot(fig, {y, y.add(0.25), y.sub(0.3)}, options);
    save(fig);
