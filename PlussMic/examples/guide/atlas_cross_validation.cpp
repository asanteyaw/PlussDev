// k-fold cross-validation with an Atlas: equal folds held together, each fold taken in turn as the test set while a line is fitted
// on the others, and the errors combined into one honest score.
#include <pluss/pluss.h>

#include <iostream>

using pluss::ScalarType;
using pluss::table::Atlas;
using pluss::table::DataFrame;
using pluss::table::Shelf;

int main() {
  // Twelve observations y = 2 + 0.5 x with noise; shuffled by a fixed seed so folds mix early and late x values.
  const std::int64_t n = 12, folds = 4, per_fold = n / folds;
  auto x = pluss::series::arange(0, n, ScalarType::Float64);
  auto y = x.mul(0.5).add(2.0).add(pluss::series::randn({n}, ScalarType::Float64, 3).mul(0.4));
  auto order = pluss::series::randperm(n, ScalarType::Int64, 9);
  auto data = DataFrame({{"x", x.index_select(0, order)}, {"y", y.index_select(0, order)}});

  // 1. An Atlas needs frames of identical shape: four folds of three rows each.
  std::vector<DataFrame> parts;
  for (std::int64_t k = 0; k < folds; ++k) parts.push_back(data.slice(k * per_fold, (k + 1) * per_fold));
  Atlas atlas(parts, Shelf(pluss::make_note({"fold0", "fold1", "fold2", "fold3"})));
  std::cout << "folds " << atlas.depth() << " of " << atlas.ncases() << " rows each\n";

  // 2. For each fold: fit y = a + b x on the other folds, predict the held-out fold, record the mean absolute error.
  std::vector<double> scores;
  for (std::int64_t k = 0; k < folds; ++k) {
    std::vector<pluss::Series> train_x, train_y;
    for (std::int64_t j = 0; j < folds; ++j) {
      if (j == k) continue;
      train_x.push_back(atlas.frame(j).features("x").as_series());
      train_y.push_back(atlas.frame(j).features("y").as_series());
    }
    auto tx = train_x[0].unite({train_x[1], train_x[2]}, 0);
    auto ty = train_y[0].unite({train_y[1], train_y[2]}, 0);
    auto design = pluss::series::ones({tx.numel(), 1}, ScalarType::Float64).unite({tx.unsqueeze(1)}, 1);
    auto beta = design.t().matmul(design).linearalg().inverse().matmul(design.t().matmul(ty.unsqueeze(1))).squeeze(1);
    auto hx = atlas.frame(k).features("x").as_series();
    auto hy = atlas.frame(k).features("y").as_series();
    auto prediction = hx.mul(beta.select(0, 1)).add(beta.select(0, 0));
    const double mae = prediction.sub(hy).abs().mean().item<double>();
    scores.push_back(mae);
    std::cout << "held out fold" << k << ": intercept " << beta.select(0, 0).item<double>() << ", slope " << beta.select(0, 1).item<double>() << ", MAE " << mae << "\n";
  }

  // 3. The cross-validated score is the mean of the fold errors; the spread says how much to trust it.
  auto s = pluss::make_series(scores);
  std::cout << "cross-validated MAE " << s.mean().item<double>() << " +/- " << s.std().item<double>() << " (noise level 0.4)\n";
}
