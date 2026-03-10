    Studio fig("catgridplot", Theme::paper());
    const Note cats = make_note({"A", "A", "A", "B", "B", "B", "C", "C", "C"});
    const Series vals = make_series({0.9, 1.2, 1.4, 1.1, 1.6, 1.9, 0.7, 0.8, 1.0});
    catgridplot(fig, {cats, cats, cats}, {vals, vals.add(0.3), vals.sub(0.2)});
    save(fig);
