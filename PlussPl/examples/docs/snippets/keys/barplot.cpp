    Studio fig("barplot", Theme::paper());
    auto& pane = fig.pane2("barplot");
    barplot(pane,
            make_note({"infra", "models", "data", "ops"}),
            make_series({21.0, 28.0, 24.0, 19.0}))
        .label("barplot");
    save(fig);
