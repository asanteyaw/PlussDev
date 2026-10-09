#include <iostream>
#include <vector>

#include "pluss/pluss.h"

int main() {
  using pluss::table::Atlas;
  using pluss::table::Axis;
  using pluss::table::DataFrame;
  using pluss::table::ErrorTarget;
  using pluss::table::RankNullRule;
  using pluss::table::RankTieRule;
  using pluss::table::ScanStatistic;
  using pluss::table::StatisticOptions;
  using pluss::table::TextAlign;
  using pluss::table::Slate;
  using pluss::table::SlateValue;
  using pluss::table::TimeWindowStatistic;
  using pluss::table::UnicodeForm;
  using pluss::table::WindowClosure;
  using pluss::table::WindowOptions;
  using pluss::table::WindowStatistic;

  auto ids = Slate(pluss::make_series(std::vector<std::int64_t>{
      1001, 1002, 1003, 1004, 1005, 1006}));
  auto scores = Slate(pluss::make_series(std::vector<double>{
      10.0, 20.0, 20.0, 40.0, 50.0, 50.0}));
  auto costs = Slate(pluss::make_series(std::vector<double>{
      2.0, 4.0, 5.0, 8.0, 10.0, 12.0}));
  auto groups = Slate(pluss::make_note({
      "west", "east", "west", "east", "west", "east"}));
  auto labels = Slate(pluss::make_note({
      "alpha-10", "Beta-20", "red|blue", "west-30", "east-40", "red|green"}));
  auto dates = Slate(pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-04T00:00:00Z",
      "2024-01-05T00:00:00Z",
      "2024-01-07T00:00:00Z",
      "2024-01-08T00:00:00Z"}));

  auto df = DataFrame({
      {"account", ids},
      {"score", scores},
      {"cost", costs},
      {"group", groups},
      {"label", labels},
      {"date", dates},
  });

  auto score_total = df.features("score").sum();
  auto first_date = df.features("date").min().as_timeline();
  auto last_date = df.features("date").max().as_timeline();
  auto score_var = df.features("score").var(StatisticOptions{.ddof = 1, .min_count = 2});
  auto score_std = df.features("score").std(StatisticOptions{.ddof = 1, .min_count = 2});
  auto score_se = df.features("score").se(ErrorTarget::Mean, StatisticOptions{.ddof = 1, .min_count = 2});
  auto score_mode = df.features("score").mode();
  auto score_rank = df.features("score").rank(RankTieRule::Dense, RankNullRule::Keep);
  auto score_freq = df.features("score").freq();
  auto score_distinct = df.features("score").distinct();
  auto score_ndistinct = df.features("score").ndistinct();
  auto score_duplicates = df.features("score").repeated();
  auto score_scan = df.features("score").scan(ScanStatistic::Sum);
  auto score_skew = df.features("score").skew();
  auto score_kurt = df.features("score").kurtosis();
  auto score_quantiles = df.features("score").percentiles({0.25, 0.5, 0.75});
  auto score_cost_cov = df.features("score").covar(df.features("cost"));
  auto score_cost_corr = df.features("score").corr_with(df.features("cost"));
  auto score_cost_dot = df.features("score").matmul(df.features("cost"));

  auto upper_label = df.features("label").nt().upper();
  auto label_lengths = df.features("label").nt().length();
  auto label_matches = df.features("label").nt().matches("[A-Za-z]+");
  auto label_parts = df.features("label").nt().split("-");
  auto label_partition = df.features("label").nt().partition("-");
  auto label_groups = df.features("label").nt().extract("([A-Za-z]+)-([0-9]+)", {"word", "number"});
  auto label_padded = df.features("label").nt().pad(12, TextAlign::Right, '.');
  auto label_chars = df.features("label").nt().chars(0, 4);
  auto label_normalized = df.features("label").nt().normalize(UnicodeForm::NFC);
  auto label_encoded = df.features("label").nt().encode("hex");
  auto label_indicators = df.features("label").nt().indicate("|");

  auto df_cov = df.covar();
  auto df_corr = df.corr();
  auto df_corr_with = df.corr_with(df);
  auto df_corr_many = df.corr_with({df, df});
  auto df_quantiles = df.percentiles({0.1, 0.5, 0.9});
  auto df_modes = df.mode();
  auto df_ranks = df.rank(RankTieRule::Average, RankNullRule::Keep);
  auto df_var = df.var(Axis::Case, StatisticOptions{.ddof = 1, .min_count = 2});
  auto df_std = df.std(Axis::Case, StatisticOptions{.ddof = 1, .min_count = 2});
  auto df_se = df.se(Axis::Case, ErrorTarget::Mean, StatisticOptions{.ddof = 1, .min_count = 2});
  auto df_skew = df.skew();
  auto df_kurt = df.kurtosis();
  auto df_scan = df.scan(ScanStatistic::Sum);
  auto numeric = df.features({"score", "cost"});
  auto df_moving = numeric.ts().rolling(3, WindowStatistic::Mean);
  auto df_moving_var = numeric.ts().rolling(3, WindowStatistic::Var, WindowOptions{.min_count = 2, .ddof = 1});
  auto df_weighted = numeric.ts().weigh({1.0, 2.0, 3.0}, WindowStatistic::Mean);
  auto df_bounded = numeric.ts().bounded(
      [](std::int64_t size, const WindowOptions&) {
        std::vector<std::int64_t> starts(static_cast<std::size_t>(size), 0);
        std::vector<std::int64_t> ends;
        ends.reserve(static_cast<std::size_t>(size));
        for (std::int64_t row = 0; row < size; ++row) {
          ends.push_back(row + 1);
        }
        return std::pair{std::move(starts), std::move(ends)};
      },
      WindowStatistic::Mean);
  auto df_growing = numeric.ts().expanding(WindowStatistic::Mean);
  auto df_decay = numeric.ts().ewm(0.4, WindowStatistic::Mean, true, true, 2);
  auto df_grouped = df.features("score").ts().grouped(
      df.features("group"), 2, WindowStatistic::Mean);
  auto df_timed = df.features("score").ts().timed(
      df.features("date"), 3LL * 24 * 60 * 60 * 1000 * 1000 * 1000,
      TimeWindowStatistic::Mean,
      StatisticOptions{.ddof = 1, .min_count = 1},
      WindowClosure::Right);

  auto cleaned_labels = df.features({"label"}).nt().casefold();
  auto label_lengths_df = df.features({"label"}).nt().length();
  auto label_tokens_df = df.features({"label"}).nt().split("-");
  auto label_indicators_df = df.features({"label"}).nt().indicate("|");

  auto mapped_scores = df.features({"score", "cost"}).map(
      [](const SlateValue& value) {
        return SlateValue(Slate(value.as_series() * 2.0));
      });
  auto row_score = df.transform(
      [](const DataFrame& row) {
        return SlateValue(Slate(row.features("score").as_series() +
                                  row.features("cost").as_series()));
      },
      Axis::Case);
  auto expanded_cases = df.transform(
      [](const DataFrame& row) {
        return row.extend(row, pluss::table::FrameMatchRule::Unite, true, false);
      },
      Axis::Case,
      pluss::table::ResultShape::Expand);
  auto folded_total = df.fold(
      [](const DataFrame& value) {
        return Slate(value.features("score").sum().as_series());
      });
  auto flowed = df.flow(
      [](const DataFrame& value) {
        return value.features({"label"}).nt().upper();
      });

  auto df_nulls_dropped = df.dropnull({"score", "label"}, true);
  auto df_ndistinct = df.features({"group", "label"}).ndistinct(true);
  auto df_freq = df.features({"group"}).freq();
  auto df_duplicated = df.features({"score"}).duplicated();
  auto df_unique_cases = df.features({"score"}).drop_duplicates();
  auto df_renamed = df.rename({{"score", "amount"}, {"cost", "expense"}});
  auto df_relabelled = df.features({"score", "cost"}).rename({"amount", "expense"});

  auto left = DataFrame({
      {"a", Slate(pluss::make_series(std::vector<double>{1.0, 3.0}))},
      {"b", Slate(pluss::make_series(std::vector<double>{2.0, 4.0}))},
  });
  auto right = DataFrame({
      {"z", Slate(pluss::make_series(std::vector<double>{5.0, 6.0}))},
  });
  auto product = left.matmul(right);

  auto prepared = df.prep_sql("SELECT account, score FROM df WHERE score > {min_score}");
  auto sql_result = prepared.execute({{"min_score", 15.0}});

  auto atlas = Atlas({numeric, numeric});
  auto text_atlas = Atlas({df, df});
  auto atlas_mapped = atlas.map(
      [](const SlateValue& value) {
        return SlateValue(Slate(value.as_series() + 1.0));
      });
  auto atlas_moving = atlas.ts().rolling(2, WindowStatistic::Sum);
  auto atlas_flowed = text_atlas.flow(
      [](const Atlas& value) {
        return value.map_frames([](const DataFrame& frame) {
          return frame.features({"label"}).nt().upper();
        });
      });

  std::cout << "score_total=" << score_total << "\n";
  std::cout << "first_date=" << first_date << "\n";
  std::cout << "last_date=" << last_date << "\n";
  std::cout << "score_var=" << score_var << "\n";
  std::cout << "score_std=" << score_std << "\n";
  std::cout << "score_se=" << score_se << "\n";
  std::cout << "score_mode cases=" << score_mode.size() << "\n";
  std::cout << "score_rank cases=" << score_rank.size() << "\n";
  std::cout << "score_freq cases=" << score_freq.size() << "\n";
  std::cout << "score_distinct cases=" << score_distinct.size() << "\n";
  std::cout << "score_ndistinct=" << score_ndistinct << "\n";
  std::cout << "score_duplicates cases=" << score_duplicates.size() << "\n";
  std::cout << "score_scan cases=" << score_scan.size() << "\n";
  std::cout << "score_skew=" << score_skew << "\n";
  std::cout << "score_kurt=" << score_kurt << "\n";
  std::cout << "score_quantiles cases=" << score_quantiles.size() << "\n";
  std::cout << "score_cost_cov=" << score_cost_cov << "\n";
  std::cout << "score_cost_corr=" << score_cost_corr << "\n";
  std::cout << "score_cost_dot=" << score_cost_dot << "\n";
  std::cout << "upper_label cases=" << upper_label.size() << "\n";
  std::cout << "label_lengths cases=" << label_lengths.size() << "\n";
  std::cout << "label_matches cases=" << label_matches.size() << "\n";
  std::cout << "label_parts features=" << label_parts.nfeatures() << "\n";
  std::cout << "label_partition features=" << label_partition.nfeatures() << "\n";
  std::cout << "label_groups features=" << label_groups.nfeatures() << "\n";
  std::cout << "label_padded cases=" << label_padded.size() << "\n";
  std::cout << "label_chars cases=" << label_chars.size() << "\n";
  std::cout << "label_normalized cases=" << label_normalized.size() << "\n";
  std::cout << "label_encoded cases=" << label_encoded.size() << "\n";
  std::cout << "label_indicators features=" << label_indicators.nfeatures() << "\n";
  std::cout << "df_cov features=" << df_cov.nfeatures() << "\n";
  std::cout << "df_corr features=" << df_corr.nfeatures() << "\n";
  std::cout << "df_corr_with cases=" << df_corr_with.size() << "\n";
  std::cout << "df_corr_many features=" << df_corr_many.nfeatures() << "\n";
  std::cout << "df_quantiles cases=" << df_quantiles.ncases() << "\n";
  std::cout << "df_modes cases=" << df_modes.ncases() << "\n";
  std::cout << "df_ranks cases=" << df_ranks.ncases() << "\n";
  std::cout << "df_var cases=" << df_var.size() << "\n";
  std::cout << "df_std cases=" << df_std.size() << "\n";
  std::cout << "df_se cases=" << df_se.size() << "\n";
  std::cout << "df_skew cases=" << df_skew.size() << "\n";
  std::cout << "df_kurt cases=" << df_kurt.size() << "\n";
  std::cout << "df_scan cases=" << df_scan.ncases() << "\n";
  std::cout << "df_moving cases=" << df_moving.ncases() << "\n";
  std::cout << "df_moving_var cases=" << df_moving_var.ncases() << "\n";
  std::cout << "df_weighted cases=" << df_weighted.ncases() << "\n";
  std::cout << "df_bounded cases=" << df_bounded.ncases() << "\n";
  std::cout << "df_growing cases=" << df_growing.ncases() << "\n";
  std::cout << "df_decay cases=" << df_decay.ncases() << "\n";
  std::cout << "df_grouped cases=" << df_grouped.size() << "\n";
  std::cout << "df_timed cases=" << df_timed.size() << "\n";
  std::cout << "cleaned_labels cases=" << cleaned_labels.ncases() << "\n";
  std::cout << "label_lengths_df cases=" << label_lengths_df.ncases() << "\n";
  std::cout << "label_tokens_df features=" << label_tokens_df.nfeatures() << "\n";
  std::cout << "label_indicators_df features=" << label_indicators_df.nfeatures() << "\n";
  std::cout << "mapped_scores cases=" << mapped_scores.ncases() << "\n";
  std::cout << "row_score cases=" << row_score.size() << "\n";
  std::cout << "expanded_cases cases=" << expanded_cases.ncases() << "\n";
  std::cout << "folded_total=" << folded_total << "\n";
  std::cout << "flowed cases=" << flowed.ncases() << "\n";
  std::cout << "df_nulls_dropped cases=" << df_nulls_dropped.ncases() << "\n";
  std::cout << "df_ndistinct values=" << df_ndistinct.size() << "\n";
  std::cout << "df_freq cases=" << df_freq.size() << "\n";
  std::cout << "df_duplicated cases=" << df_duplicated.size() << "\n";
  std::cout << "df_unique_cases cases=" << df_unique_cases.ncases() << "\n";
  std::cout << "df_renamed features=" << df_renamed.nfeatures() << "\n";
  std::cout << "df_relabelled features=" << df_relabelled.nfeatures() << "\n";
  std::cout << "product cases=" << product.ncases() << "\n";
  std::cout << "sql_result cases=" << sql_result.ncases() << "\n";
  std::cout << "atlas_mapped depth=" << atlas_mapped.depth() << "\n";
  std::cout << "atlas_moving depth=" << atlas_moving.depth() << "\n";
  std::cout << "atlas_flowed depth=" << atlas_flowed.depth() << "\n";
}
