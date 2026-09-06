#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(raw, data_options);

  // restore(bundle, policy) rebuilds a Workflow from an already-loaded
  // FlowBundle plus an EXPLICIT Policy -- unlike load(path), it never reads
  // a ".policy" sidecar, so the caller decides the policy directly.
  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_workflow_make_example_6.bundle").string();
  trained.model().save(path);
  const lymph::io::FlowBundle bundle = lymph::resolve(lymph::io::load(path), "load");

  workflow::Policy policy{};
  policy.require({"income", "balance"});
  const workflow::Workflow restored = workflow::restore(bundle, policy);
  std::cout << "restored.trained() = " << std::boolalpha << restored.trained()
            << "  policy().require_columns.size() = " << restored.policy().require_columns.size() << "\n";

  std::filesystem::remove(path);

  return 0;
}
