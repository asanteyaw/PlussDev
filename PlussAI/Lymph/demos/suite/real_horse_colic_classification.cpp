#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  std::vector<std::string> names{};
  names.reserve(28);
  for (int i = 0; i < 27; ++i) names.push_back("col_" + std::to_string(i));
  names.push_back("outcome");

  pluss::loader::ReadCsvOptions csv{};
  csv.header = false;
  csv.names = names;
  const DataFrame raw = DataFrame::load_csv((root / "data/horse_colic.csv").string(), csv);

  std::vector<std::string> features(names.begin(), names.begin() + 27);
  lymph::Payload data = lymph::payload::make(raw)
      .features(features)
      .label("outcome")
      .note(lymph::DataNotePolicy::Factorize);

  lymph::Workflow wf = lymph::workflow::make(lymph::models::decision_tree_classifier({.max_depth = 6, .min_samples_leaf = 3}));
  wf.learn_(data);
  std::cout << "horse-colic accuracy: " << wf.assess(data).accuracy.item<double>() << "\n";
  return 0;
}
