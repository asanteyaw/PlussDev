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

  lymph::prep::OutlierIsolatorOptions iso{};
  iso.estimators = 25;
  iso.max_samples_auto = false;
  iso.max_samples = 128;
  iso.contamination_auto = false;
  iso.contamination = 0.04;
  iso.random.seed = 20260811ULL;
  iso.random.deterministic = true;

  const lymph::detect::Result result =
      lymph::detect::model(lymph::models::isolation_detector(iso)).run(raw, data);

  std::cout << "oil-spill flagged: " << result.flagged_count() << "\n";
  std::cout << "oil-spill rate: " << result.flagged_rate() << "\n";
  return 0;
}
