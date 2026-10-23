// The quasi-Newton algorithms build their curvature model from the steps they take. `hessian_approximation`
// chooses the model. This program minimizes the six-variable Rosenbrock chain, whose curvature is strongly
// coupled, under bounds, with each model that needs no extra callbacks (`Bfgs`, `Lbfgs`, `Identity`), in both
// quasi-Newton algorithms, and prints the work done. `Identity` ignores curvature entirely and shows what the
// models are worth. The exact models need the problem's dual-number callbacks and are the subject of a later chapter.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  const int n = 6;
  auto name = [](int i) { return "x" + std::to_string(i); };
  using Block = optim::ParameterPack<pluss::Series>::Block;
  auto filled = [&](double v) {
    std::vector<Block> blocks;
    for (int i = 0; i < n; ++i) blocks.push_back({name(i), scalar(v).reshape({1})});
    return optim::ParameterPack<pluss::Series>(blocks);
  };
  auto start = [&] {
    std::vector<Block> blocks;
    for (int i = 0; i < n; ++i) blocks.push_back({name(i), scalar(i % 2 == 0 ? -1.2 : 1.0).reshape({1})});
    return optim::ParameterPack<pluss::Series>(blocks);
  }();

  auto objective = [&](const Pack& p) {
    optim::Trace total = p.at(name(0)) * 0.0;
    for (int i = 0; i + 1 < n; ++i) {
      total = total + 100.0 * optim::square(p.at(name(i + 1)) - optim::square(p.at(name(i)))) +
              optim::square(1.0 - p.at(name(i)));
    }
    return total;
  };
  const auto box = optim::constraints().lbs(filled(-3.0)).ubs(filled(3.0));

  struct Model { const char* label; optim::OptimaHessianApproximation model; };
  const std::vector<Model> models = {{"Bfgs", optim::OptimaHessianApproximation::Bfgs},
                                     {"Lbfgs", optim::OptimaHessianApproximation::Lbfgs},
                                     {"Identity", optim::OptimaHessianApproximation::Identity}};

  std::cout << std::left << std::setw(18) << "algorithm" << std::setw(10) << "model" << std::setw(6) << "flag"
            << std::setw(7) << "iters" << std::setw(7) << "evals" << "objective\n";
  auto run = [&](const char* label, optim::OptimaOptions options, const Model& m) {
    const auto r = optim::optima(objective, start, options, box);
    std::cout << std::left << std::setw(18) << label << std::setw(10) << m.label << std::setw(6)
              << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration << std::setw(7)
              << r.function_evaluations << std::scientific << std::setprecision(2)
              << r.evaluation.value.item<double>() << std::defaultfloat << "\n";
  };
  for (const auto& m : models) {
    optim::OptimaOptions options;
    options.algorithm = optim::OptimaAlgorithm::SequentialQP;
    options.sequential_qp.hessian_approximation = m.model;
    options.sequential_qp.max_iterations = 300;
    run("SequentialQP", options, m);
  }
  for (const auto& m : models) {
    optim::OptimaOptions options;
    options.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    options.internal_protocol.hessian_approximation = m.model;
    options.internal_protocol.max_iterations = 300;
    run("InternalProtocol", options, m);
  }
  return 0;
}
