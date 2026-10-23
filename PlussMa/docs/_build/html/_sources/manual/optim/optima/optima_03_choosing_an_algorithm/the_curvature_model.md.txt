<!-- generated from the_curvature_model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The curvature model

[Back to Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)


The quasi-Newton algorithms build a model of the curvature from the steps they take, and `hessian_approximation` chooses the model. Here a six-variable chain with strongly coupled curvature is minimized under bounds, with each model the two algorithms accept.

```cpp
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
```

Output

```text
algorithm         model     flag  iters  evals  objective
SequentialQP      Bfgs      1     56     61     1.44e-10
SequentialQP      Lbfgs     1     56     64     1.13e-09
SequentialQP      Identity  0     300    301    5.10e+00
InternalProtocol  Bfgs      1     57     65     1.51e-08
InternalProtocol  Lbfgs     1     51     64     5.71e-09
InternalProtocol  Identity  0     300    302    5.07e+00
```
The identity model uses no curvature information at all, and it shows what the other models are worth: neither algorithm finishes in 300 iterations with it, and both stop with objective values around `5` against below `1e-7` for the real models. Between the real models the choice is smaller. `Bfgs` stores a dense matrix, which is fine here and large for thousands of variables; `Lbfgs` stores a few recent steps and rebuilds the matrix from them, and it needed fewer iterations than `Bfgs` in the interior method (51 against 57) and the same number in sequential QP (56). All three algorithms accept all of these models. Two more, `Exact` and `HessianVector`, use exact second derivatives and need dual-number callbacks; chapter 6 compares all four, and chapter 7 shows how to supply the callbacks.
