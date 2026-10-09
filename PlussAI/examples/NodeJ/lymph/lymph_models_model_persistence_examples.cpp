#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

namespace {
namespace lymph = pluss::ai::lymph;
using pluss::ScalarType;
using pluss::table::DataFrame;
using pluss::table::Slate;

// BEGIN SAVE 1 / LOAD 1
void persist_a_trained_model() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({{"score", Slate(pluss::make_series({1, 2, 8, 9}, {4}, cfg))}});
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, cfg);
  const auto model = lymph::models::tree::decision_tree_classifier({.max_depth = 2}).learn(x, y);
  const std::filesystem::path path = "/tmp/pluss_lymph_persist_trained.flow";
  model.save(path.string());
  const auto restored = lymph::models::Model::load(path.string());
  std::cout << "persist_trained saved=" << std::boolalpha << restored.trained()
            << " predictions=" << restored.infer(x).size().item<std::int64_t>() << '\n';
  std::filesystem::remove(path);
}
// END SAVE 1 / LOAD 1

// BEGIN SAVE 2 / LOAD 2
void fit_and_persist_from_separate_xy() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame x({{"balance", Slate(pluss::make_series({10, 20, 800, 900}, {4}, cfg))}});
  const auto y = pluss::make_series({0, 0, 1, 1}, {4}, cfg);
  const auto template_model = lymph::models::tree::decision_tree_classifier({.max_depth = 2});
  const std::filesystem::path path = "/tmp/pluss_lymph_persist_xy.flow";
  template_model.save(path.string(), x, y);
  const auto restored = lymph::models::Model::load(path.string());
  std::cout << "persist_xy saved=" << std::boolalpha << restored.trained()
            << " predictions=" << restored.infer(x).size().item<std::int64_t>() << '\n';
  std::filesystem::remove(path);
}
// END SAVE 2 / LOAD 2

// BEGIN SAVE 3 / LOAD 3
void fit_and_persist_from_a_dataframe_contract() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame rows({{"balance", Slate(pluss::make_series({10, 20, 800, 900}, {4}, cfg))},
                        {"defaulted", Slate(pluss::make_series({0, 0, 1, 1}, {4}, cfg))}});
  lymph::options::DFBatch contract{};
  contract.feature_names = {"balance"};
  contract.label_feature = "defaulted";
  const auto template_model = lymph::models::tree::decision_tree_classifier({.max_depth = 2});
  const std::filesystem::path path = "/tmp/pluss_lymph_persist_frame.flow";
  template_model.save(path.string(), rows, contract);
  const auto restored = lymph::models::Model::load(path.string());
  std::cout << "persist_frame saved=" << std::boolalpha << restored.trained()
            << " predictions=" << restored.infer(rows, contract).size().item<std::int64_t>() << '\n';
  std::filesystem::remove(path);
}
// END SAVE 3 / LOAD 3

}  // namespace

int main() {
  persist_a_trained_model();
  fit_and_persist_from_separate_xy();
  fit_and_persist_from_a_dataframe_contract();
  return 0;
}
