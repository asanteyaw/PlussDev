#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path project = "/Users/yawasante/Downloads/stock_forecasting";
  const DataFrame raw = DataFrame::load_csv((project / "mtd_prices.csv").string());

  lymph::Payload data = lymph::payload::make(raw)
      .features({"open", "high", "low", "cum_change", "sma_7", "ema_14", "atr", "year"})
      .label("close")
      .time("date");

  lymph::Workflow wf = lymph::workflow::make(
      lymph::Policy{.require_columns = {"date", "close"}, .reject_missing = {"date", "close"}},
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("price", {"open", "high", "low", "cum_change", "sma_7", "ema_14", "atr", "year"}, {
              {"fill", lymph::stage::simple_imputer()},
              {"scale", lymph::stage::standard_scaler()},
          }),
      },
      lymph::models::linear_regression({.regularization_strength = 0.001}));

  lymph::TimeOptions time{};
  time.index = "date";
  time.gap = 1;
  time.horizon = 20;
  time.lookback = 180;
  wf.time(time);

  const lymph::CrossValidationReport cv = lymph::validate::time().folds(4).gap(1).horizon(20).lookback(180).run(wf, data);
  wf.learn_(data);
  std::cout << "stock-project folds: " << cv.report.folds.size() << "\n";
  std::cout << "stock-project rmse: " << wf.assess(data).rmse.item<double>() << "\n";
  return 0;
}
