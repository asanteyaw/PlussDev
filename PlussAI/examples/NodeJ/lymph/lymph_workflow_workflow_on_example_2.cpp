#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8}, {10}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4}, {10}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const DataFrame frame({{"income", Slate(income)}, {"balance", Slate(balance)},
                         {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  // `on(name, names)` and the two-call `branch(name).route(names)` spelling
  // build the exact same branch -- pick whichever reads better at the call
  // site. Here two otherwise-identical Workflows are built one each way and
  // trained on the same frame to show they produce identical predictions.
  workflow::Workflow one_call = workflow::make(lymph::models::logistic_regression());
  one_call.on("numeric", {"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  workflow::Workflow two_call = workflow::make(lymph::models::logistic_regression());
  two_call.branch("numeric").route({"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  const workflow::Workflow trained_one = one_call.learn(frame, data_options);
  const workflow::Workflow trained_two = two_call.learn(frame, data_options);

  const pluss::Series pred_one = trained_one.infer(frame, data_options);
  const pluss::Series pred_two = trained_two.infer(frame, data_options);

  std::cout << "one_call branch name=\"" << trained_one.branches().front().name
            << "\" stage_count=" << trained_one.branches().front().stage_count << "\n";
  std::cout << "two_call branch name=\"" << trained_two.branches().front().name
            << "\" stage_count=" << trained_two.branches().front().stage_count << "\n";
  std::cout << "pred_one = " << pred_one << "\n";
  std::cout << "pred_two = " << pred_two << "\n";

  return 0;
}
