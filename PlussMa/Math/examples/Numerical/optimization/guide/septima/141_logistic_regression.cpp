// A model fitted to data, the usual job for a first-order solver. Two hundred points in the plane carry labels +1 or -1 given by
// which side of the line 1.5 x0 - x1 + 0.3 = 0 they fall on, with 10% of the labels flipped so that the classes overlap. The model
// is logistic regression with a small L2 penalty: minimize the mean of log(1 + exp(-y (x.w + b))) plus 0.05 |w|^2. The data are
// a `Series` held fixed inside the objective, the parameters are a weight column and a bias, and the objective is a few lines of
// `pluss::Series` arithmetic. All three optimizers are used; the problem is convex, so they must agree, and the program reports how far
// apart their answers are, the fraction of points the model classifies correctly, and the iterations and evaluations each used.
#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using Pack = optim::ParameterPack<pluss::Series>;
using Block = optim::ParameterPack<pluss::Series>::Block;

const auto f64 = pluss::ScalarType::Float64;

int main() {
  const int n = 200;
  std::uint64_t state = 2024;
  const auto uniform = [&] { state = state * 6364136223846793005ULL + 1442695040888963407ULL; return ((state >> 11) + 0.5) / 9007199254740992.0; };
  std::vector<double> features, labels;
  for (int i = 0; i < n; ++i) {
    const double x0 = 4.0 * uniform() - 2.0, x1 = 4.0 * uniform() - 2.0;
    double label = 1.5 * x0 - x1 + 0.3 > 0.0 ? 1.0 : -1.0;
    if (uniform() < 0.10) label = -label;
    features.push_back(x0);
    features.push_back(x1);
    labels.push_back(label);
  }
  const auto X = pluss::make_series(features, {n, 2}).to(f64);
  const auto y = pluss::make_series(labels, {n, 1}).to(f64);

  const auto loss_of = [&](const pluss::Series& w, const pluss::Series& b) -> pluss::Series {
    const pluss::Series z = X.matmul(w) + b;  // [n, 1]
    const pluss::Series margin = y * z;
    return (1.0 + (margin * -1.0).exp()).log().sum() * (1.0 / n) + 0.05 * w.square().sum();
  };
  const auto zeros_w = pluss::Series::zeros({2, 1}, pluss::ScalarType::Float64);
  const auto zeros_b = pluss::Series::zeros({1}, pluss::ScalarType::Float64);

  std::vector<std::pair<std::string, std::vector<double>>> fits;
  std::cout << std::left << std::setw(10) << "optimizer" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << std::setw(12) << "loss"
            << "weights, bias\n";
  const auto report = [&](const std::string& name, const char* exit, std::int64_t iterations, std::int64_t evaluations, double loss,
                          const pluss::Series& w, const pluss::Series& b) {
    const auto wv = w.reshape({2}).host<double>();
    const double bias = as_f64(b);
    fits.push_back({name, {wv[0], wv[1], bias}});
    std::cout << std::left << std::setw(10) << name << std::setw(14) << exit << std::setw(8) << iterations << std::setw(8) << evaluations << std::fixed
              << std::setprecision(6) << std::setw(12) << loss << std::setprecision(4) << wv[0] << ", " << wv[1] << ", " << bias << "\n";
  };

  std::function<pluss::Series(const Pack&)> named = [&](const Pack& p) { return loss_of(p.at("w"), p.at("b")); };
  const optim::ParameterPack<pluss::Series> start(std::vector<Block>{{"w", zeros_w}, {"b", zeros_b}});
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 3000;
    options.step.adam.learning_rate = 0.1;
    options.gradient_tolerance = 1e-5;
    options.step_tolerance = 1e-5;
    const auto r = optim::septima(named, start, options);
    report("Adam", r.converged ? "Converged" : "MaxIterations", r.iteration, r.function_evaluations, r.objective_value, r.params[0], r.params[1]);
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 500;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto r = optim::septima(named, start, options);
    report("L-BFGS", r.converged ? "Converged" : "NoProgress", r.iteration, r.function_evaluations, r.objective_value, r.params[0], r.params[1]);
  }
  {
    const auto [params, r] = optim::l_sr1_trust_region(
        [&](const std::vector<pluss::Series>& blocks) -> pluss::Series { return loss_of(blocks[0], blocks[1]); }, std::vector<pluss::Series>{zeros_w, zeros_b});
    report("L-SR1", r.converged ? "Converged" : "not converged", r.iteration, r.function_evaluations, r.objective_value, params[0], params[1]);
  }

  double spread = 0.0;
  for (std::size_t k = 0; k < 3; ++k)
    for (const auto& fit : fits) spread = std::max(spread, std::abs(fit.second[k] - fits.back().second[k]));
  const auto& best = fits.back().second;
  int correct = 0;
  for (int i = 0; i < n; ++i) {
    const double z = features[2 * i] * best[0] + features[2 * i + 1] * best[1] + best[2];
    if ((z > 0.0 ? 1.0 : -1.0) == labels[static_cast<std::size_t>(i)]) ++correct;
  }
  std::cout << std::scientific << std::setprecision(1) << "\nlargest difference between the three answers: " << spread << "\n"
            << std::fixed << std::setprecision(3) << "fraction classified correctly: " << static_cast<double>(correct) / n << "\n";
  return 0;
}
