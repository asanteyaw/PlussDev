#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace stochastic = pluss::operators::stochastic;

namespace {

pluss::Series scalar(double value) { return pluss::make_series(std::vector<double>{value}, {1}).reshape({}); }
std::vector<double> evaluate(const math::Function &f, const std::vector<double> &point) {
  std::vector<pluss::Series> arguments;
  for (const double v : point) arguments.push_back(scalar(v));
  const pluss::Series out = f(pluss::RangeView<pluss::Series>(arguments)).to(pluss::pCPU, pluss::ScalarType::Float64).contiguous();
  return out.reshape({out.numel()}).host<double>();
}

constexpr const char *kPath[] = {"w1", "w2", "w3", "w4"};
const std::vector<double> kW{0.3, -0.2, 0.5, 0.9};  // a path W_1..W_4
constexpr double kDt = 0.25;

math::Function functional(std::function<math::Function(const std::vector<math::Function> &)> body) {
  return math::function({"w1", "w2", "w3", "w4"}, [body](math::Function a, math::Function b, math::Function c, math::Function d) {
    return body({a, b, c, d});
  });
}

} // namespace

TEST_CASE("Malliavin derivative of path functionals", "[calculus][malliavin]") {
  const auto D = stochastic::malliavin_derivative({"w1", "w2", "w3", "w4"});
  // F = W_N^2: D_{t_i} F = 2 W_N for every i
  const auto d1 = evaluate(D(functional([](const auto &w) { return w[3] * w[3]; })), kW);
  REQUIRE(d1.size() == 4);
  for (const double v : d1) CHECK(std::abs(v - 2.0 * 0.9) < 1e-13);
  // F = exp(W_3): D_i F = exp(W_3) for i <= 3, zero after
  const auto d2 = evaluate(D(functional([](const auto &w) { return w[2].exp(); })), kW);
  for (int i = 0; i < 3; ++i) CHECK(std::abs(d2[static_cast<std::size_t>(i)] - std::exp(0.5)) < 1e-13);
  CHECK(std::abs(d2[3]) < 1e-13);
  // F = dt sum W_j (the integral of the path): D_t F = T - t, i.e. dt (N - i)
  const auto d3 = evaluate(D(functional([](const auto &w) { return (w[0] + w[1] + w[2] + w[3]) * kDt; })), kW);
  for (int i = 0; i < 4; ++i) CHECK(std::abs(d3[static_cast<std::size_t>(i)] - kDt * (4 - i)) < 1e-13);
  // product rule: D(FG) = F DG + G DF
  const auto F = functional([](const auto &w) { return w[1].sin(); });
  const auto G = functional([](const auto &w) { return w[3] * w[2]; });
  const auto FG = functional([](const auto &w) { return w[1].sin() * (w[3] * w[2]); });
  const auto dF = evaluate(D(F), kW), dG = evaluate(D(G), kW), dFG = evaluate(D(FG), kW);
  for (int i = 0; i < 4; ++i) {
    const auto k = static_cast<std::size_t>(i);
    CHECK(std::abs(dFG[k] - (std::sin(-0.2) * dG[k] + 0.5 * 0.9 * dF[k])) < 1e-13);
  }
}

TEST_CASE("Skorokhod integral: Ito sum for adapted integrands, divergence otherwise", "[calculus][malliavin]") {
  const auto delta = stochastic::skorokhod_integral({"w1", "w2", "w3", "w4"}, kDt);
  // u_i = W_N for every i: delta(u) = W_N^2 - N dt
  const math::Function constant = functional([](const auto &w) { return math::stack({w[3], w[3], w[3], w[3]}); });
  CHECK(std::abs(evaluate(delta(constant), kW)[0] - (0.9 * 0.9 - 4 * kDt)) < 1e-13);
  // adapted u_i = W_{i-1}: the Ito sum, no correction
  const math::Function adapted = functional([](const auto &w) { return math::stack({w[0] * 0.0, w[0], w[1], w[2]}); });
  const double ito = 0.0 * 0.3 + 0.3 * (-0.2 - 0.3) + (-0.2) * (0.5 + 0.2) + 0.5 * (0.9 - 0.5);
  CHECK(std::abs(evaluate(delta(adapted), kW)[0] - ito) < 1e-13);
  // u = D F gives delta(D F) = F's Ornstein-Uhlenbeck image; for F = W_N^2: D F = 2 W_N, delta = 2 W_N^2 - 2 N dt
  const auto D = stochastic::malliavin_derivative({"w1", "w2", "w3", "w4"});
  const math::Function F = functional([](const auto &w) { return w[3] * w[3]; });
  CHECK(std::abs(evaluate(delta(D(F)), kW)[0] - (2.0 * 0.81 - 2.0 * 4 * kDt)) < 1e-13);
  CHECK_THROWS(delta(functional([](const auto &w) { return w[0]; })));
}
