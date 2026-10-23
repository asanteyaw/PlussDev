<!-- generated from the_options_that_decide_the_outcome.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The options that decide the outcome

[Back to Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)


Most of the options in each algorithm's struct do little on a well-behaved problem. This program sweeps the ones that moved the result on the problem from Program 1, so you know where to look first.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto four = [&](double a, double b, double c, double d) {
    return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
  };
  auto objective = [](const Pack& p) {
    return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
  };
  const auto constraints =
      optim::constraints()
          .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
          .nl_eq([](const Pack& p) {
            return optim::square(p.at("a")) + optim::square(p.at("b")) + optim::square(p.at("c")) +
                   optim::square(p.at("d")) - 40.0;
          })
          .lbs(four(1, 1, 1, 1))
          .ubs(four(5, 5, 5, 5));

  std::cout << std::left << std::setw(52) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(7) << "evals" << "objective\n";
  auto fmt = [](double v) {
    std::ostringstream out;
    out << v;
    return out.str();
  };
  auto run = [&](const std::string& label, const optim::OptimaOptions& options) {
    const auto r = optim::optima(objective, four(1, 5, 5, 1), options, constraints);
    std::cout << std::left << std::setw(52) << label << std::setw(6) << static_cast<int>(r.exit_flag)
              << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations << std::fixed
              << std::setprecision(4) << r.evaluation.value.item<double>() << std::defaultfloat << "\n";
  };

  for (double mu : {1e-4, 1e-2, 1.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    o.internal_protocol.initial_barrier_parameter = mu;
    run("InternalProtocol initial_barrier_parameter " + fmt(mu), o);
  }
  for (double f : {0.9, 0.99}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    o.internal_protocol.fraction_to_boundary = f;
    run("InternalProtocol fraction_to_boundary " + fmt(f), o);
  }
  for (bool on : {true, false}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    o.internal_protocol.independent_nonlinear_slacks = on;
    run(std::string("InternalProtocol independent_nonlinear_slacks ") + (on ? "true" : "false"), o);
  }
  for (double tol : {1e-3, 1e-6, 1e-10}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.optimality_tolerance = tol;
    run("SequentialQP optimality_tolerance " + fmt(tol), o);
  }
  for (double penalty : {1.0, 10.0, 100.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::MeritMethod;
    o.penalty.penalty = penalty;
    run("MeritMethod penalty " + fmt(penalty), o);
  }
  return 0;
}
```

Output

```text
setting                                             flag  iters  evals  objective
InternalProtocol initial_barrier_parameter 0.0001   1     15     26     17.0140
InternalProtocol initial_barrier_parameter 0.01     1     8      12     17.0140
InternalProtocol initial_barrier_parameter 1        1     18     43     17.0140
InternalProtocol fraction_to_boundary 0.9           1     10     14     17.0140
InternalProtocol fraction_to_boundary 0.99          1     8      12     17.0140
InternalProtocol independent_nonlinear_slacks true  1     8      12     17.0140
InternalProtocol independent_nonlinear_slacks false 1     56     293    17.0140
SequentialQP optimality_tolerance 0.001             1     4      5      17.0140
SequentialQP optimality_tolerance 1e-06             1     4      5      17.0140
SequentialQP optimality_tolerance 1e-10             1     5      6      17.0140
MeritMethod penalty 1                               1     33     84     17.0140
MeritMethod penalty 10                              1     66     180    27.1464
MeritMethod penalty 100                             1     79     269    17.0140
```
Four lessons. The initial barrier parameter of the interior method is worth a quick sweep: too small and too large both cost iterations, and the default `0.01` was the best of the three. Backing off `fraction_to_boundary` from `0.99` to `0.9` costs two iterations here. `independent_nonlinear_slacks` is the single largest effect in the table, 56 iterations and 293 evaluations without it against 8 and 12 with it, because without independent slacks a start on the boundary of a nonlinear inequality can only creep along the curve. Tightening sequential QP's `optimality_tolerance` from `1e-6` to `1e-10` costs one more iteration, from 4 to 5, with no change visible at four decimals. And the merit method's `penalty` does not just change the path: with `10` it settles at the other first-order point, `27.1464`, while `1` and `100` reach `17.0140`. When one algorithm's answer looks wrong, run it again with a different penalty before blaming the model.
