#include "../FunctionInternal.h"
#include "../OperatorInternal.h"

#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace geometry = pluss::operators::geometry;

namespace {

pluss::Series scalar(double value) {
  return pluss::make_series(std::vector<double>{value}, {1}).reshape({});
}
std::vector<double> host(const pluss::Series &value) {
  const pluss::Series cpu =
      value.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous();
  return cpu.reshape({cpu.numel()}).host<double>();
}
double at(const math::Function &f, std::initializer_list<double> point,
          std::size_t index = 0) {
  std::vector<pluss::Series> arguments;
  for (const double v : point) arguments.push_back(scalar(v));
  return host(f(pluss::RangeView<pluss::Series>(arguments)))[index];
}

// unit sphere, coordinates (theta, phi)
math::Function sphere_metric() {
  return math::function({"theta", "phi"}, [](math::Function theta, math::Function phi) {
    const math::Function zero = theta * 0.0 + phi * 0.0;
    const math::Function one = zero + 1.0;
    return math::stack({math::stack({one, zero}), math::stack({zero, theta.sin() * theta.sin()})});
  });
}
// flat plane in polar coordinates (r, th)
math::Function polar_metric() {
  return math::function({"r", "th"}, [](math::Function r, math::Function th) {
    const math::Function zero = r * 0.0 + th * 0.0;
    return math::stack({math::stack({zero + 1.0, zero}), math::stack({zero, r * r})});
  });
}
constexpr std::size_t G(int i, int j, int k) { return static_cast<std::size_t>((i * 2 + j) * 2 + k); }

} // namespace

TEST_CASE("compose substitutes Functions for variables", "[calculus][geometry][compose]") {
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * y + y; });
  const math::Function composed = math::function({"t"}, [&](math::Function t) { return f.compose({t, t * t}); });
  REQUIRE(composed.arity() == 1);
  CHECK(std::abs(at(composed, {1.7}) - (1.7 * 2.89 + 2.89)) < 1e-12);
  CHECK(std::abs(at(operators::differential("t")(composed), {1.7}) - (3.0 * 1.7 * 1.7 + 2.0 * 1.7)) < 1e-12);
  CHECK_THROWS_AS(f.compose({composed}), pluss::Error);
}

TEST_CASE("levi_civita reproduces the textbook Christoffel symbols", "[calculus][geometry]") {
  const math::Function gamma = geometry::levi_civita(sphere_metric());
  REQUIRE(gamma.rank() == 3);
  for (const double theta : {0.4, 0.9, 1.3}) {
    const double s = std::sin(theta), c = std::cos(theta);
    const std::vector<double> value = host(gamma({scalar(theta), scalar(0.2)}));
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j)
        for (int k = 0; k < 2; ++k) {
          double expected = 0.0;
          if (i == 0 && j == 1 && k == 1) expected = -s * c;
          if (i == 1 && ((j == 0 && k == 1) || (j == 1 && k == 0))) expected = c / s;
          CHECK(std::abs(value[G(i, j, k)] - expected) < 1e-10);
          CHECK(std::abs(value[G(i, j, k)] - value[G(i, k, j)]) < 1e-12); // torsion free
        }
  }
  const math::Function polar = geometry::levi_civita(polar_metric());
  const double r = 1.7;
  const std::vector<double> p = host(polar({scalar(r), scalar(0.3)}));
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k) {
        double expected = 0.0;
        if (i == 0 && j == 1 && k == 1) expected = -r;
        if (i == 1 && ((j == 0 && k == 1) || (j == 1 && k == 0))) expected = 1.0 / r;
        CHECK(std::abs(p[G(i, j, k)] - expected) < 1e-10);
      }
  CHECK_THROWS_AS(geometry::levi_civita(math::function({"x"}, [](math::Function x) { return x; })), pluss::Error);
}

TEST_CASE("absolute differential along latitude circles and parallel transport", "[calculus][geometry]") {
  const math::Function gamma = geometry::levi_civita(sphere_metric());
  const double theta0 = 1.0, omega = std::cos(theta0), s0 = std::sin(theta0);
  const math::Function curve = math::function({"t"}, [&](math::Function t) { return math::stack({t * 0.0 + theta0, t}); });
  const operators::Operator D = geometry::absolute_differential(curve, gamma, "t", {1, 0});

  // the velocity of a latitude circle is not parallel: D(xdot)/dt = (-sin cos, 0)
  const math::Function velocity = operators::differential("t")(curve);
  const math::Function acceleration = D(velocity);
  CHECK(std::abs(at(acceleration, {0.7}, 0) - (-s0 * omega)) < 1e-10);
  CHECK(std::abs(at(acceleration, {0.7}, 1)) < 1e-10);

  // closed-form parallel transport around the latitude: V = (cos(omega t), -sin(omega t) / sin(theta0))
  const math::Function transported = math::function({"t"}, [&](math::Function t) {
    return math::stack({(t * omega).cos(), (t * omega).sin() * (-1.0 / s0)});
  });
  for (const double t : {0.0, 0.8, 2.5, 5.9}) {
    CHECK(std::abs(at(D(transported), {t}, 0)) < 1e-10);
    CHECK(std::abs(at(D(transported), {t}, 1)) < 1e-10);
  }
  // after a full circuit the frame has rotated by 2 pi cos(theta0): holonomy angle 2 pi (1 - cos theta0)
  const double two_pi = 2.0 * std::acos(-1.0);
  const double v_theta = at(transported, {two_pi}, 0), v_phi = at(transported, {two_pi}, 1) * s0; // orthonormal components
  CHECK(std::abs(std::atan2(-v_phi, v_theta) - std::remainder(two_pi * omega, two_pi)) < 1e-10);

  // a field that is not transported has D V != 0: V = (1, 0) gives (0, cot theta0)
  const math::Function constant = math::function({"t"}, [](math::Function t) { return math::stack({t * 0.0 + 1.0, t * 0.0}); });
  CHECK(std::abs(at(D(constant), {0.3}, 0)) < 1e-10);
  CHECK(std::abs(at(D(constant), {0.3}, 1) - omega / s0) < 1e-10);

  // the equator and meridians are geodesics: their velocity is parallel
  const math::Function equator = math::function({"t"}, [](math::Function t) { return math::stack({t * 0.0 + std::acos(-1.0) / 2.0, t}); });
  const operators::Operator De = geometry::absolute_differential(equator, gamma, "t");
  const math::Function eq_velocity = operators::differential("t")(equator);
  CHECK(std::abs(at(De(eq_velocity), {0.5}, 0)) < 1e-10);
  CHECK(std::abs(at(De(eq_velocity), {0.5}, 1)) < 1e-10);
}

TEST_CASE("absolute differential is compatible with the metric", "[calculus][geometry]") {
  const math::Function metric = sphere_metric();
  const math::Function gamma = geometry::levi_civita(metric);
  const math::Function curve = math::function({"t"}, [](math::Function t) {
    return math::stack({t.sin() * 0.3 + 1.0, t * 0.5 + t * t * 0.1});
  });
  const math::Function v = math::function({"t"}, [](math::Function t) { return math::stack({t.sin(), (t * 2.0).cos()}); });
  const math::Function w = math::function({"t"}, [](math::Function t) { return math::stack({t * t, (t * 3.0).sin() + 1.0}); });
  const operators::Operator D = geometry::absolute_differential(curve, gamma, "t", {1, 0});

  const auto g_on_curve = [&](double t) { // g_ij(x(t)) as numbers
    return host(metric({scalar(at(curve, {t}, 0)), scalar(at(curve, {t}, 1))}));
  };
  const auto pairing = [&](double t, const std::vector<double> &a, const std::vector<double> &b) {
    const std::vector<double> g = g_on_curve(t);
    double sum = 0.0;
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j) sum += g[static_cast<std::size_t>(i * 2 + j)] * a[static_cast<std::size_t>(i)] * b[static_cast<std::size_t>(j)];
    return sum;
  };
  const auto value = [&](const math::Function &f, double t) { return host(f({scalar(t)})); };
  const double t0 = 0.9, h = 1e-5;
  const double lhs = (pairing(t0 + h, value(v, t0 + h), value(w, t0 + h)) - pairing(t0 - h, value(v, t0 - h), value(w, t0 - h))) / (2.0 * h);
  const double rhs = pairing(t0, value(D(v), t0), value(w, t0)) + pairing(t0, value(v, t0), value(D(w), t0));
  CHECK(std::abs(lhs - rhs) < 1e-6);
}

TEST_CASE("covariant derivative of tensor fields", "[calculus][geometry]") {
  const math::Function metric = sphere_metric();
  const math::Function gamma = geometry::levi_civita(metric);
  const std::vector<double> point{0.8, 0.4};
  // metric compatibility: nabla g = 0 and nabla g^-1 = 0, in both coordinate directions
  for (const char *direction : {"theta", "phi"}) {
    const math::Function dg = geometry::covariant(direction, gamma, {0, 2})(metric);
    const math::Function dginv = geometry::covariant(direction, gamma, {2, 0})(metric.inverse());
    for (std::size_t index = 0; index < 4; ++index) {
      CHECK(std::abs(at(dg, {point[0], point[1]}, index)) < 1e-9);
      CHECK(std::abs(at(dginv, {point[0], point[1]}, index)) < 1e-9);
    }
  }
  // the covariant Hessian of a scalar is symmetric (torsion free)
  const math::Function f = math::function({"theta", "phi"}, [](math::Function theta, math::Function phi) {
    return (theta * theta).sin() * phi.cos() + theta * phi;
  });
  const math::Function gradient = operators::gradient({"theta", "phi"})(f); // covector V_i = d_i f
  const math::Function d_theta = geometry::covariant("theta", gamma, {0, 1})(gradient);
  const math::Function d_phi = geometry::covariant("phi", gamma, {0, 1})(gradient);
  CHECK(std::abs(at(d_theta, {point[0], point[1]}, 1) - at(d_phi, {point[0], point[1]}, 0)) < 1e-9);
  CHECK(std::abs(at(d_theta, {point[0], point[1]}, 1)) > 1e-3); // not trivially zero
  // flat space: the Cartesian-constant field e_x written in polar components is parallel
  const math::Function polar_gamma = geometry::levi_civita(polar_metric());
  const math::Function ex = math::function({"r", "th"}, [](math::Function r, math::Function th) {
    return math::stack({th.cos() + r * 0.0, th.sin() * (-1.0) / r});
  });
  for (const char *direction : {"r", "th"}) {
    const math::Function nabla = geometry::covariant(direction, polar_gamma, {1, 0})(ex);
    CHECK(std::abs(at(nabla, {1.3, 0.6}, 0)) < 1e-10);
    CHECK(std::abs(at(nabla, {1.3, 0.6}, 1)) < 1e-10);
  }
}

TEST_CASE("geometry operators validate their inputs", "[calculus][geometry][errors]") {
  const math::Function gamma = geometry::levi_civita(sphere_metric());
  const math::Function curve = math::function({"t"}, [](math::Function t) { return math::stack({t, t}); });
  const math::Function scalar_field = math::function({"t"}, [](math::Function t) { return t * t; });
  CHECK_THROWS_AS(geometry::absolute_differential(curve, gamma, "t", {1, 0})(scalar_field), pluss::Error); // rank != signature
  CHECK_THROWS_AS(geometry::absolute_differential(scalar_field, gamma, "t"), pluss::Error);
  CHECK_THROWS_AS(geometry::covariant("nope", gamma, {0, 0}), pluss::Error);
  CHECK_THROWS_AS(geometry::covariant("theta", curve, {0, 0}), pluss::Error); // not a rank-3 connection
  CHECK_FALSE(geometry::absolute_differential(curve, gamma, "t").capabilities().serializable);
  CHECK(geometry::absolute_differential(curve, gamma, "t").capabilities().linear);
}

TEST_CASE("magnitude_differential keeps the old behaviour and the legacy key loads", "[calculus][geometry][io]") {
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x * x - x; });
  const operators::Operator magnitude = operators::magnitude_differential("x");
  CHECK_FALSE(magnitude.capabilities().linear);
  CHECK(std::abs(at(magnitude(square), {-0.5}) - std::abs(3.0 * 0.25 - 1.0)) < 1e-12);
  CHECK(magnitude.key() == "magnitude_differential");
  const std::string payload(operators::detail::OperatorAccess::payload(magnitude));
  const operators::Operator legacy = operators::detail::OperatorAccess::restore("absolute_differential", 1, payload);
  CHECK(std::abs(at(legacy(square), {-0.5}) - at(magnitude(square), {-0.5})) < 1e-15);
  CHECK(legacy.key() == "magnitude_differential");
  CHECK_THROWS_AS(operators::detail::OperatorAccess::restore("absolute_differential", 2, payload), pluss::Error);
}
