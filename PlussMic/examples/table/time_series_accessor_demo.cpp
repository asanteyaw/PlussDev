#include <pluss/mic.h>

#include <array>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

using pluss::table::DataFrame;
using pluss::table::Thread;
using pluss::table::TsAlignMode;
using pluss::table::TsOptions;
using pluss::table::TsReindexFill;

int main() {
  const std::vector<std::string_view> iso{
      "2024-01-01T00:00:00Z",
      "2024-01-01T12:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-02T12:00:00Z",
      "2024-01-03T00:00:00Z",
      "2024-01-03T12:00:00Z",
  };
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(iso.size())};
  const auto t = pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(iso.data(), iso.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));

  DataFrame df({
      {"date", Thread(t)},
      {"adj_close", Thread(pluss::make_series(std::vector<double>{100.0, 102.0, 101.0, 105.0, 110.0, 112.0}))},
      {"volume", Thread(pluss::make_series(std::vector<double>{10.0, 11.0, 13.0, 12.0, 15.0, 17.0}))},
  });
  const auto ts_opts = TsOptions{.axis = df.features("date")};

  // Basic TS transforms.
  auto lr = df.features({"adj_close"}).ts().log_returns();
  auto roll = df.features({"adj_close"}).ts().rolling(3, pluss::table::WindowStatistic::Std, {.min_count = 2});
  auto exp = df.features({"adj_close"}).ts().expanding(pluss::table::WindowStatistic::Mean, {.min_count = 2});

  // Model-ready feature blocks.
  auto lag_block = df.features({"adj_close", "volume"}).ts().lag_block({1, 2, 3});
  auto roll_block = df.features({"adj_close", "volume"}).ts().rolling_block({2, 3});
  auto cal = df.features({"adj_close"}).ts(ts_opts).calendar();

  // Time-axis utilities: regularization + alignment/reindex.
  auto daily = df.features({"adj_close", "volume"}).ts(ts_opts).resample("D", pluss::subsp::stats::ts::ResampleOp::Mean);
  auto dense = df.features({"adj_close"}).ts(ts_opts).insert_gaps("H", 6);

  const std::vector<std::string_view> target_iso{
      "2024-01-01T00:00:00Z",
      "2024-01-01T06:00:00Z",
      "2024-01-01T12:00:00Z",
      "2024-01-01T18:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-02T06:00:00Z",
      "2024-01-02T12:00:00Z",
      "2024-01-02T18:00:00Z",
      "2024-01-03T00:00:00Z",
      "2024-01-03T06:00:00Z",
      "2024-01-03T12:00:00Z",
  };
  const std::array<std::int64_t, 1> target_shape{static_cast<std::int64_t>(target_iso.size())};
  const auto target_t = pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(target_iso.data(), target_iso.size()),
      std::span<const std::int64_t>(target_shape.data(), target_shape.size()));
  auto aligned = df.features({"adj_close"}).ts(ts_opts).align_to(Thread(target_t), TsAlignMode::Nearest);
  auto reindexed = df.features({"adj_close"}).ts(ts_opts).align_to(Thread(target_t), TsAlignMode::Exact, TsReindexFill::Forward);

  // One-liner assignment pattern.
  df.set("log_returns") = df.features({"adj_close"}).ts().log_returns();

  std::cout << "lr columns=" << lr.nfeatures() << ", rows=" << lr.ncases() << "\n";
  std::cout << "roll columns=" << roll.nfeatures() << ", rows=" << roll.ncases() << "\n";
  std::cout << "exp columns=" << exp.nfeatures() << ", rows=" << exp.ncases() << "\n";
  std::cout << "lag_block columns=" << lag_block.nfeatures() << ", rows=" << lag_block.ncases() << "\n";
  std::cout << "roll_block columns=" << roll_block.nfeatures() << ", rows=" << roll_block.ncases() << "\n";
  std::cout << "calendar columns=" << cal.nfeatures() << ", rows=" << cal.ncases() << "\n";
  std::cout << "daily rows=" << daily.ncases() << ", dense rows=" << dense.ncases() << "\n";
  std::cout << "aligned rows=" << aligned.ncases() << ", reindexed rows=" << reindexed.ncases() << "\n";
  std::cout << "df has log_returns=" << (df.get("log_returns").has_value() ? "true" : "false") << "\n";
  return 0;
}
