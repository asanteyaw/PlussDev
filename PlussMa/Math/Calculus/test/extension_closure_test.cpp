// Tranches 14-16: vector calculus, calculus of variations, adjoints, fractional calculus, convolution,
// and the serialization round trip of every new factory.
#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <sstream>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace fractional = pluss::operators::fractional;
namespace variational = pluss::operators::variational;
namespace stochastic = pluss::operators::stochastic;
namespace measure = pluss::operators::measure;

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kInf = std::numeric_limits<double>::infinity();

pluss::Series scalar(double value) { return pluss::make_series(std::vector<double>{value}, {1}).reshape({}); }
std::vector<double> evaluate(const math::Function &f, std::initializer_list<double> point) {
  std::vector<pluss::Series> arguments;
  for (const double v : point) arguments.push_back(scalar(v));
  const pluss::Series out = f(pluss::RangeView<pluss::Series>(arguments)).to(pluss::pCPU, pluss::ScalarType::Float64).contiguous();
  return out.reshape({out.numel()}).host<double>();
}
double at(const math::Function &f, std::initializer_list<double> point) { return evaluate(f, point)[0]; }
double at0(const math::Function &f) {
  const pluss::Series out = f().to(pluss::pCPU, pluss::ScalarType::Float64).contiguous();
  return out.reshape({out.numel()}).host<double>()[0];
}

math::Function xyz(std::function<math::Function(math::Function, math::Function, math::Function)> body) {
  return math::function({"x", "y", "z"}, [body](math::Function x, math::Function y, math::Function z) { return body(x, y, z); });
}

} // namespace

TEST_CASE("divergence, curl and laplacian satisfy the vector identities", "[calculus][extension][vector]") {
  const auto F = xyz([](math::Function x, math::Function y, math::Function z) { return math::stack({x * x * y, y * z, z * x}); });
  CHECK(std::abs(at(operators::divergence({"x", "y", "z"})(F), {1.5, -0.4, 0.7}) - (2 * 1.5 * -0.4 + 0.7 + 1.5)) < 1e-13);
  const auto f = xyz([](math::Function x, math::Function y, math::Function z) { return x * x * y * z + (x * z).sin(); });
  const auto grad = operators::gradient({"x", "y", "z"})(f);
  for (const double v : evaluate(operators::curl({"x", "y", "z"})(grad), {0.3, 0.9, -1.2})) CHECK(std::abs(v) < 1e-12);  // curl grad = 0
  const auto G = xyz([](math::Function x, math::Function y, math::Function z) { return math::stack({y * z * z, x * x * z, x * y}); });
  CHECK(std::abs(at(operators::divergence({"x", "y", "z"})(operators::curl({"x", "y", "z"})(G)), {0.3, 0.9, -1.2})) < 1e-12);  // div curl = 0
  const auto c = evaluate(operators::curl({"x", "y", "z"})(G), {1.0, 2.0, 3.0});
  CHECK(std::abs(c[0]) < 1e-12);  // d(xy)/dy - d(x^2 z)/dz = x - x^2 = 0 at x = 1
  CHECK(std::abs(c[1] - 10.0) < 1e-12);  // d(y z^2)/dz - d(xy)/dx = 2yz - y
  CHECK(std::abs(c[2] + 3.0) < 1e-12);   // d(x^2 z)/dx - d(y z^2)/dy = 2xz - z^2
  CHECK(std::abs(at(operators::laplacian({"x", "y", "z"})(xyz([](math::Function x, math::Function y, math::Function z) { return x * x + y * y * y + z * z * z * z; })), {1.0, 2.0, 3.0}) - (2 + 12 + 108)) < 1e-12);
  const auto planar = math::function({"x", "y"}, [](math::Function x, math::Function y) { return math::stack({-y, x}); });
  CHECK(std::abs(at(operators::curl({"x", "y"})(planar), {0.4, 0.2}) - 2.0) < 1e-13);  // rigid rotation
}

TEST_CASE("directional and material derivatives", "[calculus][extension][vector]") {
  const auto f = math::function({"x", "y", "t"}, [](math::Function x, math::Function y, math::Function t) { return x * y * t; });
  const auto v = math::function({"x", "y", "t"}, [](math::Function x, math::Function y, math::Function) { return math::stack({y, x}); });
  const double x = 0.7, y = 1.3, t = 2.0;
  CHECK(std::abs(at(operators::directional(v, {"x", "y"})(f), {x, y, t}) - (y * y * t + x * x * t)) < 1e-13);
  CHECK(std::abs(at(operators::material(v, "t", {"x", "y"})(f), {x, y, t}) - (x * y + y * y * t + x * x * t)) < 1e-13);
}

TEST_CASE("Euler-Lagrange equations", "[calculus][extension][variational]") {
  const auto EL = variational::euler_lagrange("x", {"u", "u1", "u2", "u3", "u4"});
  // harmonic oscillator L = u'^2 / 2 - w^2 u^2 / 2:  -w^2 u - u''
  const double w = 1.7;
  const auto osc = math::function({"x", "u", "u1", "u2", "u3", "u4"},
                                  [w](math::Function, math::Function, math::Function u1, math::Function, math::Function, math::Function) { return u1 * u1 * 0.5; })
                       .arg("x") * 0.0;  // placeholder to keep the signature uniform below
  (void)osc;
  const auto L = math::function({"x", "u", "u1", "u2", "u3", "u4"},
                                [w](math::Function, math::Function u, math::Function u1, math::Function, math::Function, math::Function) { return u1 * u1 * 0.5 - u * u * (w * w * 0.5); });
  const auto eq = EL(L);
  CHECK(std::abs(at(eq, {0.0, 0.8, 0.3, -0.5, 0.0, 0.0}) - (-w * w * 0.8 + 0.5)) < 1e-13);
  // Newton's law: u = cos(w x) makes the stationarity condition vanish
  const double xx = 0.9;
  CHECK(std::abs(at(eq, {xx, std::cos(w * xx), -w * std::sin(w * xx), -w * w * std::cos(w * xx), 0.0, 0.0})) < 1e-13);
  // mechanical L = u'^2/2 - u^4/4:  -u^3 - u''
  const auto mech = math::function({"x", "u", "u1", "u2", "u3", "u4"},
                                   [](math::Function, math::Function u, math::Function u1, math::Function, math::Function, math::Function) { return u1 * u1 * 0.5 - u * u * u * u * 0.25; });
  CHECK(std::abs(at(EL(mech), {0.0, 1.2, 0.1, 0.4, 0.0, 0.0}) - (-1.2 * 1.2 * 1.2 - 0.4)) < 1e-13);
  // minimal surface L = sqrt(1 + u'^2):  -u'' / (1 + u'^2)^(3/2)
  const auto area = math::function({"x", "u", "u1", "u2", "u3", "u4"}, [](math::Function, math::Function, math::Function u1, math::Function, math::Function, math::Function) { return (u1 * u1 + 1.0).sqrt(); });
  CHECK(std::abs(at(EL(area), {0.0, 0.0, 0.8, 0.6, 0.0, 0.0}) + 0.6 / std::pow(1.0 + 0.64, 1.5)) < 1e-13);
  // a second-order Lagrangian L = u''^2 / 2:  u''''
  const auto beam = math::function({"x", "u", "u1", "u2", "u3", "u4"}, [](math::Function, math::Function, math::Function, math::Function u2, math::Function, math::Function) { return u2 * u2 * 0.5; });
  CHECK(std::abs(at(EL(beam), {0.0, 0.0, 0.0, 0.0, 0.0, 3.5}) - 3.5) < 1e-13);
  // an x-dependent Lagrangian: L = x u'^2 / 2 gives -(x u')' = -u' - x u''
  const auto weighted = math::function({"x", "u", "u1", "u2", "u3", "u4"}, [](math::Function x, math::Function, math::Function u1, math::Function, math::Function, math::Function) { return x * u1 * u1 * 0.5; });
  CHECK(std::abs(at(EL(weighted), {2.0, 0.0, 0.7, 0.3, 0.0, 0.0}) + (0.7 + 2.0 * 0.3)) < 1e-13);
  // too few jets is reported, not silently truncated
  CHECK_THROWS(variational::euler_lagrange("x", {"u", "u1"})(math::function({"x", "u", "u1"}, [](math::Function, math::Function, math::Function u1) { return u1 * u1; })));
}

TEST_CASE("formal adjoints", "[calculus][extension][adjoint]") {
  const auto f = math::function({"x"}, [](math::Function x) { return (-(x * x) * 0.5).exp(); });
  const auto g = math::function({"x"}, [](math::Function x) { return (x + 0.4) * (-(x * x) * 0.3).exp(); });
  const auto m = math::function({"x"}, [](math::Function x) { return x * x + 1.0; });
  const auto inner = [](const math::Function &a, const math::Function &b) {
    return at0(operators::integral("x", -kInf, kInf)(operators::multiply(b)(a)));
  };
  const std::vector<operators::Operator> cases{
      operators::differential("x"), operators::differential("x", 2), operators::multiply(m),
      operators::compose(operators::multiply(m), operators::differential("x")),
      operators::linear_combination({{2.0, operators::differential("x")}, {3.0, operators::scale(1.5)}, {-0.5, operators::shift("x", 0.3)}}),
  };
  for (const operators::Operator &A : cases) {
    const operators::Operator Astar = operators::adjoint(A);
    CHECK(std::abs(inner(A(f), g) - inner(f, Astar(g))) < 1e-6);
  }
  CHECK(std::abs(inner(operators::differential("x")(f), g) + inner(f, operators::differential("x")(g))) < 1e-6);
  CHECK_THROWS(operators::adjoint(operators::integral("x", 0.0)));
  CHECK(operators::adjoint(operators::differential("x")).capabilities().adjoint_known);
}

TEST_CASE("fractional integrals and derivatives", "[calculus][extension][fractional]") {
  const auto x = math::function({"x"}, [](math::Function v) { return v; });
  const auto constant = math::function({"x"}, [](math::Function v) { return v * 0.0 + 1.0; });
  const double a = 0.5, at2 = 2.0;
  const auto RL = fractional::differential("x", a, fractional::Kind::RiemannLiouville);
  const auto Ca = fractional::differential("x", a, fractional::Kind::Caputo);
  CHECK(std::abs(at(RL(x), {at2}) - 2.0 * std::sqrt(at2 / kPi)) < 1e-13);           // D^{1/2} x = 2 sqrt(x / pi)
  CHECK(std::abs(at(Ca(constant), {at2})) < 1e-15);                                  // Caputo of a constant
  CHECK(std::abs(at(RL(constant), {at2}) - std::pow(at2, -a) / std::tgamma(1.0 - a)) < 1e-13);  // RL of a constant
  CHECK(RL(x).provenance() != math::Provenance::Quadrature);                         // polynomial: closed form
  const auto I = fractional::integral("x", a);
  CHECK(std::abs(at(I(x * x), {at2}) - std::tgamma(3.0) / std::tgamma(3.5) * std::pow(at2, 2.5)) < 1e-13);
  // integer order is the ordinary derivative
  CHECK(std::abs(at(fractional::differential("x", 2.0)(x * x * x), {at2}) - 6.0 * at2) < 1e-12);
  // lazy path: exp(x) has the Mittag-Leffler series  sum x^(k+a) / Gamma(k+a+1)
  const auto ex = math::function({"x"}, [](math::Function v) { return v.exp(); });
  const double xv = 1.3;
  double series = 0.0, dseries = 0.0;
  for (int k = 0; k < 60; ++k) {
    series += std::pow(xv, k + a) / std::tgamma(k + a + 1.0);
    dseries += std::pow(xv, k + a - 1.0) / std::tgamma(k + a);
  }
  const auto Iex = I(ex);
  CHECK(Iex.provenance() == math::Provenance::Quadrature);
  CHECK(std::abs(at(Iex, {xv}) - series) < 1e-5);
  CHECK(std::abs(at(RL(ex), {xv}) - dseries) < 1e-4);
  CHECK(std::abs(at(Ca(ex), {xv}) - series) < 1e-5);   // m = 1, f' = f
  // semigroup  I^0.3 I^0.4 x = I^0.7 x
  const auto I3 = fractional::integral("x", 0.3), I4 = fractional::integral("x", 0.4);
  CHECK(std::abs(at(I3(I4(x)), {at2}) - std::tgamma(2.0) / std::tgamma(2.7) * std::pow(at2, 1.7)) < 1e-5);
  // a shifted lower terminal: I^{1/2} 1 = (x - a)^{1/2} / Gamma(3/2)
  CHECK(std::abs(at(fractional::integral("x", a, 1.0)(constant), {3.0}) - std::sqrt(2.0) / std::tgamma(1.5)) < 1e-6);
  CHECK_THROWS(fractional::integral("x", -0.5));
}

TEST_CASE("convolution", "[calculus][extension][convolution]") {
  const auto gauss = math::function({"x"}, [](math::Function v) { return (-(v * v) * 0.5).exp(); });
  const auto conv = operators::convolution("x", gauss)(gauss);  // sqrt(pi) exp(-x^2 / 4)
  for (const double v : {0.0, 1.0, 2.5}) CHECK(std::abs(at(conv, {v}) - std::sqrt(kPi) * std::exp(-v * v / 4.0)) < 1e-6);
}

TEST_CASE("every serializable extension factory round-trips", "[calculus][extension][io]") {
  const auto F = xyz([](math::Function x, math::Function y, math::Function z) { return math::stack({x * x * y, y * z, z * x}); });
  const auto s = xyz([](math::Function x, math::Function y, math::Function z) { return x * x * y + z; });
  const auto jets = math::function({"x", "u", "u1", "u2", "u3", "u4"}, [](math::Function, math::Function u, math::Function u1, math::Function, math::Function, math::Function) { return u1 * u1 * 0.5 - u * u; });
  const auto poly = math::function({"x"}, [](math::Function v) { return v * v; });
  const double points[] = {0.5, 2.0}, weights[] = {0.25, 0.75}, nulls[] = {0.5};
  const auto roundtrip = [](const operators::Operator &op) {
    std::stringstream stream;
    math::io::save(op, stream);
    return math::io::load_operator(stream);
  };
  const auto same = [](const math::Function &a, const math::Function &b, std::initializer_list<double> p) {
    const auto va = evaluate(a, p), vb = evaluate(b, p);
    REQUIRE(va.size() == vb.size());
    for (std::size_t i = 0; i < va.size(); ++i) CHECK(va[i] == vb[i]);
  };
  const operators::Operator div = operators::divergence({"x", "y", "z"}), curl = operators::curl({"x", "y", "z"}),
                            lap = operators::laplacian({"x", "y", "z"});
  same(div(F), roundtrip(div)(F), {0.4, 0.5, 0.6});
  same(curl(F), roundtrip(curl)(F), {0.4, 0.5, 0.6});
  same(lap(s), roundtrip(lap)(s), {0.4, 0.5, 0.6});
  const auto EL = variational::euler_lagrange("x", {"u", "u1", "u2", "u3", "u4"});
  same(EL(jets), roundtrip(EL)(jets), {0.1, 0.2, 0.3, 0.4, 0.5, 0.6});
  const auto I = fractional::integral("x", 0.5), D = fractional::differential("x", 0.5, fractional::Kind::RiemannLiouville);
  same(I(poly), roundtrip(I)(poly), {1.5});
  same(D(poly), roundtrip(D)(poly), {1.5});
  const auto lebesgue = measure::lebesgue("x", 0.0, 1.0, nulls), atomic = measure::atomic("x", points, weights);
  CHECK(at0(lebesgue(poly)) == at0(roundtrip(lebesgue)(poly)));
  CHECK(at0(atomic(poly)) == at0(roundtrip(atomic)(poly)));
  const auto path = stochastic::malliavin_derivative({"x", "y", "z"});
  const auto delta = stochastic::skorokhod_integral({"x", "y", "z"}, 0.25);
  same(path(s), roundtrip(path)(s), {0.4, 0.5, 0.6});
  same(delta(F), roundtrip(delta)(F), {0.4, 0.5, 0.6});
}

#include <chrono>
#include <thread>

TEST_CASE("new operator results evaluate concurrently without shared mutation", "[calculus][extension][threads]") {
  const auto gauss = math::function({"x"}, [](math::Function v) { return (-(v * v) * 0.5).exp(); });
  const auto rational = math::function({"x"}, [](math::Function v) { return 1.0 / (v + 1.0); });
  const math::Function transform = operators::laplace::transform("x", "s")(rational);  // lazy infinite integral
  const math::Function fractional_lazy = fractional::integral("x", 0.5)(gauss);          // lazy, composed
  const math::Function closed = operators::fourier::transform("x", "w")(gauss);          // table
  const math::Function convolved = operators::convolution("x", gauss)(gauss);
  const double expected[] = {at(transform, {1.0}), at(fractional_lazy, {1.2}), at(closed, {0.7}), at(convolved, {0.5})};
  std::vector<std::thread> pool;
  std::vector<int> failures(8, 0);
  for (int t = 0; t < 8; ++t) {
    pool.emplace_back([&, t] {
      for (int i = 0; i < 20; ++i) {
        const double got[] = {at(transform, {1.0}), at(fractional_lazy, {1.2}), at(closed, {0.7}), at(convolved, {0.5})};
        for (int k = 0; k < 4; ++k) if (got[k] != expected[k]) ++failures[static_cast<std::size_t>(t)];
      }
    });
  }
  for (std::thread &th : pool) th.join();
  for (const int f : failures) CHECK(f == 0);
}

TEST_CASE("extension benchmarks", "[.perf]") {
  // geometric contraction: the covariant derivative of a vector field on the unit sphere, over a batch of points
  const auto metric = math::function({"theta", "phi"}, [](math::Function th, math::Function) {
    const math::Function s = th.sin();
    return math::stack({math::stack({th * 0.0 + 1.0, th * 0.0}), math::stack({th * 0.0, s * s})});
  });
  const auto gamma = operators::geometry::levi_civita(metric);
  const auto field = math::function({"theta", "phi"}, [](math::Function th, math::Function ph) { return math::stack({th.cos(), ph.sin()}); });
  const auto nabla = operators::geometry::covariant("theta", gamma, {1, 0})(field);
  const std::int64_t n = 200000;
  std::vector<double> a(static_cast<std::size_t>(n)), b(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) { a[static_cast<std::size_t>(i)] = 0.1 + 3.0 * i / n; b[static_cast<std::size_t>(i)] = 6.0 * i / n; }
  const pluss::Series th = pluss::make_series(a, {n}), ph = pluss::make_series(b, {n});
  const auto t0 = std::chrono::steady_clock::now();
  const pluss::Series out = nabla(pluss::RangeView<pluss::Series>(std::vector<pluss::Series>{th, ph}));
  const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  const auto t1 = std::chrono::steady_clock::now();
  const pluss::Series connection = gamma(pluss::RangeView<pluss::Series>(std::vector<pluss::Series>{th, ph}));
  const double gamma_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count();
  const auto t2 = std::chrono::steady_clock::now();
  const pluss::Series inv = math::function({"theta", "phi"}, [](math::Function a2, math::Function) {
    return math::stack({math::stack({a2 * 0.0 + 1.0, a2 * 0.0}), math::stack({a2 * 0.0, a2.sin() * a2.sin()})}).inverse();
  })(pluss::RangeView<pluss::Series>(std::vector<pluss::Series>{th, ph}));
  const double inverse_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t2).count();
  std::printf("  of which the connection alone: %.2f ms (%lld values); a batched 2x2 inverse: %.2f ms\n", gamma_ms, static_cast<long long>(connection.numel()), inverse_ms);
  (void)inv;
  std::printf("covariant derivative over %lld points: %.2f ms (%lld values)\n", static_cast<long long>(n), ms, static_cast<long long>(out.numel()));
}

TEST_CASE("closed-form small determinants and inverses agree with the identity", "[calculus][extension][tensor]") {
  for (const int n : {1, 2, 3}) {
    const auto A = math::function({"a", "b"}, [n](math::Function a, math::Function b) {
      std::vector<math::Function> rows;
      for (int i = 0; i < n; ++i) {
        std::vector<math::Function> row;
        for (int j = 0; j < n; ++j) row.push_back(a * (0.3 * (i + 1) - 0.2 * j) + b * (0.1 * i * j) + (i == j ? 2.0 : 0.15 * (i - j)));
        rows.push_back(math::stack(pluss::RangeView<math::Function>(row)));
      }
      return math::stack(pluss::RangeView<math::Function>(rows));
    });
    const auto product = A.matmul(A.inverse());
    const auto values = evaluate(product, {0.7, -0.4});
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) CHECK(std::abs(values[static_cast<std::size_t>(i * n + j)] - (i == j ? 1.0 : 0.0)) < 1e-13);
    // det of a triangular-plus-diagonal matrix by expansion, cross-checked against the product of the pivots for n = 1
    if (n == 1) CHECK(std::abs(at(A.det(), {0.7, -0.4}) - 2.0 - 0.7 * 0.3) < 1e-13);
  }
}
