    Studio fig("get_dataset_names", Theme::paper());
    auto& pane = fig.pane2("get_dataset_names");
    const auto names = get_dataset_names();
    const std::vector<double> counts(static_cast<std::size_t>(names.numel()), 1.0);
    barplot(pane, names, make_series(counts)).label("datasets");
    save(fig);
