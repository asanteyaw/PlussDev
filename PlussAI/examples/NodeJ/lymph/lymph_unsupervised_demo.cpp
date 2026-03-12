#include <pluss/lymph.h>

#include <iostream>
#include <vector>

int main() {
  using namespace pluss::ai::nodej;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const Series x = pluss::make_series(std::vector<double>{
                                          -2.0, -1.0,
                                          -1.8, -0.9,
                                          -1.6, -0.8,
                                           1.2,  1.1,
                                           1.4,  1.2,
                                           1.6,  1.3},
                                      {6, 2},
                                      cfg);
  const FeatureBatch batch{
      .x = x,
      .layout = ObservationLayout::Rows,
  };

  auto kmeans_or = pluss::ai::lymph::quick::kmeans_clusterer(batch, 2);
  if (!kmeans_or.ok()) {
    std::cerr << "lymph::kmeans_clusterer failed: " << kmeans_or.status().message << "\n";
    return 1;
  }
  auto labels_or = kmeans_or.value()->map(batch);
  if (!labels_or.ok()) {
    std::cerr << "kmeans map failed: " << labels_or.status().message << "\n";
    return 1;
  }
  std::cout << "lymph kmeans labels: ";
  for (double v : labels_or.value().reshape({6}).to(pluss::pCPU, pluss::ScalarType::Float64).to_vec<double>()) {
    std::cout << v << " ";
  }
  std::cout << "\n";

  std::vector<double> pca_vals{};
  for (int i = 0; i < 40; ++i) {
    const double x0 = -2.0 + 4.0 * static_cast<double>(i) / 39.0;
    const double x1 = 0.7 * x0;
    const double x2 = -0.5 * x0 + 0.8 * x1;
    pca_vals.push_back(x0);
    pca_vals.push_back(x1);
    pca_vals.push_back(x2);
  }
  const FeatureBatch pca_batch{
      .x = pluss::make_series(pca_vals, {40, 3}, cfg),
      .layout = ObservationLayout::Rows,
  };
  auto pca_or = pluss::ai::lymph::quick::pca_mapper(pca_batch, 2, false);
  if (!pca_or.ok()) {
    std::cerr << "lymph::pca_mapper failed: " << pca_or.status().message << "\n";
    return 1;
  }
  auto mapped_or = pca_or.value()->map(pca_batch);
  if (!mapped_or.ok()) {
    std::cerr << "pca map failed: " << mapped_or.status().message << "\n";
    return 1;
  }
  std::cout << "lymph pca output shape: ["
            << mapped_or.value().sizes()[0] << ", "
            << mapped_or.value().sizes()[1] << "]\n";
  std::cout << "lymph_unsupervised_demo complete\n";
  return 0;
}

