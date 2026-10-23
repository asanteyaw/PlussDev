// After the quadratic subproblem proposes a step, a backtracking line search shortens it until the merit
// function falls enough. `initial_step_size` is the fraction of the proposed step tried first (the full step, 1,
// is the natural choice for a Newton-type method; a smaller one gives up its fast local convergence). `armijo`
// sets how much of the predicted fall counts as "enough", `backtracking` is the factor the step is multiplied by
// after each rejection, and `max_line_search_iterations` caps the rejections. Gentle backtracking keeps more of
// each step and spends more evaluations hunting for the largest acceptable one; aggressive backtracking spends
// fewer evaluations per iteration and takes shorter steps. The problem is a curved valley with an inequality and
// an equality constraint, started far from the solution.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> start_point() { return optim::pack_scalars({{"x", scalar(-1.9)}, {"y", scalar(2.0)}}); }
// Rosenbrock's curved valley, restricted to the disc x^2 + y^2 <= 1.5 and to the hyperbola x y = 0.4.
auto objective = [](const Pack& p) {
  return 100.0 * optim::square(p.at("y") - optim::square(p.at("x"))) + optim::square(1.0 - p.at("x"));
};
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return optim::square(p.at("x")) + optim::square(p.at("y")) - 1.5; })
      .nl_eq([](const Pack& p) { return p.at("x") * p.at("y") - 0.4; });
}

int main() {
  std::cout << std::left << std::setw(34) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(7) << "evals" << std::setw(13) << "objective" << "violation\n";
  auto run = [&](const std::string& label, const optim::OptimaOptions& options, const auto& start, const auto& cons, const auto& f) {
    const auto r = optim::optima(f, start, options, cons);
    std::cout << std::left << std::setw(34) << label << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7)
              << r.iteration << std::setw(7) << r.function_evaluations << std::fixed << std::setprecision(5)
              << std::setw(13) << r.evaluation.value.template item<double>() << std::scientific << std::setprecision(1)
              << r.evaluation.max_constraint_violation << std::defaultfloat << "\n";
  };
  for (double b : {0.2, 0.5, 0.8}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.backtracking = b;
    run("backtracking " + std::to_string(b).substr(0, 4), o, start_point(), constraints(), objective);
  }
  for (double a : {1e-6, 1e-4, 1e-1}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.armijo = a;
    std::ostringstream label;
    label << "armijo " << a;
    run(label.str(), o, start_point(), constraints(), objective);
  }
  for (std::int64_t k : {1, 3, 20}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.max_line_search_iterations = k;
    run("max_line_search_iterations " + std::to_string(k), o, start_point(), constraints(), objective);
  }
  for (double s0 : {0.1, 0.5, 1.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.initial_step_size = s0;
    run("initial_step_size " + std::to_string(s0).substr(0, 4), o, start_point(), constraints(), objective);
  }
  return 0;
}
