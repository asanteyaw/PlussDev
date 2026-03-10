    Studio fig("pieplot", Theme::paper());
    auto& pane = fig.pane2("pieplot");
    pieplot(
        pane,
        make_series({18.0, 27.0, 21.0, 34.0}),
        {"NLP", "Risk", "Ops", "Infra"})
        .label("pieplot");
    save(fig);
