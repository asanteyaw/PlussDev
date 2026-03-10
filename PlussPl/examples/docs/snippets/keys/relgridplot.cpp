    Studio fig("relgridplot", Theme::paper());
    relgridplot(
        fig,
        {x, x, x},
        {y, y.mul(0.8), y.mul(0.6).add(0.2)},
        "scatter");
    save(fig);
