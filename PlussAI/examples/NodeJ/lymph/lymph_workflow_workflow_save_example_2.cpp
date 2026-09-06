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
  });
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);

  const workflow::Payload payload = workflow::payload::make(raw).features({"income", "balance"}).label(y);
  const workflow::Workflow trained = workflow::make(lymph::models::logistic_regression()).learn(payload);

  lymph::io::FlowBundleMetadata metadata{};
  metadata.producer = "lymph_workflow_save_example_2";
  metadata.producer_version = "0.1";

  // save(path, payload, train_options, metadata) embeds the Payload's data
  // (run through Policy, same as learn(payload) would) into the bundle
  // alongside the fitted model -- a self-describing archive that records
  // what the model was trained from, not just its learned parameters.
  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_workflow_save_example_2.bundle").string();
  trained.save(path, payload, lymph::options::Train{}, metadata);

  const workflow::Workflow loaded = workflow::load(path);
  std::cout << "loaded.trained() = " << std::boolalpha << loaded.trained() << "\n";
  std::cout << "pred = " << loaded.infer(payload) << "\n";

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");

  return 0;
}
