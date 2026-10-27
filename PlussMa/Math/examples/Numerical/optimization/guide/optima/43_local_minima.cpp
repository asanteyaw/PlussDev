// A nonconvex objective has more than one local minimum, and which one a solver reports depends on the
// algorithm and on the start. f(x) = (x^2 - 1)^2 + 0.3 x on [-2, 2] has a deep minimum near x = -1.04
// (f = -0.31) and a shallow one near x = 0.96 (f = 0.29). Every run below exits with flag 1, which
// certifies a local minimum only. Nothing in the result says whether a better one exists.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto one = [&](double x) { return optim::pack_scalars({{"x", scalar(x)}}); };

  auto objective = [](const Pack& p) {
    return (p.at("x").square() - 1.0).square() + 0.3 * p.at("x");
  };
  const auto box = optim::constraints().lbs(one(-2.0)).ubs(one(2.0));

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};
  const std::vector<double> starts = {-1.5, -0.3, 0.3, 1.5};

  int converged = 0;
  std::cout << std::fixed << std::setprecision(3);
  std::cout << std::left << std::setw(18) << "start";
  for (double s : starts) std::cout << std::right << std::setw(9) << s;
  std::cout << "\n";
  for (const auto& row : rows) {
    std::cout << std::left << std::setw(18) << row.label;
    for (double s : starts) {
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      const auto r = optim::optima(objective, one(s), options, box);
      std::cout << std::right << std::setw(9) << as_f64(r.params.at("x"));
      if (r.converged) ++converged;
    }
    std::cout << "\n";
  }
  std::cout << "runs that exited with flag Converged: " << converged << " of 16\n";
  return 0;
}
