<!-- generated from a_model_fitted_to_data.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A model fitted to data

[Back to Choosing a Solver, and Worked Problems](../septima_05_choosing_and_worked_problems.md)


Two hundred points in the plane are labelled by which side of a line they fall on, with one label in ten flipped. The model is logistic regression with a small L2 penalty. The data are a `Series` held fixed inside the objective; the parameters are a weight column and a bias; the loss is a few lines of `Trace` arithmetic.

```cpp
#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;
using Pack = optim::ParameterPack<Trace>;
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

  const auto loss_of = [&](const Trace& w, const Trace& b) -> Trace {
    const Trace z = optim::matmul(optim::fixed(X), w) + b;  // [n, 1]
    const Trace margin = optim::fixed(y) * z;
    return optim::sum(optim::log(1.0 + optim::exp(margin * -1.0))) * (1.0 / n) + 0.05 * optim::sum(optim::square(w));
  };
  const auto zeros_w = pluss::Series::zeros({2, 1}, pluss::ScalarType::Float64);
  const auto zeros_b = pluss::Series::zeros({1}, pluss::ScalarType::Float64);

  std::vector<std::pair<std::string, std::vector<double>>> fits;
  std::cout << std::left << std::setw(10) << "optimizer" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << std::setw(12) << "loss"
            << "weights, bias\n";
  const auto report = [&](const std::string& name, const char* exit, std::int64_t iterations, std::int64_t evaluations, double loss,
                          const pluss::Series& w, const pluss::Series& b) {
    const auto wv = w.reshape({2}).to_vec<double>();
    const double bias = b.item<double>();
    fits.push_back({name, {wv[0], wv[1], bias}});
    std::cout << std::left << std::setw(10) << name << std::setw(14) << exit << std::setw(8) << iterations << std::setw(8) << evaluations << std::fixed
              << std::setprecision(6) << std::setw(12) << loss << std::setprecision(4) << wv[0] << ", " << wv[1] << ", " << bias << "\n";
  };

  std::function<Trace(const Pack&)> named = [&](const Pack& p) { return loss_of(p.at("w"), p.at("b")); };
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
        [&](const std::vector<Trace>& blocks) -> Trace { return loss_of(blocks[0], blocks[1]); }, std::vector<pluss::Series>{zeros_w, zeros_b});
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
```

Output

```text
optimizer exit          iters   evals   loss        weights, bias
Adam      Converged     190     190     0.557757    0.7150, -0.4648, -0.0354
L-BFGS    NoProgress    33      33      0.557757    0.7150, -0.4648, -0.0354
L-SR1     Converged     8       9       0.557757    0.7150, -0.4648, -0.0354

largest difference between the three answers: 2.3e-05
fraction classified correctly: 0.825
```
The problem is convex, so every optimizer must find the same answer, and they do: the three fits agree to 2.3e-05, with a loss of 0.557757, weights `(0.7150, -0.4648)` and a bias of -0.0354. The weights are in the ratio 1.54 to -1, close to the 1.5 to -1 of the line that generated the labels, and the model classifies 82.5% of the points correctly, which is about as good as it can do when one label in ten is wrong.

The cost is where they differ. L-SR1 converges in 7 iterations and 8 evaluations, L-BFGS in 33 (and ends with `NoProgress`, having reached the limit of its tolerances), Adam in 190. For a smooth model fitted to a modest amount of data, with all the data in the objective at once, a curvature method is the right choice. Adam's strength is elsewhere: it needs only a gradient and tolerates a gradient that is noisy, which a curvature method does not.
