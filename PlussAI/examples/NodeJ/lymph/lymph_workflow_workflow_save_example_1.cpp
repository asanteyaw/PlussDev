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
      {"customer_id", Thread(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression(), policy).learn(raw, data_options);

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_workflow_save_example_1.bundle").string();
  trained.save(path);

  // load reconstructs both the Model bundle and the Policy sidecar
  // (path + ".policy") -- the reloaded Workflow drops "customer_id" exactly
  // as the original did.
  const workflow::Workflow loaded = workflow::load(path);
  std::cout << "loaded.policy().drop_columns == original: "
            << std::boolalpha << (loaded.policy().drop_columns == policy.drop_columns) << "\n";

  const pluss::Series original_pred = trained.infer(raw, data_options);
  const pluss::Series loaded_pred = loaded.infer(raw, data_options);
  std::cout << "original_pred = " << original_pred << "\n";
  std::cout << "loaded_pred   = " << loaded_pred << "\n";

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");

  return 0;
}
