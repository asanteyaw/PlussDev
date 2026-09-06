#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series num_income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8}, {10}, f64);
  const pluss::Series num_balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4}, {10}, f64);
  const pluss::Series num_tenure = pluss::make_series(
      {1.0, 2.0, 1.0, 3.0, 2.0, 4.0, 3.0, 1.0, 2.0, 3.0}, {10}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const DataFrame frame({{"num_income", Thread(num_income)},
                         {"num_balance", Thread(num_balance)},
                         {"num_tenure", Thread(num_tenure)},
                         {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"num_income", "num_balance", "num_tenure"};
  data_options.label_feature = "target";

  // A predicate route matches columns by (name, dtype) instead of an explicit
  // name list, so it keeps matching as new "num_*" columns are added to the
  // frame without touching the Workflow definition.
  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.on([](std::string_view name, std::string_view) { return name.rfind("num_", 0) == 0; })
      .add(lymph::prep::stage::standard_scaler());

  const workflow::Workflow trained = wf.learn(frame, data_options);
  std::cout << "branch_count = " << trained.branch_count() << "\n";
  const auto& b = trained.branches().front();
  std::cout << "branch name=\"" << b.name << "\" named=" << std::boolalpha << b.named
            << " stage_count=" << b.stage_count << "\n";

  const pluss::Series pred = trained.infer(frame, data_options);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
