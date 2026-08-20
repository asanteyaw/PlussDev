#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  std::vector<std::string> names{};
  names.reserve(61);
  for (int i = 0; i < 60; ++i) names.push_back("band_" + std::to_string(i));
  names.push_back("rock_or_mine");

  pluss::loader::ReadCsvOptions csv{};
  csv.header = false;
  csv.names = names;
  const DataFrame raw = DataFrame::load_csv((root / "data/sonar.csv").string(), csv);

  std::vector<std::string> features(names.begin(), names.begin() + 60);
  lymph::DataFrameBatchOptions data{};
  data.feature_names = features;

  lymph::DecomposeFamilyOptions pca{};
  pca.components = 3;
  const lymph::embed::Result embedded = lymph::embed::model(lymph::model::pca(pca)).run(raw, data);

  lymph::ClusterFamilyOptions kmeans{};
  kmeans.clusters = 2;
  kmeans.restarts = 2;
  const lymph::cluster::Result clustered = lymph::cluster::model(lymph::model::kmeans(kmeans)).run(raw, data);

  std::cout << "sonar components: " << embedded.component_count() << "\n";
  std::cout << "sonar clusters: " << clustered.cluster_count() << "\n";
  return 0;
}
