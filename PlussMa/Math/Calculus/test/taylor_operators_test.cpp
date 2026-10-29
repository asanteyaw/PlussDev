#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <sstream>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace expansion = pluss::operators::expansion;

namespace {

pluss::Series scalar(double value) {
  return pluss::make_series(std::vector<double>{value}, {1}).reshape({});
}
std::vector<double> host(const pluss::Series &value) {
  const pluss::Series cpu = value.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous();
  return cpu.reshape({cpu.numel()}).host<double>();
}
double at(const math::Function &f, std::initializer_list<double> point, std::size_t index = 0) {
  std::vector<pluss::Series> arguments;
  for (const double v : point) arguments.push_back(scalar(v));
  return host(f(pluss::RangeView<pluss::Series>(arguments)))[index];
}
double factorial(int n) { double v = 1.0; for (int k = 2; k <= n; ++k) v *= k; return v; }

math::Function exp_function() { return math::function({"x"}, [](math::Function x) { return x.exp(); }); }

} // namespace

TEST_CASE("Taylor polynomials of the elementary functions", "[calculus][taylor]") {
  // exp at 0 and at 1
  const math::Function exp_at_0 = expansion::taylor("x", 0.0, 6)(exp_function());
  const math::Function exp_at_1 = expansion::taylor("x", 1.0, 6)(exp_function());
  for (const double x : {-0.7, 0.3, 1.1}) {
    double series = 0.0;
    for (int k = 0; k <= 6; ++k) series += std::pow(x, k) / factorial(k);
    CHECK(std::abs(at(exp_at_0, {x}) - series) < 1e-13);
    double shifted = 0.0;
    for (int k = 0; k <= 6; ++k) shifted += std::exp(1.0) * std::pow(x - 1.0, k) / factorial(k);
    CHECK(std::abs(at(exp_at_1, {x}) - shifted) < 1e-12);
  }
  // sin: odd terms only; log1p: (-1)^(k+1) x^k / k; (1 + x)^p: binomial coefficients
  const math::Function sine = expansion::taylor("x", 0.0, 7)(math::function({"x"}, [](math::Function x) { return x.sin(); }));
  CHECK(std::abs(at(sine, {0.4}) - (0.4 - std::pow(0.4, 3) / 6.0 + std::pow(0.4, 5) / 120.0 - std::pow(0.4, 7) / 5040.0)) < 1e-14);
  const math::Function log1p = expansion::taylor_coefficients("x", 0.0, 5)(math::function({"x"}, [](math::Function x) { return x.log1p(); }));
  const std::vector<double> c = host(log1p());
  REQUIRE(c.size() == 6);
  CHECK(std::abs(c[0]) < 1e-14);
  for (int k = 1; k <= 5; ++k) CHECK(std::abs(c[static_cast<std::size_t>(k)] - (k % 2 ? 1.0 : -1.0) / k) < 1e-13);
  const math::Function binomial = expansion::taylor_coefficients("x", 0.0, 4)(math::function({"x"}, [](math::Function x) { return (x + 1.0).pow(2.5); }));
  const std::vector<double> b = host(binomial());
  const double expected[] = {1.0, 2.5, 2.5 * 1.5 / 2.0, 2.5 * 1.5 * 0.5 / 6.0, 2.5 * 1.5 * 0.5 * -0.5 / 24.0};
  for (std::size_t k = 0; k < 5; ++k) CHECK(std::abs(b[k] - expected[k]) < 1e-13);
}

TEST_CASE("Taylor polynomials reproduce polynomials and respect derivatives", "[calculus][taylor]") {
  const math::Function p = math::function({"x"}, [](math::Function x) { return x.pow(3) - x * 2.0 + 1.0; });
  const math::Function t = expansion::taylor("x", 0.7, 4)(p); // degree 3 <= order: exact
  for (const double x : {-1.3, 0.0, 0.7, 2.2}) CHECK(std::abs(at(t, {x}) - at(p, {x})) < 1e-12);
  // d/dx T_n[f] = T_{n-1}[f'] 
  const math::Function f = math::function({"x"}, [](math::Function x) { return (x * x).sin() + x.exp(); });
  const math::Function lhs = operators::differential("x")(expansion::taylor("x", 0.2, 5)(f));
  const math::Function rhs = expansion::taylor("x", 0.2, 4)(operators::differential("x")(f));
  for (const double x : {-0.5, 0.1, 0.9}) CHECK(std::abs(at(lhs, {x}) - at(rhs, {x})) < 1e-11);
}

TEST_CASE("the Taylor remainder bounds the error", "[calculus][taylor]") {
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.sin() + x.cos() * 0.5; });
  const std::int64_t order = 5;
  const double a = 0.3;
  const math::Function t = expansion::taylor("x", a, order)(f);
  const math::Function factor = expansion::taylor_remainder("x", order)(f);
  double previous = 0.0;
  for (const double h : {0.4, 0.2, 0.1}) {
    const double error = std::abs(at(f, {a + h}) - at(t, {a + h}));
    // Lagrange: the factor is f^(n+1)/(n+1)!; |f^(6)| <= 1.5 for this f, so the bound is 1.5/720 h^6
    CHECK(error <= 1.5 / factorial(6) * std::pow(h, 6) * 1.0000001);
    CHECK(std::abs(at(factor, {a + 0.5 * h})) <= 1.5 / factorial(6) * 1.0000001);
    if (previous > 0.0) CHECK(error < previous / 30.0); // h -> h/2 divides the error by about 2^6 = 64
    previous = error;
  }
}

TEST_CASE("multivariable Taylor polynomials", "[calculus][taylor]") {
  const math::Function e = math::function({"x", "y"}, [](math::Function x, math::Function y) { return (x + y).exp(); });
  const math::Function t = expansion::taylor({"x", "y"}, {0.0, 0.0}, 3)(e);
  const double x = 0.3, y = -0.2;
  double series = 0.0;
  for (int i = 0; i <= 3; ++i)
    for (int j = 0; i + j <= 3; ++j) series += std::pow(x, i) * std::pow(y, j) / (factorial(i) * factorial(j));
  CHECK(std::abs(at(t, {x, y}) - series) < 1e-13);
  // a polynomial of total degree <= order is exact, with a centre away from the origin
  const math::Function q = math::function({"x", "y", "z"}, [](math::Function a, math::Function b, math::Function c) { return a * b * c + a * a - b; });
  const math::Function qt = expansion::taylor({"x", "y", "z"}, {0.5, -1.0, 2.0}, 3)(q);
  CHECK(std::abs(at(qt, {1.2, 0.4, -0.7}) - at(q, {1.2, 0.4, -0.7})) < 1e-12);
  // order 1 is the tangent plane
  const math::Function plane = expansion::taylor({"x", "y"}, {1.0, 2.0}, 1)(math::function({"x", "y"}, [](math::Function a, math::Function b) { return a * a * b; }));
  CHECK(std::abs(at(plane, {1.5, 2.5}) - (2.0 + 2.0 * 2.0 * 0.5 + 1.0 * 0.5)) < 1e-12);
}

TEST_CASE("free variables stay free in the expansion", "[calculus][taylor]") {
  const math::Function f = math::function({"x", "a"}, [](math::Function x, math::Function a) { return a * x.exp(); });
  const math::Function t = expansion::taylor("x", 0.0, 4)(f);
  REQUIRE(t.arity() == 2);
  CHECK(std::abs(at(t, {0.5, 3.0}) - 3.0 * (1.0 + 0.5 + 0.125 + 0.5 * 0.5 * 0.5 / 6.0 + std::pow(0.5, 4) / 24.0)) < 1e-12);
  const math::Function coefficients = expansion::taylor_coefficients("x", 0.0, 3)(f);
  REQUIRE(coefficients.arity() == 1); // the expanded variable is bound; the coefficients depend on a only
  const std::vector<double> c = host(coefficients({scalar(2.0)}));
  CHECK(std::abs(c[2] - 1.0) < 1e-12); // a / 2!
}

TEST_CASE("Taylor operators validate, compose, serialize", "[calculus][taylor][io]") {
  CHECK_THROWS_AS(expansion::taylor("x", 0.0, -1), pluss::Error);
  CHECK_THROWS_AS(expansion::taylor("x", INFINITY, 2), pluss::Error);
  CHECK_THROWS_AS(expansion::taylor("nope", 0.0, 2)(exp_function()), pluss::Error);
  CHECK_THROWS_AS(expansion::taylor({"x", "y"}, {0.0}, 2), pluss::Error);
  // log at 0 is outside the domain: the failure is a domain error when the coefficients are evaluated
  const math::Function log_taylor = expansion::taylor("x", 0.0, 2)(math::function({"x"}, [](math::Function x) { return x.log(); }));
  CHECK_THROWS_AS(at(log_taylor, {0.5}), pluss::Error);
  // composes with other operators: the integral of the expansion is the expansion of the integral
  const operators::Operator integrate_taylor = operators::compose(operators::antiderivative("x"), expansion::taylor("x", 0.0, 4));
  const math::Function integrated = integrate_taylor(exp_function());
  double series = 0.0;
  for (int k = 0; k <= 4; ++k) series += std::pow(0.8, k + 1) / factorial(k + 1);
  CHECK(std::abs(at(integrated, {0.8}) - series) < 1e-12);
  // operator and result both round-trip
  const operators::Operator op = expansion::taylor("x", 0.4, 5, {.nonsmooth = math::NonSmooth::Piecewise, .max_nodes = 5000});
  std::stringstream operator_stream;
  math::io::save(op, operator_stream);
  const operators::Operator restored = math::io::load_operator(operator_stream);
  CHECK(restored.key() == "taylor");
  CHECK(at(restored(exp_function()), {0.9}) == at(op(exp_function()), {0.9}));
  const operators::Operator multi = expansion::taylor({"x", "y"}, {0.1, 0.2}, 2);
  std::stringstream multi_stream;
  math::io::save(multi, multi_stream);
  const math::Function e = math::function({"x", "y"}, [](math::Function x, math::Function y) { return (x * y).exp(); });
  CHECK(at(math::io::load_operator(multi_stream)(e), {0.3, 0.5}) == at(multi(e), {0.3, 0.5}));
  std::stringstream function_stream;
  const math::Function polynomial = op(exp_function());
  math::io::save(polynomial, function_stream);
  CHECK(std::abs(at(math::io::load_function(function_stream), {0.6}) - at(polynomial, {0.6})) < 1e-15);
  const operators::Operator coefficients = expansion::taylor_coefficients("x", 0.0, 3);
  std::stringstream c_stream;
  math::io::save(coefficients, c_stream);
  CHECK(math::io::load_operator(c_stream).key() == "taylor_coefficients");
  const operators::Operator remainder = expansion::taylor_remainder("x", 3);
  std::stringstream r_stream;
  math::io::save(remainder, r_stream);
  CHECK(math::io::load_operator(r_stream).key() == "taylor_remainder");
}

// ---- the power-series (jet) engine and coefficient-vector operators ------------------------------------

TEST_CASE("the power-series engine reproduces known series to high order", "[calculus][taylor][series]") {
  const auto coefficients = [](const math::Function &f, std::int64_t order, double center = 0.0) {
    return host(expansion::taylor_coefficients("x", center, order, {}, expansion::Engine::Jet)(f)());
  };
  // exp, order 30
  const std::vector<double> e = coefficients(exp_function(), 30);
  REQUIRE(e.size() == 31);
  for (int k = 0; k <= 30; ++k) CHECK(std::abs(e[static_cast<std::size_t>(k)] * factorial(k) - 1.0) < 1e-9);
  // geometric series 1 / (1 - x): every coefficient is 1
  const std::vector<double> g = coefficients(math::function({"x"}, [](math::Function x) { return 1.0 / (1.0 - x); }), 20);
  for (const double c : g) CHECK(std::abs(c - 1.0) < 1e-12);
  // tan and tanh
  const std::vector<double> t = coefficients(math::function({"x"}, [](math::Function x) { return x.tan(); }), 9);
  const double tan_expected[] = {0, 1, 0, 1.0 / 3, 0, 2.0 / 15, 0, 17.0 / 315, 0, 62.0 / 2835};
  for (std::size_t k = 0; k < 10; ++k) CHECK(std::abs(t[k] - tan_expected[k]) < 1e-13);
  const std::vector<double> th = coefficients(math::function({"x"}, [](math::Function x) { return x.tanh(); }), 9);
  for (std::size_t k = 0; k < 10; ++k) CHECK(std::abs(th[k] - (k % 4 == 3 ? -1.0 : 1.0) * tan_expected[k]) < 1e-13);
  // sqrt(1 + x) and (1 + x)^(-1/2): binomial series
  const std::vector<double> s = coefficients(math::function({"x"}, [](math::Function x) { return (x + 1.0).sqrt(); }), 6);
  const double sqrt_expected[] = {1, 0.5, -0.125, 0.0625, -0.0390625, 0.02734375, -0.0205078125};
  for (std::size_t k = 0; k < 7; ++k) CHECK(std::abs(s[k] - sqrt_expected[k]) < 1e-13);
  // a composition: exp(sin x) = 1 + x + x^2/2 - x^4/8 - x^5/15 - x^6/240 + ...
  const std::vector<double> c = coefficients(math::function({"x"}, [](math::Function x) { return x.sin().exp(); }), 6);
  const double composite[] = {1, 1, 0.5, 0, -0.125, -1.0 / 15, -1.0 / 240};
  for (std::size_t k = 0; k < 7; ++k) CHECK(std::abs(c[k] - composite[k]) < 1e-13);
  // log(1 + x) via log1p and log, centred away from the origin
  const std::vector<double> l = coefficients(math::function({"x"}, [](math::Function x) { return x.log(); }), 5, 2.0);
  CHECK(std::abs(l[0] - std::log(2.0)) < 1e-14);
  for (int k = 1; k <= 5; ++k) CHECK(std::abs(l[static_cast<std::size_t>(k)] - (k % 2 ? 1.0 : -1.0) / (k * std::pow(2.0, k))) < 1e-13);
}

TEST_CASE("the power-series engine agrees with the derivative graph", "[calculus][taylor][series]") {
  const math::Function f = math::function({"x"}, [](math::Function x) { return (x * x + 1.0).log() * x.cos() + x.exp() / (x + 3.0); });
  const auto structural = host(expansion::taylor_coefficients("x", 0.4, 7, {}, expansion::Engine::Structural)(f)());
  const auto jet = host(expansion::taylor_coefficients("x", 0.4, 7, {}, expansion::Engine::Jet)(f)());
  REQUIRE(structural.size() == jet.size());
  for (std::size_t k = 0; k < jet.size(); ++k) CHECK(std::abs(structural[k] - jet[k]) < 1e-11);
  // the polynomial built from jet coefficients is the same polynomial
  const math::Function p_structural = expansion::taylor("x", 0.4, 7, {}, expansion::Engine::Structural)(f);
  const math::Function p_jet = expansion::taylor("x", 0.4, 7, {}, expansion::Engine::Jet)(f);
  for (const double x : {-0.3, 0.2, 0.9}) CHECK(std::abs(at(p_structural, {x}) - at(p_jet, {x})) < 1e-11);
  // Auto switches to the jet engine above order 8, where the derivative graph would be large
  const math::Function high = expansion::taylor("x", 0.0, 25)(exp_function());
  double series = 0.0;
  for (int k = 0; k <= 25; ++k) series += std::pow(0.9, k) / factorial(k);
  CHECK(std::abs(at(high, {0.9}) - series) < 1e-12);
  CHECK(high.provenance() == math::Provenance::Structural); // the jet engine builds an analytic graph
  std::stringstream saved;
  math::io::save(high, saved);
  CHECK(std::abs(at(math::io::load_function(saved), {0.9}) - series) < 1e-12);
  CHECK(std::abs(at(operators::differential("x")(high), {0.9}) - at(expansion::taylor("x", 0.0, 24)(exp_function()), {0.9})) < 1e-12);
  CHECK(expansion::taylor("x", 0.0, 4)(exp_function()).provenance() == math::Provenance::Structural);
}

TEST_CASE("jets are batched over the free variables", "[calculus][taylor][series]") {
  const math::Function f = math::function({"x", "a"}, [](math::Function x, math::Function a) { return a * (a * x).exp(); });
  const pluss::Series a = pluss::make_series(std::vector<double>{1.0, 2.0, 0.5}, {3});
  // the expanded variable is replaced by its centre, so the coefficients depend on `a` only
  const pluss::Series c = expansion::taylor_coefficients("x", 0.0, 6, {}, expansion::Engine::Jet)(f)({a});
  REQUIRE(c.dim() == 2);
  REQUIRE(c.size(0) == 3);
  REQUIRE(c.size(1) == 7);
  const std::vector<double> flat = host(c);
  const double av[] = {1.0, 2.0, 0.5};
  for (int row = 0; row < 3; ++row)
    for (int k = 0; k <= 6; ++k) CHECK(std::abs(flat[static_cast<std::size_t>(row * 7 + k)] - std::pow(av[row], k + 1) / factorial(k)) < 1e-12);
}

TEST_CASE("operators on coefficient vectors", "[calculus][taylor][series]") {
  const math::Function sine = math::function({"x"}, [](math::Function x) { return x.sin(); });
  const math::Function cosine = math::function({"x"}, [](math::Function x) { return x.cos(); });
  const auto coefficients_of = [](const math::Function &f, std::int64_t order) { return expansion::taylor_coefficients("x", 0.0, order, {}, expansion::Engine::Jet)(f); };
  // evaluating the coefficients gives back the Taylor polynomial
  CHECK(std::abs(at(expansion::power_series("x", 0.0)(coefficients_of(sine, 9)), {0.6}) - at(expansion::taylor("x", 0.0, 9)(sine), {0.6})) < 1e-13);
  // termwise derivative of the sine series is the cosine series (one coefficient fewer)
  const std::vector<double> d = host(expansion::series_derivative()(coefficients_of(sine, 9))());
  const std::vector<double> cos_c = host(coefficients_of(cosine, 8)());
  REQUIRE(d.size() == 9);
  for (std::size_t k = 0; k < d.size(); ++k) CHECK(std::abs(d[k] - cos_c[k]) < 1e-13);
  // termwise integral of the cosine series is the sine series (one more), with a zero constant term
  const std::vector<double> integral = host(expansion::series_integral()(coefficients_of(cosine, 8))());
  const std::vector<double> sin_c = host(coefficients_of(sine, 9)());
  REQUIRE(integral.size() == 10);
  for (std::size_t k = 0; k < integral.size(); ++k) CHECK(std::abs(integral[k] - sin_c[k]) < 1e-13);
  // Cauchy product: sin x cos x = sin(2x) / 2
  const math::Function sin2 = math::function({"x"}, [](math::Function x) { return (x * 2.0).sin() * 0.5; });
  const std::vector<double> product = host(expansion::series_product(coefficients_of(cosine, 9))(coefficients_of(sine, 9))());
  const std::vector<double> expected = host(coefficients_of(sin2, 9)());
  for (std::size_t k = 0; k < product.size(); ++k) CHECK(std::abs(product[k] - expected[k]) < 1e-12);
  // exp(x) exp(-x) = 1
  const math::Function inverse = math::function({"x"}, [](math::Function x) { return (x * -1.0).exp(); });
  const std::vector<double> one_series = host(expansion::series_product(coefficients_of(inverse, 12))(coefficients_of(exp_function(), 12))());
  CHECK(std::abs(one_series[0] - 1.0) < 1e-14);
  for (std::size_t k = 1; k < one_series.size(); ++k) CHECK(std::abs(one_series[k]) < 1e-12);
  CHECK_THROWS_AS(expansion::series_derivative()(exp_function()), pluss::Error); // not a coefficient vector
  // these three save; the product holds a Function and does not
  std::stringstream stream;
  math::io::save(expansion::power_series("x", 0.25), stream);
  CHECK(math::io::load_operator(stream).key() == "power_series");
  std::stringstream refused;
  CHECK_THROWS(math::io::save(expansion::series_product(coefficients_of(sine, 3)), refused));
}

TEST_CASE("the power-series engine rejects what it cannot do", "[calculus][taylor][series][errors]") {
  const math::Function vector = math::function({"x"}, [](math::Function x) { return math::stack({x, x * x}); });
  CHECK_THROWS_AS(expansion::taylor_coefficients("x", 0.0, 3, {}, expansion::Engine::Jet)(vector), pluss::Error);
  const math::Function opaque = math::function({"x"}, [](const pluss::Series &x) { return x.mul(2.0); });
  CHECK_THROWS_AS(expansion::taylor_coefficients("x", 0.0, 3, {}, expansion::Engine::Jet)(opaque), pluss::Error);
  const math::Function reciprocal = math::function({"x"}, [](math::Function x) { return 1.0 / x; });
  CHECK_THROWS_AS(at(expansion::taylor_coefficients("x", 0.0, 3, {}, expansion::Engine::Jet)(reciprocal), {0.0}), pluss::Error);
  CHECK_THROWS_AS(at(expansion::taylor_coefficients("x", -1.0, 3, {}, expansion::Engine::Jet)(math::function({"x"}, [](math::Function x) { return x.log(); })), {0.0}), pluss::Error);
}
