    Studio fig("disgridplot", Theme::paper());
    disgridplot(fig, {y, y.add(0.25), y.sub(0.3)}, "hist");
    save(fig);
