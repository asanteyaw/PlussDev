#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

// workflow::Workflow::save(path) writes two files: the model bundle at
// `path`, and a Policy sidecar at `path + ".policy"`. workflow::load reads
// both back, so the reloaded Workflow applies the identical admission
// contract.
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
      (std::filesystem::temp_directory_path() / "lymph_io_save_example_2.flow").string();
  trained.save(path);

  std::cout << "bundle written  = " << std::boolalpha << std::filesystem::exists(path) << "\n";
  std::cout << "sidecar written = " << std::filesystem::exists(path + ".policy") << "\n";

  const workflow::Workflow loaded = workflow::load(path);
  std::cout << "loaded.policy().drop_columns[0] = " << loaded.policy().drop_columns.front() << "\n";
  std::cout << "original = " << trained.infer(raw, data_options) << "\n";
  std::cout << "loaded   = " << loaded.infer(raw, data_options) << "\n";

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");
  return 0;
}
