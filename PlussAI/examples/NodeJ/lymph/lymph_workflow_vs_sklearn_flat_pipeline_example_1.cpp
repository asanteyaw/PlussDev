#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The lymph equivalent of sklearn's flat Pipeline([("imputer", ...),
// ("scaler", ...), ("model", LogisticRegression())]) -- one chain of steps
// over every feature column, no per-column routing needed. There is no
// separate "flat" constructor; route::remaining_of({}) (the complement of an
// empty name list) matches every column, so one branch on all of them plays
// the same role a bare, unrouted Pipeline step list would in sklearn.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64))},
      {"balance", Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}, {6}, f64))},
      {"target", Thread(pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.on(lymph::prep::route::remaining_of(std::vector<std::string>{}))
      .add(lymph::prep::stage::simple_imputer())
      .add(lymph::prep::stage::standard_scaler());

  const workflow::Workflow trained = wf.learn(raw, data_options);
  std::cout << "branch_count = " << trained.branch_count()
            << "  stage_count = " << trained.stage_count() << "\n";
  std::cout << "pred = " << trained.infer(raw, data_options) << "\n";

  return 0;
}
