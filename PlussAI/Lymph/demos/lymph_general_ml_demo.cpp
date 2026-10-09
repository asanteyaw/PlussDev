#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <cmath>
#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  std::vector<double> shaped_values{};
  shaped_values.reserve(36);
  for (int i = 0; i < 12; ++i) {
    const double t = static_cast<double>(i + 1);
    shaped_values.push_back(std::exp(t / 8.0));
    shaped_values.push_back((i % 2 == 0) ? 0.0 : t);
    shaped_values.push_back(1.0);
  }
  const lymph::FeatureBatch shaped{
      .x = pluss::make_series(shaped_values, {12, 3}, f64),
      .layout = lymph::ObservationLayout::Rows,
  };

  const lymph::shape::Result profile = lymph::shape::data(shaped, {"skewed", "mixed", "constant"}).run({
      .skew_threshold = 0.2,
      .heavy_tail_threshold = 0.5,
  });
  lymph::models::Model shaped_model = lymph::models::logistic_regression({.fit_intercept = true});
  profile.plan({.skew_threshold = 0.2, .heavy_tail_threshold = 0.5}).apply(shaped_model);

  std::vector<double> two_group_values{};
  two_group_values.reserve(32);
  for (int i = 0; i < 8; ++i) {
    two_group_values.push_back(-2.0 + 0.025 * static_cast<double>(i));
    two_group_values.push_back(-1.0 + 0.015 * static_cast<double>(i));
  }
  for (int i = 0; i < 8; ++i) {
    two_group_values.push_back(2.0 + 0.025 * static_cast<double>(i));
    two_group_values.push_back(1.0 + 0.015 * static_cast<double>(i));
  }
  const lymph::FeatureBatch two_groups{
      .x = pluss::make_series(two_group_values, {16, 2}, f64),
      .layout = lymph::ObservationLayout::Rows,
  };

  lymph::ClusterFamilyOptions cluster_options{};
  cluster_options.clusters = 2;
  cluster_options.restarts = 1;
  lymph::TrainOptions seeded{};
  seeded.random.seed = 31;
  seeded.random.deterministic = true;
  const lymph::models::cluster::Result grouped = lymph::models::cluster::model(lymph::models::kmeans(cluster_options)).run(two_groups, seeded);

  lymph::DecomposeFamilyOptions pca_options{};
  pca_options.components = 1;
  const lymph::models::embed::Result embedded = lymph::models::embed::model(lymph::models::pca(pca_options)).run(shaped);

  lymph::prep::OutlierIsolatorOptions detector_options{};
  detector_options.estimators = 5;
  detector_options.max_samples_auto = false;
  detector_options.max_samples = 10;
  detector_options.contamination_auto = false;
  detector_options.contamination = 0.08;
  detector_options.random.seed = 37;
  detector_options.random.deterministic = true;
  const lymph::models::detect::Result flagged =
      lymph::models::detect::model(lymph::models::isolation_detector(detector_options)).run(two_groups);

  std::vector<double> labels(16, -1.0);
  labels[0] = 0.0;
  labels[1] = 0.0;
  labels[8] = 1.0;
  labels[9] = 1.0;
  lymph::models::semi::Options semi_options{};
  semi_options.select = lymph::models::semi::SelectKind::TopK;
  semi_options.top_k = 4;
  semi_options.max_rounds = 1;
  const lymph::models::semi::Result pseudo =
      lymph::models::semi::model(lymph::models::gaussian_nb_classifier())
          .run(lymph::LabeledBatch{
                   .features = two_groups,
                   .y = pluss::make_series(labels, {16}, f64),
               },
               semi_options);

  std::cout << "profile features: " << profile.feature_count() << "\n";
  std::cout << "shape plan features: " << profile.plan({.skew_threshold = 0.2, .heavy_tail_threshold = 0.5}).feature_count() << "\n";
  std::cout << "clusters: " << grouped.cluster_count() << "\n";
  std::cout << "components: " << embedded.component_count() << "\n";
  std::cout << "flagged: " << flagged.flagged_count() << "\n";
  std::cout << "pseudo labels: " << pseudo.pseudo_labeled_count() << "\n";
  return 0;
}
