#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const Series time = pluss::series::linspace(0.0, 119.0, 120, ScalarType::Float64);
  const Series temp = time.mul(0.05).sin().mul(12.0).add(22.0);
  const Series load = temp.mul(2.0).add(time.mul(0.03)).add(time.mul(0.17).cos());
  const DataFrame raw({{"day", Slate(time)}, {"temp", Slate(temp)}, {"load", Slate(load)}});

  lymph::workflow::Payload data = lymph::workflow::payload::make(raw)
      .features({"temp"})
      .label("load")
      .time("day");

  lymph::TimeOptions time_rules{};
  time_rules.index = "day";
  time_rules.gap = 2;
  time_rules.horizon = 12;
  time_rules.lookback = 48;

  lymph::Workflow wf = lymph::workflow::make(
      lymph::workflow::Policy{.require_columns = {"day", "temp", "load"}, .reject_missing = {"day", "load"}},
      std::vector<lymph::WorkflowBranchSpec>{
          lymph::workflow::branch("num", {"temp"}, {
              {"scale", lymph::prep::stage::standard_scaler()},
          }),
      },
      lymph::models::gradient_boost_regressor({.learners = 40, .learning_rate = 0.05}));
  wf.time(time_rules);

  auto split = lymph::selection::validate::time().folds(3).gap(2).horizon(12).lookback(48);
  const auto cv = split.run(wf, data, {}, {});
  wf.learn_(data);

  std::cout << "time folds: " << cv.report.folds.size() << "\n";
  std::cout << "time-aware: " << wf.time_aware() << "\n";
  return 0;
}
