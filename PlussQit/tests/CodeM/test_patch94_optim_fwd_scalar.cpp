// Fixed-width forward-mode scalars: exact gradients of a scalar recurrence without the reverse-mode tape.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>
#include <pluss/subsp/math/optim_fwd.h>

#include <array>
#include <cmath>
#include <vector>

namespace {
using Catch::Approx;
namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using pluss::ScalarType;

std::vector<double> data(int n) {
  std::vector<double> y(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) y[static_cast<std::size_t>(i)] = 0.01 * std::sin(0.37 * i) + 0.004 * std::cos(1.3 * i);
  return y;
}

Series vec(std::vector<double> v) {
  return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(ScalarType::Float64);
}
}  // namespace

TEST_CASE("forward-mode scalars give the same gradient as the reverse-mode tape", "[optim][fwd]") {
  const auto y = data(200);
  using F = optim::Fwd<4>;
  // variance recurrence h_t = w + b (h_{t-1} - w) + a (y_{t-1}^2 - h_{t-1}) with Gaussian likelihood
  const auto fwd = [&](const std::array<F, 4>& p) -> F {
    F h = p[3], total(0.0);
    for (std::size_t t = 1; t < y.size(); ++t) {
      h = p[0] + p[1] * (h - p[0]) + p[2] * (y[t - 1] * y[t - 1] - h);
      total = total + optim::log(h) * 0.5 + (y[t] * y[t]) / h * 0.5;
    }
    return total;
  };
  optim::ConstrainedProblem by_fwd = optim::problem().objective_analytic(optim::fwd_objective<4>(fwd)).build();
  optim::ConstrainedProblem by_tape;
  by_tape.objective.autodiff = [&](const optim::ParameterPack<optim::Trace>& q) -> optim::Trace {
    const optim::Trace w = q.at("p1"), b = q.at("p2"), a = q.at("p3");
    optim::Trace h = q.at("p4"), total = h * 0.0;
    for (std::size_t t = 1; t < y.size(); ++t) {
      h = w + b * (h - w) + a * (y[t - 1] * y[t - 1] - h);
      total = total + optim::log(h) * 0.5 + (y[t] * y[t]) / h * 0.5;
    }
    return total;
  };

  const std::vector<double> x0{1e-4, 0.85, 0.08, 1.2e-4};
  std::vector<optim::ParameterPack<Series>::Block> blocks;
  for (std::size_t i = 0; i < x0.size(); ++i)
    blocks.push_back({"p" + std::to_string(i + 1), pluss::make_series(x0[i]).to(ScalarType::Float64).reshape({1})});
  optim::ParameterPack<Series> pk(blocks);
  const auto e_tape = optim::evaluate_problem(by_tape, pk);
  const auto e_fwd = optim::evaluate_problem(by_fwd, pk);
  REQUIRE(e_fwd.value.item<double>() == Approx(e_tape.value.item<double>()).epsilon(1e-12));
  const auto g_tape = optim::flatten_values(e_tape.gradient).to_vec<double>();
  const auto g_fwd = optim::flatten_values(e_fwd.gradient).to_vec<double>();
  for (std::size_t i = 0; i < g_tape.size(); ++i) REQUIRE(g_fwd[i] == Approx(g_tape[i]).epsilon(1e-9));
}

TEST_CASE("an objective built from forward-mode scalars can be optimized", "[optim][fwd]") {
  using F = optim::Fwd<2>;
  const auto rosen = [](const std::array<F, 2>& p) -> F {
    return 100.0 * optim::square(p[1] - optim::square(p[0])) + optim::square(1.0 - p[0]);
  };
  const auto problem = optim::problem().objective_analytic(optim::fwd_objective<2>(rosen)).build();
  for (auto algorithm : {optim::OptimaAlgorithm::InternalProtocol, optim::OptimaAlgorithm::SequentialQP}) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    options.sequential_qp.max_iterations = 200;
    const auto r = optim::optima(problem, vec({-1.2, 1.0}), options);
    const auto x = r.params.at("x").to_vec<double>();
    REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
    REQUIRE(x[0] == Approx(1.0).margin(1e-4));
    REQUIRE(x[1] == Approx(1.0).margin(1e-4));
  }
}

TEST_CASE("a forward-mode objective rejects a mismatched parameter count", "[optim][fwd]") {
  using F = optim::Fwd<3>;
  const auto problem = optim::problem().objective_analytic(optim::fwd_objective<3>([](const std::array<F, 3>& p) { return p[0] + p[1] + p[2]; })).build();
  const auto r = optim::optima(problem, vec({1.0, 2.0}));
  REQUIRE(r.exit_flag != optim::OptimaExitFlag::Converged);
}
