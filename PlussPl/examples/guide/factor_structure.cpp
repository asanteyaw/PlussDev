// Which of twelve measured factors move together? A correlation matrix drawn as a heat map, reordered by clustering, with pairwise and grouped views.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace pluss::pl;

static std::string out(const char *name) {
  const char *dir = std::getenv("PLUSSMA_GUIDE_ASSETS");
  return ((dir ? std::filesystem::path(dir) : std::filesystem::path(".")) / name).string();
}


int main() {
  auto loaded = flash::load_dataset("factor_blocks");
  const auto data = loaded->frame();
  const int k = 6;
  std::vector<pluss::Series> columns;
  for (int i = 1; i <= k; ++i) columns.push_back(data.features("factor_0" + std::to_string(i)).as_series());
  auto stacked = columns[0].unsqueeze(0).unite({columns[1].unsqueeze(0), columns[2].unsqueeze(0), columns[3].unsqueeze(0), columns[4].unsqueeze(0), columns[5].unsqueeze(0)}, 0);
  auto correlation = stacked.corrcoef();
  std::cout << "correlation matrix " << correlation.size(0) << " x " << correlation.size(1) << "\n";
  std::cout << "largest off-diagonal correlation " << (correlation - pluss::series::eye(k, pluss::ScalarType::Float64)).abs().max().item<double>() << "\n";

  // Heat map of the correlation matrix: x and y are the column and row positions, z the matrix itself.
  auto axis = pluss::series::arange(0, k, pluss::ScalarType::Float64);
  auto fig = studio("factor_correlation");
  fig.mosaic(1, 2);
  fig.headline("Six factors");
  auto &heat = fig.pane2("Correlation heat map");
  flash::heatmap(heat, axis, axis, correlation);
  auto &cluster = fig.pane2("Clustered");
  flash::clustermap(cluster, axis, axis, correlation);
  fig.show();

  // Pairwise views of the first four factors, and the three-way view of factors one to three.
  auto pairs = studio("factor_pairs");
  flash::pairplot(pairs, {columns[0], columns[1], columns[2], columns[3]});
  pairs.show();
  auto tri = studio("factor_triplot");
  flash::triplot(tri, {columns[0], columns[1], columns[2]});
  tri.show();
}
