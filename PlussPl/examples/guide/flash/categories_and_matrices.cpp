// Categories and matrices from a dataset. catplot, heatmap and clustermap take columns, not names: you pull each column out of the dataset's
// frame (a text column as a Note, a numeric one as a Series) and pass it in. This program draws every catplot kind with its options, then a
// correlation matrix as a heat map, a masked and annotated heat map, and a clustered map.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <iostream>

using namespace pluss::pl;

int main() {
  auto loaded = flash::load_dataset("tips");
  const auto tips = loaded->frame();
  const auto day = tips.features("day").as_note();
  const auto meal = tips.features("meal").as_note();
  const auto bill = tips.features("total_bill").as_series();
  std::cout << tips.ncases() << " parties, " << day.unique().numel() << " days, " << meal.unique().numel() << " meals\n";

  // 1. One catplot per kind: the same bills by day.
  const std::vector<std::pair<const char *, flash::CatPlotKind>> kinds{{"strip", flash::CatPlotKind::Strip}, {"swarm", flash::CatPlotKind::Swarm}, {"box", flash::CatPlotKind::Box},
                                                                      {"violin", flash::CatPlotKind::Violin}, {"bar", flash::CatPlotKind::Bar},   {"count", flash::CatPlotKind::Count}};
  for (const auto &[name, kind] : kinds) {
    auto fig = studio(std::string("flash_cat_") + name);
    auto &pane = fig.pane2(name);
    flash::CatPlotOptions options;
    options.kind = kind;
    flash::catplot(pane, day, bill, options);
    pane.x_label("day").y_label("total bill");
    fig.show();
  }

  // 2. Options: a hue splits each day by meal, dodge places the groups side by side, the order fixes the day order, and for bars the estimator
  //    and error bar choose the summary (a median with a confidence interval here).
  auto grouped = studio("flash_cat_hue");
  grouped.mosaic(1, 2);
  flash::CatPlotOptions box;
  box.kind = flash::CatPlotKind::Box;
  box.dodge = true;
  box.order = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  flash::catplot(grouped.pane2("Box by meal"), day, bill, meal, box);
  flash::CatPlotOptions bars;
  bars.kind = flash::CatPlotKind::Bar;
  bars.estimator = flash::CatEstimator::Median;
  bars.errorbar = flash::CatErrorBar::CI;
  bars.confidence_level = 0.9;
  bars.order = box.order;
  flash::catplot(grouped.pane2("Median bill, 90% interval"), day, bill, bars);
  grouped.show();

  // 3. A matrix from a dataset: the correlation of six factors, drawn with the defaults.
  auto factors = flash::load_dataset("factor_blocks")->frame();
  const int k = 6;
  std::vector<pluss::Series> rows;
  for (int i = 1; i <= k; ++i) rows.push_back(factors.features("factor_0" + std::to_string(i)).as_series().unsqueeze(0));
  auto correlation = rows[0].unite({rows[1], rows[2], rows[3], rows[4], rows[5]}, 0).corrcoef();
  auto axis = pluss::series::arange(0, k, pluss::ScalarType::Float64);
  std::cout << "correlation " << correlation.size(0) << " x " << correlation.size(1) << ", smallest " << correlation.min().item<double>() << ", largest off-diagonal "
            << (correlation - pluss::series::eye(k, pluss::ScalarType::Float64)).max().item<double>() << "\n";
  auto plain = studio("flash_heat_default");
  flash::heatmap(plain.pane2("Defaults"), axis, axis, correlation);
  plain.show();

  // 4. HeatmapOptions: fix the colour range to the full correlation scale, print the value in each cell, and keep only the lower triangle: a zero in the mask hides the cell.
  flash::HeatmapOptions heat;
  heat.vmin = -1.0;
  heat.vmax = 1.0;
  heat.annotate_levels = true;
  heat.mask = pluss::series::ones({k, k}, pluss::ScalarType::Float64).triu(1).eq(0.0);
  auto tuned = studio("flash_heat_options");
  flash::heatmap(tuned.pane2("Range -1 to 1, annotated, lower triangle"), axis, axis, correlation, heat);
  tuned.show();

  // 5. clustermap reorders rows and columns so similar ones sit together; cluster_rows and cluster_cols switch each direction.
  auto clusters = studio("flash_clustermap");
  clusters.mosaic(1, 3);
  flash::ClusterMapOptions both;
  flash::ClusterMapOptions rows_only;
  rows_only.cluster_cols = false;
  flash::ClusterMapOptions cols_only;
  cols_only.cluster_rows = false;
  flash::clustermap(clusters.pane2("Rows and columns"), axis, axis, correlation, both);
  flash::clustermap(clusters.pane2("Rows only"), axis, axis, correlation, rows_only);
  flash::clustermap(clusters.pane2("Columns only"), axis, axis, correlation, cols_only);
  clusters.show();
}
