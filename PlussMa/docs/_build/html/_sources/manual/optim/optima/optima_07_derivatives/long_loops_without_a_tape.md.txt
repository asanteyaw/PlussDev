<!-- generated from long_loops_without_a_tape.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Long loops without a tape

[Back to Derivatives](../optima_07_derivatives.md)


Some objectives are a loop of a few scalar operations repeated thousands of times: a state-space filter, a volatility model, a recursion over a time series. Reverse mode records every operation of the loop on a tape, so the tape grows with the length of the loop. `Fwd<N>` carries the value and the `N` partial derivatives in plain doubles instead: no tape and no allocation. The program fits a GARCH(1,1) model, three parameters, to 2000 simulated returns, with the log-likelihood written once as a template and used with both number types.

```cpp
#include <pluss/ma.h>

#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace optim = pluss::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> params3(double omega, double alpha, double beta) {
  return optim::pack_scalars({{"omega", scalar(omega)}, {"alpha", scalar(alpha)}, {"beta", scalar(beta)}});
}

// Simulated returns from a GARCH(1,1) with omega = 0.05, alpha = 0.1, beta = 0.85, from a fixed generator.
std::vector<double> simulate(int n) {
  std::uint64_t state = 12345;
  const auto uniform = [&] { state = state * 6364136223846793005ULL + 1442695040888963407ULL; return ((state >> 11) + 0.5) / 9007199254740992.0; };
  std::vector<double> r(static_cast<std::size_t>(n));
  double variance = 0.05 / (1.0 - 0.1 - 0.85);
  for (int t = 0; t < n; ++t) {
    const double z = std::sqrt(-2.0 * std::log(uniform())) * std::cos(2.0 * 3.14159265358979323846 * uniform());
    r[static_cast<std::size_t>(t)] = std::sqrt(variance) * z;
    variance = 0.05 + 0.1 * r[static_cast<std::size_t>(t)] * r[static_cast<std::size_t>(t)] + 0.85 * variance;
  }
  return r;
}

template <class Number>
Number negative_log_likelihood(const std::vector<double>& r, const Number& omega, const Number& alpha, const Number& beta) {
  Number variance = omega / (1.0 - alpha - beta);
  Number total = variance * 0.0;
  for (double x : r) {
    total = total + 0.5 * (optim::log(variance) + (x * x) / variance);
    variance = omega + alpha * (x * x) + beta * variance;
  }
  return total / static_cast<double>(r.size());
}

int main() {
  const auto returns = simulate(2000);
  const auto lower = params3(1e-4, 1e-3, 1e-3), upper = params3(1.0, 0.5, 0.995);
  const auto start = params3(0.1, 0.05, 0.8);

  optim::ConstrainedProblem traced;
  traced.objective.autodiff = [&](const optim::ParameterPack<optim::Trace>& p) -> optim::Trace {
    return negative_log_likelihood<optim::Trace>(returns, p.at("omega"), p.at("alpha"), p.at("beta"));
  };
  traced.nonlinear.inequality = [](const optim::ParameterPack<optim::Trace>& p) -> optim::Trace { return p.at("alpha") + p.at("beta") - 0.999; };
  traced.bounds.lower = lower;
  traced.bounds.upper = upper;

  using F = optim::Fwd<3>;
  optim::ConstrainedProblem forward = traced;
  forward.objective.autodiff = nullptr;
  forward.objective.analytic = optim::fwd_objective<3>([&](const std::array<F, 3>& p) -> F {
    return negative_log_likelihood<F>(returns, p[0], p[1], p[2]);
  });

  optim::OptimaState results[2];
  const optim::ConstrainedProblem* problems[2] = {&traced, &forward};
  for (int i = 0; i < 2; ++i) {
    results[i] = optim::optima(*problems[i], start);
  }

  const char* labels[2] = {"reverse mode (Trace)", "forward mode (Fwd<3>)"};
  std::cout << std::left << std::setw(24) << "mode" << std::setw(8) << "exit" << std::setw(12) << "iterations" << std::setw(12) << "objective"
            << "omega, alpha, beta\n";
  for (int i = 0; i < 2; ++i) {
    const auto& p = results[i].params;
    std::cout << std::left << std::setw(24) << labels[i] << std::setw(8) << static_cast<int>(results[i].exit_flag) << std::setw(12)
              << results[i].iteration << std::fixed << std::setprecision(6) << std::setw(12) << results[i].evaluation.value.item<double>()
              << std::setprecision(4) << p.at("omega").item<double>() << ", " << p.at("alpha").item<double>() << ", " << p.at("beta").item<double>() << "\n";
  }
  return 0;
}
```

Output

```text
mode                    exit    iterations  objective   omega, alpha, beta
reverse mode (Trace)    1       11          0.433348    0.0340, 0.0807, 0.8835
forward mode (Fwd<3>)   1       11          0.433348    0.0340, 0.0807, 0.8835
```
Both modes give the same fit: 11 iterations, an objective of 0.433348, parameters (0.0340, 0.0807, 0.8835). The choice is therefore about how the derivative is computed, not about the answer. Forward mode does `N` derivative updates for every operation of the plain function, so it suits a small `N` (up to a few dozen); with hundreds of parameters, use `autodiff`, whose work does not grow with `N` in that way. Which is quicker for your function depends on its length, on `N` and on your machine, so measure both before committing.
