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
                         {"category", Slate(pluss::make_note(std::vector<std::string>{
                             "a", "b", "a", "b", "a", "b", "a", "b", "a", "b"}))},
                         {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance", "category"};
  data_options.label_feature = "target";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());
  wf.on("category", {"category"}).add(lymph::prep::stage::one_hot_encoder());

  const workflow::Workflow trained = wf.learn(frame, data_options);
  std::cout << "branch_count = " << trained.branch_count() << "\n";
  for (const auto& b : trained.branches()) {
    std::cout << "  branch name=\"" << b.name << "\" named=" << std::boolalpha << b.named
              << " stage_count=" << b.stage_count << "\n";
  }
  std::cout << "stage_count = " << trained.stage_count() << "\n";

  const pluss::Series pred = trained.infer(frame, data_options);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
