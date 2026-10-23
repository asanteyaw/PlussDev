// The algorithms differ in how they treat constraints on the way to the answer, not only at the end. This
// program records the constraint violation at every iteration of each algorithm, from a start inside the
// feasible region and from a start outside it. Whether intermediate points are usable matters if the
// objective is only defined where the constraints hold.
#include <pluss/ma.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto two = [&](double a, double b) { return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}}); };

  // minimize (a - 3)^2 + (b - 3)^2 subject to a^2 + b^2 <= 2 (a disc around the origin)
  auto objective = [](const Pack& p) {
    return optim::square(p.at("a") - 3.0) + optim::square(p.at("b") - 3.0);
  };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::square(p.at("a")) + optim::square(p.at("b")) - 2.0; });

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  for (const auto& start : {std::pair{"inside the disc at (0, 0)", two(0, 0)},
                            std::pair{"outside the disc at (3, 3)", two(3, 3)}}) {
    std::cout << "start " << start.first << "\n";
    for (const auto& row : rows) {
      std::vector<double> violation;
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      options.progress_hook = [&](const optim::OptimaProgressSnapshot& s) {
        if (s.stage == optim::OptimaProgressStage::Iteration)
          violation.push_back(s.evaluation.max_constraint_violation);
        return false;
      };
      const auto r = optim::optima(objective, start.second, options, disc);
      const auto infeasible = std::count_if(violation.begin(), violation.end(), [](double v) { return v > 1e-6; });
      std::cout << "  " << std::left << std::setw(17) << row.label << "iterations " << std::setw(3)
                << violation.size() << " infeasible iterates " << std::setw(3) << infeasible << " flag "
                << static_cast<int>(r.exit_flag) << "\n";
    }
  }
  return 0;
}
