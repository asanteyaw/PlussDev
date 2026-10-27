#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

int main() {
  using namespace pluss::ai::nodej;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  std::vector<double> xvals{};
  for (int i = 0; i < 30; ++i) {
    xvals.push_back(-3.0 + 0.1 * i);
    xvals.push_back(-2.5 + 0.05 * i);
  }
  for (int i = 0; i < 30; ++i) {
    xvals.push_back(2.0 + 0.08 * i);
    xvals.push_back(2.5 + 0.06 * i);
  }
  const FeatureBatch features{
      .x = pluss::make_series(xvals, {60, 2}, cfg),
      .layout = ObservationLayout::Rows,
  };

  ModelSpec kmeans_spec{
      .task = TaskKind::Transform,
      .family = ModelFamily::Cluster,
      .options = ClusterFamilyOptions{
          .method = ClusterMethod::KMeans,
          .clusters = 2,
          .max_iterations = 200,
          .restarts = 5,
          .tolerance = 1e-5,
      },
  };
  auto kmeans_or = make_transformer(kmeans_spec);
  if (!kmeans_or.ok()) {
    std::cerr << "make_transformer(kmeans) failed: " << kmeans_or.status().message << "\n";
    return 1;
  }
  auto kmeans = kmeans_or.move_value();
  if (!kmeans->learn(features).ok()) {
    std::cerr << "kmeans learn failed\n";
    return 1;
  }
  auto labels_or = kmeans->map(features);
  if (!labels_or.ok()) {
    std::cerr << "kmeans map failed: " << labels_or.status().message << "\n";
    return 1;
  }
  const auto labels = labels_or.value().reshape({60}).to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  std::cout << "kmeans labels (first 10): ";
  for (int i = 0; i < 10; ++i) std::cout << labels[static_cast<std::size_t>(i)] << " ";
  std::cout << "\n";

  std::vector<double> pca_vals{};
  for (int i = 0; i < 80; ++i) {
    const double x0 = -2.0 + 4.0 * static_cast<double>(i) / 79.0;
    const double x1 = 0.8 * x0 + 0.2 * std::sin(3.0 * x0);
    const double x2 = -0.5 * x0 + 0.7 * x1;
    pca_vals.push_back(x0);
    pca_vals.push_back(x1);
    pca_vals.push_back(x2);
  }
  const FeatureBatch pca_features{
      .x = pluss::make_series(pca_vals, {80, 3}, cfg),
      .layout = ObservationLayout::Rows,
  };

  ModelSpec pca_spec{
      .task = TaskKind::Transform,
      .family = ModelFamily::Decompose,
      .options = DecomposeFamilyOptions{
          .method = DecomposeMethod::Pca,
          .components = 2,
          .whiten = false,
          .max_iterations = 400,
          .tolerance = 1e-6,
      },
  };
  auto pca_or = make_transformer(pca_spec);
  if (!pca_or.ok()) {
    std::cerr << "make_transformer(pca) failed: " << pca_or.status().message << "\n";
    return 1;
  }
  auto pca = pca_or.move_value();
  if (!pca->learn(pca_features).ok()) {
    std::cerr << "pca learn failed\n";
    return 1;
  }
  auto proj_or = pca->map(pca_features);
  if (!proj_or.ok()) {
    std::cerr << "pca map failed: " << proj_or.status().message << "\n";
    return 1;
  }
  std::cout << "pca output shape: ["
            << proj_or.value().sizes()[0] << ", "
            << proj_or.value().sizes()[1] << "]\n";
  std::cout << "nodej_unsupervised_demo complete\n";
  return 0;
}

