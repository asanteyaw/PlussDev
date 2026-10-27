// Constrained least squares, with the answer checked against Optima. Choose non-negative weights that sum to 1 so that a
// weighted mix of five series matches a target as closely as possible: minimize |A w - t|^2 subject to w >= 0 and sum(w) = 1.
// The data are generated from known weights (0.5, 0, 0.3, 0, 0.2) plus a little noise, so the right answer has two weights at
// zero, which are active bounds. Optima solves it as a quadratic program; Septima uses its bound projection for w >= 0 and an
// equality constraint for the sum. The table reports the weights, the sum, the objective and the number of function evaluations.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;

int main() {
  const int rows = 40, columns = 5;
  std::uint64_t state = 99;
  const auto uniform = [&] { state = state * 6364136223846793005ULL + 1442695040888963407ULL; return ((state >> 11) + 0.5) / 9007199254740992.0; };
  const std::vector<double> truth = {0.5, 0.0, 0.3, 0.0, 0.2};
  std::vector<double> a, t;
  for (int i = 0; i < rows; ++i) {
    double mix = 0.0;
    for (int j = 0; j < columns; ++j) {
      const double v = uniform() - 0.5;
      a.push_back(v);
      mix += truth[static_cast<std::size_t>(j)] * v;
    }
    t.push_back(mix + 0.01 * (uniform() - 0.5));
  }
  const auto A = pluss::make_series(a, {rows, columns}).to(f64);
  const auto target = pluss::make_series(t, {rows}).to(f64);
  const auto objective = [&](const pluss::Series& w) -> pluss::Series {
    const pluss::Series mix = A.matmul(w.reshape({columns, 1}));
    return (mix - target.reshape({rows, 1})).square().sum();
  };
  const auto start = pluss::make_series(std::vector<double>(columns, 0.2), {columns}).to(f64);

  std::cout << std::left << std::setw(18) << "solver" << std::setw(8) << "evals" << std::setw(12) << "objective" << std::setw(10) << "sum(w)" << "weights\n";
  const auto row = [&](const char* label, std::int64_t evals, double value, const Series& w) {
    const auto v = w.host<double>();
    double sum = 0.0;
    for (double x : v) sum += x;
    std::cout << std::left << std::setw(18) << label << std::setw(8) << evals << std::fixed << std::setprecision(5) << std::setw(12) << value << std::setw(10) << sum;
    for (double x : v) std::cout << std::setprecision(4) << x << "  ";
    std::cout << "\n";
  };

  // Optima: bounds and one linear equality.
  const auto o = optim::optima(objective, start, {},
                               optim::constraints()
                                   .lbs(pluss::make_series(std::vector<double>(columns, 0.0), {columns}).to(f64))
                                   .ln_eq(pluss::make_series(std::vector<double>(columns, 1.0), {1, columns}).to(f64), pluss::make_series(std::vector<double>{1.0}, {1}).to(f64)));
  row("Optima", o.function_evaluations, o.evaluation.value.item<double>(), o.params.at("x"));

  // Septima: bounds by projection, the sum by an equality residual.
  optim::SeptimaConstraints constraints;
  constraints.lbs = {pluss::make_series(std::vector<double>(columns, 0.0), {columns}).to(f64)};
  constraints.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].sum() - 1.0}; };
  optim::SeptimaSolveOptions options;
  options.max_iterations = 4000;
  options.gradient_tolerance = 1e-5;
  options.step_tolerance = 1e-6;
  options.step.adam.learning_rate = 0.01;  // the constraint schedule is the library default
  const auto s = optim::septima(objective, start, options, constraints);
  row("Septima, Adam", s.function_evaluations, s.objective_value, s.params[0]);
  std::cout << "\nSeptima's equality violation: " << std::scientific << std::setprecision(1) << s.constraint_violation << ", exit "
            << (s.converged ? "Converged" : "not converged") << " after " << s.iteration << " iterations\n";
  return 0;
}
