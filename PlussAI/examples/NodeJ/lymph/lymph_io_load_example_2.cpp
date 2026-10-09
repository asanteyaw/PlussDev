#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

// workflow::load(path) restores a Workflow: the trained model plus its
// Policy sidecar. The reloaded object still strips the same raw column on
// every verb.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"customer_id", Slate(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_load_example_2.flow").string();
  {
    workflow::Policy policy{};
    policy.drop({"customer_id"});
    workflow::make(lymph::models::logistic_regression(), policy)
        .learn(raw, data_options)
        .save(path);
  }

  const workflow::Workflow loaded = workflow::load(path);
  std::cout << "loaded.trained()               = " << std::boolalpha << loaded.trained() << "\n";
  std::cout << "loaded.policy().drop_columns.size() = " << loaded.policy().drop_columns.size() << "\n";

  // Incoming rows still carry customer_id -- the restored Policy strips it.
  const DataFrame incoming({
      {"customer_id", Slate(pluss::make_series({9, 10, 11}, {3}, f64))},
      {"income", Slate(pluss::make_series({1.2, 4.2, 1.6}, {3}, f64))},
      {"balance", Slate(pluss::make_series({0.3, 2.2, 0.5}, {3}, f64))},
  });
  std::cout << "pred = " << loaded.infer(incoming, data_options) << "\n";

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");
  return 0;
}
