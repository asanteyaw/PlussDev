// When the objective is a long loop of scalar operations, the objective can be differentiated in two ways with the same
// result. This fits the three parameters of a GARCH(1,1) volatility model to 2000 simulated returns: the log-likelihood
// is a loop that updates a variance once per observation. Reverse mode records every step of the loop on a tape;
// `Fwd<3>` carries the three partial derivatives in plain doubles through the same loop, with no tape. The program
// fits the model both ways and shows that the fits agree: same iterations, same objective, same parameters.
#include <pluss/ma.h>

#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace optim = pluss::subsp::math::optim;

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

// The same text runs on a Series and on a forward-mode number; a Series takes its logarithm as a method.
inline pluss::Series log(const pluss::Series& x) { return x.log(); }

template <class Number>
Number negative_log_likelihood(const std::vector<double>& r, const Number& omega, const Number& alpha, const Number& beta) {
  Number variance = omega / (1.0 - alpha - beta);
  Number total = variance * 0.0;
  for (double x : r) {
    total = total + 0.5 * (log(variance) + (x * x) / variance);
    variance = omega + alpha * (x * x) + beta * variance;
  }
  return total / static_cast<double>(r.size());
}

int main() {
  const auto returns = simulate(2000);
  const auto lower = params3(1e-4, 1e-3, 1e-3), upper = params3(1.0, 0.5, 0.995);
  const auto start = params3(0.1, 0.05, 0.8);

  optim::ConstrainedProblem traced;
  traced.objective.autodiff = [&](const optim::ParameterPack<pluss::Series>& p) -> pluss::Series {
    return negative_log_likelihood<pluss::Series>(returns, p.at("omega"), p.at("alpha"), p.at("beta"));
  };
  traced.nonlinear.inequality = [](const optim::ParameterPack<pluss::Series>& p) -> pluss::Series { return p.at("alpha") + p.at("beta") - 0.999; };
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

  const char* labels[2] = {"reverse mode (pluss::Series)", "forward mode (Fwd<3>)"};
  std::cout << std::left << std::setw(24) << "mode" << std::setw(8) << "exit" << std::setw(12) << "iterations" << std::setw(12) << "objective"
            << "omega, alpha, beta\n";
  for (int i = 0; i < 2; ++i) {
    const auto& p = results[i].params;
    std::cout << std::left << std::setw(24) << labels[i] << std::setw(8) << static_cast<int>(results[i].exit_flag) << std::setw(12)
              << results[i].iteration << std::fixed << std::setprecision(6) << std::setw(12) << results[i].evaluation.value.item<double>()
              << std::setprecision(4) << as_f64(p.at("omega")) << ", " << as_f64(p.at("alpha")) << ", " << as_f64(p.at("beta")) << "\n";
  }
  return 0;
}
