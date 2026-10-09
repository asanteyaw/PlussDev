#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  const DataFrame raw = DataFrame::load_csv((root / "data/sp500_data.csv").string());

  lymph::workflow::Payload data = lymph::workflow::payload::make(raw)
      .features({"Open", "High", "Low", "Volume"})
      .label("Close")
      .time("Date");

  lymph::TimeOptions time{};
  time.index = "Date";
  time.gap = 1;
  time.horizon = 15;
  time.lookback = 180;

  lymph::Workflow wf = lymph::workflow::make(
      lymph::workflow::Policy{.require_columns = {"Date", "Close"}, .reject_missing = {"Date", "Close"}},
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("market", {"Open", "High", "Low", "Volume"}, {
              {"fill", lymph::prep::stage::simple_imputer()},
              {"scale", lymph::prep::stage::standard_scaler()},
          }),
      },
      lymph::models::gradient_boost_regressor({.learners = 60, .learning_rate = 0.04}));
  wf.time(time);

  auto cv = lymph::selection::validate::time().folds(4).gap(1).horizon(15).lookback(180).run(wf, data);
  wf.learn_(data);
  std::cout << "sp500 folds: " << cv.report.folds.size() << "\n";
  std::cout << "sp500 rmse: " << wf.assess(data).rmse.item<double>() << "\n";
  return 0;
}
