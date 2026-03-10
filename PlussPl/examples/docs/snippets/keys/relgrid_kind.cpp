    Studio fig("relgrid_kind", Theme::paper());
    RelGridPlotOptions options;
    options.kind = relgrid_kind("line");
    relgridplot(
        fig,
        {x, x, x},
        {y, y.mul(0.8), y.mul(0.6).add(0.2)},
        options);
    save(fig);
