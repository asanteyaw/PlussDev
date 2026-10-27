// Restoration is the last resort of sequential QP. It runs only after the line search has failed twice in a
// row (once with the curvature model reset), and it spends a few iterations minimizing the constraint violation
// alone to get back to a point where the linearization is useful again. This program provokes it on purpose: a
// curved equality constraint, an infeasible start, and a line search starved down to one or three trial steps.
// With a sensible line-search budget the two settings of `enable_restoration` are indistinguishable, because the
// last resort is never reached; with a starved budget restoration spends extra evaluations and still cannot
// rescue the run, so it is no substitute for the line search.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  const auto start = optim::pack_scalars({{"x", scalar(-2.0)}, {"y", scalar(-1.2)}});
  auto f = [](const Pack& p) { return 2.0 * (p.at("x").square() + p.at("y").square() - 1.0) - p.at("x"); };
  const auto cons = optim::constraints().nl_eq(
      [](const Pack& p) { return p.at("x").square() + p.at("y").square() - 1.0; });

  std::cout << std::left << std::setw(26) << "max_line_search_iterations" << std::setw(13) << "restoration" << std::setw(6)
            << "flag" << std::setw(7) << "iters" << std::setw(7) << "evals" << "message\n";
  for (std::int64_t budget : {1, 3, 20}) {
    for (bool on : {true, false}) {
      optim::OptimaOptions o;
      o.algorithm = optim::OptimaAlgorithm::SequentialQP;
      o.sequential_qp.max_line_search_iterations = budget;
      o.sequential_qp.max_second_order_corrections = 0;
      o.sequential_qp.enable_restoration = on;
      const auto r = optim::optima(f, start, o, cons);
      std::cout << std::left << std::setw(26) << budget << std::setw(13) << (on ? "enabled" : "disabled") << std::setw(6)
                << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations
                << r.message.substr(0, 56) << "\n";
    }
  }
  return 0;
}
