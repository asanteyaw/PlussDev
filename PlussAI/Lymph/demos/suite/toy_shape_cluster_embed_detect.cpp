#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series axis = pluss::series::linspace(1.0, 48.0, 48, ScalarType::Float64);
  const lymph::FeatureBatch shaped{
      .x = axis.log().unite({axis.sqrt(), axis.mul(0.1).sin().add(1.0)}, 0).reshape({48, 3}),
      .layout = lymph::ObservationLayout::Rows,
  };

  const lymph::shape::Result profile = lymph::shape::data(shaped, {"log_axis", "sqrt_axis", "wave"}).run();
  lymph::Model shaped_model = lymph::models::linear_regression();
  profile.plan().apply(shaped_model);

  std::vector<double> points{};
  points.reserve(80);
  for (int i = 0; i < 20; ++i) {
    points.push_back(-2.0 + 0.02 * static_cast<double>(i));
    points.push_back(-1.0 + 0.02 * static_cast<double>(i));
  }
  for (int i = 0; i < 20; ++i) {
    points.push_back(2.0 + 0.02 * static_cast<double>(i));
    points.push_back(1.0 + 0.02 * static_cast<double>(i));
  }
  const lymph::FeatureBatch groups{
      .x = pluss::make_series(points, {40, 2}, f64),
      .layout = lymph::ObservationLayout::Rows,
  };

  lymph::ClusterFamilyOptions kmeans{};
  kmeans.clusters = 2;
  kmeans.restarts = 1;
  const lymph::cluster::Result clustered = lymph::cluster::model(lymph::models::kmeans(kmeans)).run(groups);

  lymph::DecomposeFamilyOptions pca{};
  pca.components = 1;
  const lymph::embed::Result embedded = lymph::embed::model(lymph::models::pca(pca)).run(shaped);

  lymph::prep::OutlierIsolatorOptions iso{};
  iso.estimators = 7;
  iso.max_samples_auto = false;
  iso.max_samples = 20;
  iso.contamination_auto = false;
  iso.contamination = 0.05;
  const lymph::detect::Result detected =
      lymph::detect::model(lymph::models::isolation_detector(iso)).run(groups);

  std::cout << "shape features: " << profile.feature_count() << "\n";
  std::cout << "clusters: " << clustered.cluster_count() << "\n";
  std::cout << "components: " << embedded.component_count() << "\n";
  std::cout << "flagged rate: " << detected.flagged_rate() << "\n";
  return 0;
}
