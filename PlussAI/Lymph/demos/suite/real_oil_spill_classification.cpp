#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  std::vector<std::string> names{};
  names.reserve(50);
  for (int i = 0; i < 49; ++i) names.push_back("sensor_" + std::to_string(i));
  names.push_back("spill");

  pluss::loader::ReadCsvOptions csv{};
  csv.header = false;
  csv.names = names;
  const DataFrame raw = DataFrame::load_csv((root / "data/oil_spill.csv").string(), csv);

  std::vector<std::string> features(names.begin(), names.begin() + 49);
  lymph::DataFrameBatchOptions data{};
  data.feature_names = features;
  data.label_feature = "spill";

  lymph::Workflow wf = lymph::workflow::make(lymph::model::random_forest_classifier({.learners = 60, .subsample = 0.9}));
  wf.learn_(raw, data);
  std::cout << "oil-spill classifier f1: " << wf.assess(raw, data).f1.item<double>() << "\n";
  return 0;
}
