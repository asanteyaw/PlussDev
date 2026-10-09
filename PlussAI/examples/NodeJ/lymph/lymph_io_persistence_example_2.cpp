#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

// Model artifact vs. Workflow artifact: the same trained model, saved both
// ways. The Model bundle restores the learned pipeline; the Workflow bundle
// additionally restores the Policy admission contract.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"id", Slate(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  workflow::Policy policy{};
  policy.drop({"id"});

  const workflow::Workflow wf =
      workflow::make(lymph::models::logistic_regression(), policy).learn(raw, data_options);

  const std::string model_path =
      (std::filesystem::temp_directory_path() / "lymph_io_persistence_example_2_model.flow").string();
  const std::string wf_path =
      (std::filesystem::temp_directory_path() / "lymph_io_persistence_example_2_wf.flow").string();

  wf.model().save(model_path);   // just the learned pipeline
  wf.save(wf_path);              // pipeline + Policy sidecar

  const lymph::models::Model loaded_model = lymph::models::Model::load(model_path);
  const workflow::Workflow loaded_wf = workflow::load(wf_path);

  std::cout << "loaded_model.trained()          = " << std::boolalpha << loaded_model.trained() << "\n";
  std::cout << "loaded_wf.trained()             = " << loaded_wf.trained() << "\n";
  std::cout << "loaded_wf.policy().drop_columns = " << loaded_wf.policy().drop_columns.size()
            << " (Model bundle has no Policy concept)\n";

  std::filesystem::remove(model_path);
  std::filesystem::remove(wf_path);
  std::filesystem::remove(wf_path + ".policy");
  return 0;
}
