#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  workflow::Policy policy{};
  policy.require({"income", "balance", "default"});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression(), policy).learn(raw, data_options);

  // load(path) = resolve(io::load(path), ...) + restore(bundle,
  // load_policy(path)) in one call -- the everyday reload path, reading
  // both the model bundle and its Policy sidecar back in one call.
  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_workflow_make_example_7.bundle").string();
  trained.save(path);

  const workflow::Workflow loaded = workflow::load(path);
  std::cout << "loaded.trained() = " << std::boolalpha << loaded.trained()
            << "  require_columns.size() = " << loaded.policy().require_columns.size() << "\n";

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");

  return 0;
}
