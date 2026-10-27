// OptimaOutput is the structured report inside every result. This program prints every field of
// one solve so each can be matched to the table in the text. The problem is a disc-constrained fit
// solved by sequential QP.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });

  const auto r = optim::optima(objective, vec({0.0, 0.0}), {}, disc);
  const auto& o = r.output;

  std::cout << std::boolalpha << std::setprecision(4) << std::scientific;
  std::cout << "algorithm                             " << o.algorithm << "\n";
  std::cout << "iterations                            " << o.iterations << "\n";
  std::cout << "function_evaluations                  " << o.function_evaluations << "\n";
  std::cout << "constraint_violation                  " << o.constraint_violation << "\n";
  std::cout << "first_order_optimality                " << o.first_order_optimality << "\n";
  std::cout << "step_size                             " << o.step_size << "\n";
  std::cout << "line_search_fraction                  " << o.line_search_fraction << "\n";
  std::cout << "backend_parameter                     " << o.backend_parameter << "\n";
  std::cout << "complementarity                       " << o.complementarity << "\n";
  std::cout << "krylov_iterations                     " << o.krylov_iterations << "\n";
  std::cout << "best_feasible_available               " << o.best_feasible_available << "\n";
  std::cout << "returned_best_feasible                " << o.returned_best_feasible << "\n";
  std::cout << "best_feasible_value                   " << o.best_feasible_value << "\n";
  std::cout << "best_feasible_constraint_violation    " << o.best_feasible_constraint_violation << "\n";
  std::cout << "best_feasible_first_order_optimality  " << o.best_feasible_first_order_optimality << "\n";
  std::cout << "multiplier_max_norm                   " << o.multiplier_max_norm << "\n";
  std::cout << "active_multiplier_count               " << o.active_multiplier_count << "\n";
  std::cout << "detail_message                        " << o.detail_message << "\n";
  std::cout << "stall_category                        " << optim::optima_stall_category_name(o.stall_category) << "\n";
  std::cout << "stall_measure                         " << o.stall_measure << "\n";
  std::cout << "stall_reference                       " << o.stall_reference << "\n";
  return 0;
}
