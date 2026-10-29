#include "../FunctionInternal.h"

#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <sstream>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

namespace {

constexpr auto F64 = pluss::ScalarType::Float64;

pluss::Series scalar(double value) {
  return pluss::make_series(std::vector<double>{value}, {1}).reshape({});
}
std::vector<double> host(const pluss::Series &value) {
  const pluss::Series cpu = value.to(pluss::pCPU, F64).contiguous();
  return cpu.reshape({cpu.numel()}).host<double>();
}
double one(const pluss::Series &value) { return host(value)[0]; }
double at(const math::Function &f, std::initializer_list<double> point, std::size_t index = 0) {
  std::vector<pluss::Series> arguments;
  for (const double v : point) arguments.push_back(scalar(v));
  return host(f(pluss::RangeView<pluss::Series>(arguments)))[index];
}

// one Wiener path on [0, 1] with `steps` steps: times [N] and path [N]
struct Path {
  pluss::Series t, w;
  sde::WienerPath raw;
  double sum_dw2 = 0.0, w_final = 0.0;
};
Path make_path(std::int64_t steps, std::uint64_t seed) {
  Path p;
  p.raw = sde::simulate_wiener_path(scalar(0.0), 0.0, 1.0, steps, seed);
  p.w = p.raw.w;
  p.t = pluss::make_series(p.raw.times, {steps + 1});
  const std::vector<double> w = host(p.w);
  for (std::size_t i = 0; i + 1 < w.size(); ++i) p.sum_dw2 += (w[i + 1] - w[i]) * (w[i + 1] - w[i]);
  p.w_final = w.back();
  return p;
}
math::Function noise_function() {
  return math::function({"t", "W"}, [](math::Function, math::Function w) { return w; });
}
double integrate(const math::Function &f, const stochastic::IntegralOptions &options, const Path &p) {
  return one(stochastic::stochastic_integral("W", options)(f)({p.t, p.w}));
}

} // namespace

TEST_CASE("the discrete stochastic integral obeys its exact identities", "[calculus][stochastic]") {
  const Path p = make_path(2000, 7);
  const math::Function f = noise_function();
  const double ito = integrate(f, {}, p);
  const double strat = integrate(f, {.convention = stochastic::Convention::Stratonovich}, p);
  const double klim = integrate(f, {.convention = stochastic::Convention::Klimontovich}, p);
  const double theta = integrate(f, {.convention = stochastic::Convention::Theta, .theta = 0.3}, p);
  CHECK(std::abs(ito - (p.w_final * p.w_final - p.sum_dw2) / 2.0) < 1e-12);
  CHECK(std::abs(strat - p.w_final * p.w_final / 2.0) < 1e-12);
  CHECK(std::abs(klim - (p.w_final * p.w_final + p.sum_dw2) / 2.0) < 1e-12);
  CHECK(std::abs(theta - (ito + 0.3 * p.sum_dw2)) < 1e-12);
  CHECK(std::abs((strat - ito) - p.sum_dw2 / 2.0) < 1e-12); // the Ito-Stratonovich gap is half the quadratic variation
  // the constant integrand integrates to the increment of the noise
  const math::Function constant = math::function({"t", "W"}, [](math::Function t, math::Function w) { return t * 0.0 + w * 0.0 + 1.0; });
  CHECK(std::abs(integrate(constant, {}, p) - p.w_final) < 1e-12);
}

TEST_CASE("the stochastic integral agrees with the sde engine and Series", "[calculus][stochastic]") {
  const Path p = make_path(1000, 11);
  const math::Function f = math::function({"t", "W"}, [](math::Function t, math::Function w) { return w.sin() + t; });
  const auto integrand = [](double t, const pluss::Series &w) { return w.sin().add(t); };
  CHECK(std::abs(integrate(f, {}, p) - one(sde::ito_integral(integrand, p.raw).value)) < 1e-12);
  CHECK(std::abs(integrate(f, {.convention = stochastic::Convention::Stratonovich}, p) - one(sde::strat_integral(integrand, p.raw).value)) < 1e-12);
  // Stratonovich integral of sin(W) is 1 - cos(W_T) in the limit; Ito differs by half the integral of cos(W) dt
  const double strat = integrate(f, {.convention = stochastic::Convention::Stratonovich}, p);
  const double ito = integrate(f, {}, p);
  const std::vector<double> w = host(p.w), t = host(p.t);
  double cos_dt = 0.0;
  for (std::size_t i = 0; i + 1 < w.size(); ++i) cos_dt += std::cos(w[i]) * (t[i + 1] - t[i]);
  CHECK(std::abs((strat - ito) - 0.5 * cos_dt) < 3e-2);
}

TEST_CASE("cumulative integrals start at zero and end at the terminal value", "[calculus][stochastic]") {
  const Path p = make_path(500, 5);
  const math::Function f = noise_function();
  const auto running = host(stochastic::stochastic_integral("W", {.cumulative = true})(f)({p.t, p.w}));
  REQUIRE(running.size() == 501);
  CHECK(running.front() == 0.0);
  CHECK(std::abs(running.back() - integrate(f, {}, p)) < 1e-12);
}

TEST_CASE("Ito integrals are martingales with the isometry", "[calculus][stochastic][montecarlo]") {
  // 100000 independent paths: arguments are [B, N] with the path on the last axis
  const std::int64_t batch = 100000, steps = 50;
  const sde::WienerPath raw = sde::simulate_wiener_path(pluss::series::zeros({batch}, F64), 0.0, 1.0, steps, 99);
  const pluss::Series w = raw.w.transpose(0, 1).contiguous(); // [B, N]
  const pluss::Series t = pluss::make_series(raw.times, {steps + 1}).unsqueeze(0).expand({batch, steps + 1}).contiguous();
  const math::Function f = noise_function();
  const pluss::Series values = stochastic::stochastic_integral("W")(f)({t, w});
  REQUIRE(values.dim() == 1);
  REQUIRE(values.size(0) == batch);
  const double mean = values.mean().item<double>();
  const double second = values.square().mean().item<double>();
  CHECK(std::abs(mean) < 5.0 * 0.7071 / std::sqrt(static_cast<double>(batch)));
  CHECK(std::abs(second - 0.5) < 0.07 * 0.5); // E[(int W dW)^2] = T^2 / 2
  // the Stratonovich integral has mean E[W_T^2]/2 = T/2
  const pluss::Series strat = stochastic::stochastic_integral("W", {.convention = stochastic::Convention::Stratonovich})(f)({t, w});
  CHECK(std::abs(strat.mean().item<double>() - 0.5) < 5.0 * 0.7071 / std::sqrt(static_cast<double>(batch)));
}

TEST_CASE("quadratic variation and covariation", "[calculus][stochastic]") {
  const Path p = make_path(20000, 3);
  const math::Function w = noise_function();
  const double qv = one(stochastic::quadratic_variation()(w)({p.t, p.w}));
  CHECK(std::abs(qv - p.sum_dw2) < 1e-12);
  CHECK(std::abs(qv - one(sde::ito_integral([](double, const pluss::Series &x) { return x; }, p.raw).realized_quadratic_variation)) < 1e-12);
  CHECK(std::abs(qv - 1.0) < 6.0 * std::sqrt(2.0 / 20000.0));
  // covariation with an independent path vanishes; with itself it is the quadratic variation
  const Path q = make_path(20000, 4);
  const math::Function other = math::function({"t", "W"}, [](math::Function, math::Function x) { return x; });
  (void)other;
  const math::Function pair = math::function({"W", "V"}, [](math::Function x, math::Function) { return x; });
  const math::Function second = math::function({"W", "V"}, [](math::Function, math::Function y) { return y; });
  const double cross = one(stochastic::covariation(second)(pair)({p.w, q.w}));
  CHECK(std::abs(cross) < 6.0 / std::sqrt(20000.0));
  CHECK(std::abs(one(stochastic::covariation(w)(w)({p.t, p.w})) - qv) < 1e-12);
  // [int W dW] = int W^2 dt for the Ito integral process
  const math::Function integral_process = stochastic::stochastic_integral("W", {.cumulative = true})(w);
  const double process_qv = one(stochastic::quadratic_variation()(integral_process)({p.t, p.w}));
  const std::vector<double> wv = host(p.w), tv = host(p.t);
  double expected = 0.0;
  for (std::size_t i = 0; i + 1 < wv.size(); ++i) expected += wv[i] * wv[i] * (tv[i + 1] - tv[i]);
  CHECK(std::abs(process_qv - expected) < 0.05 * expected + 1e-3);
}

TEST_CASE("derivatives of stochastic integrals follow the pathwise rules", "[calculus][stochastic][derivative]") {
  const Path p = make_path(400, 21);
  const math::Function f = math::function({"a", "t", "W"}, [](math::Function a, math::Function, math::Function w) { return a * w; });
  const math::Function integral = stochastic::stochastic_integral("W")(f);
  const math::Function d_a = operators::differential("a")(integral);
  const double ito = integrate(noise_function(), {}, p);
  CHECK(std::abs(one(d_a({scalar(2.0), p.t, p.w})) - ito) < 1e-12);
  CHECK(std::abs(one(integral({scalar(2.0), p.t, p.w})) - 2.0 * ito) < 1e-12);
  // the noise itself is not an ordinary variable of differentiation
  CHECK_THROWS_AS(operators::differential("W")(integral), pluss::Error);
  try {
    (void)operators::differential("W")(integral);
  } catch (const pluss::Error &error) {
    CHECK(std::string(error.what()).find("Malliavin") != std::string::npos);
  }
}

TEST_CASE("path functionals validate their arguments", "[calculus][stochastic][errors]") {
  const math::Function f = noise_function();
  CHECK_THROWS_AS(stochastic::stochastic_integral("missing")(f), pluss::Error);
  CHECK_THROWS_AS(stochastic::stochastic_integral("W", {.convention = stochastic::Convention::Theta, .theta = 1.5}), pluss::Error);
  const math::Function opaque = math::function({"W"}, [](const pluss::Series &x) { return x.mul(2.0); });
  CHECK_THROWS_AS(stochastic::stochastic_integral("W")(opaque), pluss::Error);
  const math::Function vector = math::function({"t", "W"}, [](math::Function t, math::Function w) { return math::stack({t, w}); });
  CHECK_THROWS_AS(stochastic::stochastic_integral("W")(vector), pluss::Error);
  const Path p = make_path(10, 1);
  CHECK_THROWS_AS(stochastic::stochastic_integral("W", {.path_axis = 3})(f)({p.t, p.w}), pluss::Error);
  CHECK(stochastic::stochastic_integral("W").capabilities().path_functional);
  CHECK(stochastic::stochastic_integral("W").capabilities().linear);
  CHECK_FALSE(stochastic::quadratic_variation().capabilities().linear);
  CHECK(operators::compose(stochastic::stochastic_integral("W"), operators::scale(2.0)).capabilities().path_functional);
}

TEST_CASE("stochastic functions round-trip through serialization", "[calculus][stochastic][io]") {
  const Path p = make_path(300, 8);
  const math::Function f = math::function({"t", "W"}, [](math::Function t, math::Function w) { return w * w + t; });
  const math::Function integral = stochastic::stochastic_integral("W", {.convention = stochastic::Convention::Theta, .theta = 0.25, .cumulative = true})(f);
  std::stringstream stream;
  math::io::save(integral, stream);
  const math::Function restored = math::io::load_function(stream);
  const auto a = host(integral({p.t, p.w})), b = host(restored({p.t, p.w}));
  REQUIRE(a.size() == b.size());
  for (std::size_t i = 0; i < a.size(); ++i) CHECK(std::abs(a[i] - b[i]) < 1e-14);
}

// ---- structural stochastic algebra -----------------------------------------------------------------------

TEST_CASE("the generator of one-dimensional diffusions", "[calculus][stochastic][generator]") {
  const double theta = 1.3, m = 0.7, sigma = 0.4;
  const math::Function drift = math::function({"x"}, [&](math::Function x) { return (m - x) * theta; });
  const math::Function diffusion = math::function({"x"}, [&](math::Function x) { return x * 0.0 + sigma; });
  const operators::Operator L = stochastic::generator(drift, diffusion, {"x"});
  const math::Function identity = math::function({"x"}, [](math::Function x) { return x; });
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x; });
  const double x = 0.9;
  CHECK(std::abs(at(L(identity), {x}) - theta * (m - x)) < 1e-12);               // Ornstein-Uhlenbeck
  CHECK(std::abs(at(L(square), {x}) - (2.0 * theta * x * (m - x) + sigma * sigma)) < 1e-12);
  // geometric Brownian motion: L x^p = (p a + p(p-1) b^2 / 2) x^p
  const double a = 0.3, b = 0.5, p = 3.0;
  const operators::Operator G = stochastic::generator(
      math::function({"x"}, [&](math::Function y) { return y * a; }),
      math::function({"x"}, [&](math::Function y) { return y * b; }), {"x"});
  const math::Function power = math::function({"x"}, [&](math::Function y) { return y.pow(p); });
  CHECK(std::abs(at(G(power), {x}) - (p * a + 0.5 * p * (p - 1.0) * b * b) * std::pow(x, p)) < 1e-12);
  // linear, and composes with the other operators (the generator applied twice is a fourth-order polynomial operator)
  CHECK(L.capabilities().linear);
  CHECK(std::abs(at(operators::compose(L, L)(identity), {x}) - (-theta) * theta * (m - x)) < 1e-12);
}

TEST_CASE("the generator of a two-dimensional diffusion", "[calculus][stochastic][generator]") {
  // dX = A X dt + S dW with constant matrices: L(x1 x2) = (A x)_1 x2 + (A x)_2 x1 + (S S^T)_12
  const math::Function drift = math::function({"x1", "x2"}, [](math::Function x1, math::Function x2) {
    return math::stack({x1 * (-1.0) + x2 * 0.5, x1 * 0.25 + x2 * (-2.0)});
  });
  const math::Function diffusion = math::function({"x1", "x2"}, [](math::Function x1, math::Function x2) {
    const math::Function zero = x1 * 0.0 + x2 * 0.0;
    return math::stack({math::stack({zero + 0.6, zero + 0.1}), math::stack({zero, zero + 0.3})}); // columns: [[0.6, 0], [0.1, 0.3]]
  });
  const operators::Operator L = stochastic::generator(drift, diffusion, {"x1", "x2"});
  const math::Function product = math::function({"x1", "x2"}, [](math::Function x1, math::Function x2) { return x1 * x2; });
  const double x1 = 0.8, x2 = -0.4;
  // S = [[0.6, 0], [0.1, 0.3]] (rows), S S^T = [[0.36, 0.06], [0.06, 0.10]]
  const double expected = (-x1 + 0.5 * x2) * x2 + (0.25 * x1 - 2.0 * x2) * x1 + 0.06;
  CHECK(std::abs(at(L(product), {x1, x2}) - expected) < 1e-12);
  CHECK_THROWS_AS(stochastic::generator(drift, diffusion, {"x1"}), pluss::Error);
}

TEST_CASE("stochastic differentials: Ito's formula, the chain rule and the theta family", "[calculus][stochastic][differential]") {
  const double a = 0.3, b = 0.5;
  const math::Function drift = math::function({"x"}, [&](math::Function x) { return x * a; });
  const math::Function diffusion = math::function({"x"}, [&](math::Function x) { return x * b; });
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x; });
  const double x = 1.4;
  const auto coefficients = [&](stochastic::Convention c, double theta = 0.0) {
    return host(stochastic::stochastic_differential(drift, diffusion, {"x"}, c, theta)(square)({scalar(x)}));
  };
  const auto ito = coefficients(stochastic::Convention::Ito);
  CHECK(std::abs(ito[0] - (2.0 * x * a * x + b * b * x * x)) < 1e-12); // g' mu + sigma^2 g'' / 2, g = x^2
  CHECK(std::abs(ito[1] - 2.0 * x * b * x) < 1e-12);
  const auto strat = coefficients(stochastic::Convention::Stratonovich);
  CHECK(std::abs(strat[0] - 2.0 * x * a * x) < 1e-12);                 // ordinary chain rule
  CHECK(std::abs(strat[1] - ito[1]) < 1e-12);
  const auto klim = coefficients(stochastic::Convention::Klimontovich);
  CHECK(std::abs(klim[0] - (2.0 * x * a * x - b * b * x * x)) < 1e-12);
  const auto theta = coefficients(stochastic::Convention::Theta, 0.25);
  CHECK(std::abs(theta[0] - (2.0 * x * a * x + (0.5 - 0.25) * 2.0 * b * b * x * x)) < 1e-12); // (1/2 - theta) g'' sigma^2
  // convention_change moves the drift between readings: Ito drift = Stratonovich drift + sigma sigma' / 2
  const operators::Operator to_ito = stochastic::convention_change(0.5, 0.0, diffusion, {"x"});
  CHECK(std::abs(at(to_ito(drift), {x}) - (a * x + 0.5 * b * b * x)) < 1e-12);
  const operators::Operator back = stochastic::convention_change(0.0, 0.5, diffusion, {"x"});
  CHECK(std::abs(at(back(to_ito(drift)), {x}) - a * x) < 1e-12);
  // consistency: Ito's formula applied to the converted drift equals the Stratonovich rule applied to the original
  // drift, after converting g's own drift back (g(X) in the Stratonovich reading has drift a_strat)
  const math::Function ito_drift = to_ito(drift);
  const auto from_ito = host(stochastic::stochastic_differential(ito_drift, diffusion, {"x"}, stochastic::Convention::Ito)(square)({scalar(x)}));
  // the Ito drift of Y = X^2 is a_I; its Stratonovich drift is a_I - (1/2) b_Y db_Y/dy with b_Y = 2 x sigma(x) and y = x^2
  const double b_y = 2.0 * x * b * x, db_dx = 2.0 * b * x + 2.0 * x * b, db_dy = db_dx / (2.0 * x);
  CHECK(std::abs((from_ito[0] - 0.5 * b_y * db_dy) - strat[0]) < 1e-10);
}

TEST_CASE("multi-dimensional stochastic differentials and convention change", "[calculus][stochastic][differential]") {
  const math::Function drift = math::function({"x1", "x2"}, [](math::Function x1, math::Function x2) { return math::stack({x1 * 0.2, x2 * (-0.1)}); });
  const math::Function diffusion = math::function({"x1", "x2"}, [](math::Function x1, math::Function x2) {
    // sigma = [[0.5 x1, 0], [0.1 x1, 0.3 x2]] written by columns
    return math::stack({math::stack({x1 * 0.5, x1 * 0.1}), math::stack({x1 * 0.0, x2 * 0.3})});
  });
  const math::Function g = math::function({"x1", "x2"}, [](math::Function x1, math::Function x2) { return x1 * x2; });
  const double x1 = 1.1, x2 = 0.7;
  const std::vector<double> coefficients = host(stochastic::stochastic_differential(drift, diffusion, {"x1", "x2"})(g)({scalar(x1), scalar(x2)}));
  REQUIRE(coefficients.size() == 3);
  // Ito: a = x2 mu1 + x1 mu2 + (S S^T)_12 ; b = S^T grad g with grad g = (x2, x1)
  const double s11 = 0.5 * x1, s21 = 0.1 * x1, s22 = 0.3 * x2;
  CHECK(std::abs(coefficients[0] - (x2 * 0.2 * x1 + x1 * (-0.1) * x2 + s11 * s21)) < 1e-12);
  CHECK(std::abs(coefficients[1] - (s11 * x2 + s21 * x1)) < 1e-12);
  CHECK(std::abs(coefficients[2] - s22 * x1) < 1e-12);
  // c_i = sum_{j,k} sigma_jk d_j sigma_ik : c_1 = s11 * 0.5 (d1 s11 = 0.5), c_2 = s11 * 0.1 (d1 s21) + s22 * 0.3 (d2 s22)
  const operators::Operator to_ito = stochastic::convention_change(0.5, 0.0, diffusion, {"x1", "x2"});
  const std::vector<double> converted = host(to_ito(drift)({scalar(x1), scalar(x2)}));
  CHECK(std::abs(converted[0] - (0.2 * x1 + 0.5 * (s11 * 0.5))) < 1e-12);
  CHECK(std::abs(converted[1] - (-0.1 * x2 + 0.5 * (s11 * 0.1 + s22 * 0.3))) < 1e-12);
}

TEST_CASE("an Ito SDE and its Stratonovich reading agree after the drift conversion", "[calculus][stochastic][montecarlo]") {
  // dX = a X dt + b X o dW (Stratonovich): X_T = X0 exp(a T + b W_T), so E[X_T] = X0 exp((a + b^2/2) T)
  const double a = 0.1, b = 0.4, x0 = 1.0;
  const math::Function drift = math::function({"x"}, [&](math::Function x) { return x * a; });
  const math::Function diffusion = math::function({"x"}, [&](math::Function x) { return x * b; });
  const math::Function ito_drift = stochastic::convention_change(0.5, 0.0, diffusion, {"x"})(drift);
  const std::int64_t paths = 40000;
  const pluss::Series y0 = pluss::series::full({paths}, x0, pluss::Config{}.with_dtype(F64));
  const auto drift_of = [](const math::Function &f) {
    return [f](double, const pluss::Series &y, const pluss::Series &) { return f({y}); };
  };
  sde::SdeOptions options;
  options.steps = 200;
  options.seed = 123;
  options.method = sde::SdeMethod::StratonovichHeun;
  const double strat_mean = sde::fluctua(drift_of(drift), drift_of(diffusion), y0, 0.0, 1.0, pluss::series::undefined(), options).y_final.mean().item<double>();
  options.method = sde::SdeMethod::EulerMaruyama;
  options.seed = 456;
  const double ito_mean = sde::fluctua(drift_of(ito_drift), drift_of(diffusion), y0, 0.0, 1.0, pluss::series::undefined(), options).y_final.mean().item<double>();
  const double exact = x0 * std::exp(a + 0.5 * b * b);
  CHECK(std::abs(strat_mean - exact) < 0.02);
  CHECK(std::abs(ito_mean - exact) < 0.02);
  // without the conversion the Ito reading of the same coefficients is a different process
  const double wrong_mean = sde::fluctua(drift_of(drift), drift_of(diffusion), y0, 0.0, 1.0, pluss::series::undefined(), options).y_final.mean().item<double>();
  CHECK(std::abs(wrong_mean - x0 * std::exp(a)) < 0.02);
  CHECK(std::abs(wrong_mean - exact) > 0.03);
}

TEST_CASE("operators that hold only numbers and names serialize", "[calculus][stochastic][io]") {
  const Path p = make_path(100, 31);
  const math::Function f = noise_function();
  const auto round_trip = [](const operators::Operator &op) {
    std::stringstream stream;
    math::io::save(op, stream);
    return math::io::load_operator(stream);
  };
  const operators::Operator integral = stochastic::stochastic_integral("W", {.convention = stochastic::Convention::Theta, .theta = 0.3, .cumulative = true});
  const operators::Operator restored = round_trip(integral);
  CHECK(restored.key() == "stochastic_integral");
  CHECK(restored.capabilities().path_functional);
  const auto a = host(integral(f)({p.t, p.w})), b = host(restored(f)({p.t, p.w}));
  REQUIRE(a.size() == b.size());
  for (std::size_t i = 0; i < a.size(); ++i) CHECK(a[i] == b[i]);
  CHECK(std::abs(one(round_trip(stochastic::quadratic_variation())(f)({p.t, p.w})) - p.sum_dw2) < 1e-12);
  // operators that hold Functions are not serializable
  const math::Function drift = math::function({"x"}, [](math::Function x) { return x; });
  std::stringstream refused;
  CHECK_THROWS(math::io::save(stochastic::generator(drift, drift, {"x"}), refused));
  CHECK_THROWS(math::io::save(stochastic::covariation(f), refused));
}
