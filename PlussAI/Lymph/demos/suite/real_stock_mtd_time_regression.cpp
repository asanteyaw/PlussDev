#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  const DataFrame raw = DataFrame::load_csv((root / "data/mtd_prices.csv").string());

  lymph::workflow::Payload data = lymph::workflow::payload::make(raw)
      .features({"open", "high", "low", "cum_change", "cma_7", "sma_7", "ema_14", "atr", "year"})
      .label("close")
      .time("date");

  lymph::TimeOptions time{};
  time.index = "date";
  time.gap = 1;
  time.horizon = 20;
  time.lookback = 180;

  lymph::Workflow wf = lymph::workflow::make(
      lymph::workflow::Policy{.require_columns = {"date", "close"}, .reject_missing = {"date", "close"}},
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("market", {"open", "high", "low", "cum_change", "cma_7", "sma_7", "ema_14", "atr", "year"}, {
              {"fill", lymph::prep::stage::simple_imputer()},
              {"scale", lymph::prep::stage::standard_scaler()},
          }),
      },
      lymph::models::random_forest_regressor({.learners = 80, .subsample = 0.9}));
  wf.time(time);

  const auto cv = lymph::selection::validate::time().folds(4).gap(1).horizon(20).lookback(180).run(wf, data);
  wf.learn_(data);
  std::cout << "mtd folds: " << cv.report.folds.size() << "\n";
  std::cout << "mtd r2: " << wf.assess(data).r2.item<double>() << "\n";
  return 0;
}
