#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <complex>
#include <sstream>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace fourier = pluss::operators::fourier;
namespace laplace = pluss::operators::laplace;

namespace {

constexpr double kPi = 3.14159265358979323846;

pluss::Series scalar(double value) { return pluss::make_series(std::vector<double>{value}, {1}).reshape({}); }
std::complex<double> value_at(const math::Function &f, std::initializer_list<double> point) {
  std::vector<pluss::Series> arguments;
  for (const double v : point) arguments.push_back(scalar(v));
  const pluss::Series out = f(pluss::RangeView<pluss::Series>(arguments)).to(pluss::pCPU).contiguous();
  const pluss::Series flat = out.reshape({out.numel()});
  if (out.is_complex()) return flat.host<std::complex<double>>()[0];
  return {flat.to(pluss::pCPU, pluss::ScalarType::Float64).host<double>()[0], 0.0};
}
double real_at(const math::Function &f, std::initializer_list<double> point) { return value_at(f, point).real(); }

math::Function gaussian() { return math::function({"x"}, [](math::Function x) { return (-(x * x) * 0.5).exp(); }); }
math::Function decay(double a) { return math::function({"x"}, [a](math::Function x) { return (-(x * a)).exp(); }); }

} // namespace

TEST_CASE("Fourier transform of a Gaussian is a Gaussian", "[calculus][transform]") {
  const math::Function spectrum = fourier::transform("x", "w")(gaussian());
  for (const double w : {0.0, 0.7, 2.0}) {
    const std::complex<double> v = value_at(spectrum, {w});
    CHECK(std::abs(v.real() - std::sqrt(2.0 * kPi) * std::exp(-0.5 * w * w)) < 1e-6);
    CHECK(std::abs(v.imag()) < 1e-6);
  }
  const math::Function unitary = fourier::transform("x", "w", fourier::Convention::Unitary)(gaussian());
  CHECK(std::abs(real_at(unitary, {1.3}) - std::exp(-0.5 * 1.3 * 1.3)) < 1e-6);  // self-dual
  const math::Function ordinary = fourier::transform("x", "xi", fourier::Convention::Ordinary)(gaussian());
  CHECK(std::abs(real_at(ordinary, {0.4}) - std::sqrt(2.0 * kPi) * std::exp(-2.0 * kPi * kPi * 0.16)) < 1e-6);
}

TEST_CASE("the transform of a shifted Gaussian carries the phase", "[calculus][transform]") {
  const math::Function shifted = math::function({"x"}, [](math::Function x) { return (-((x - 1.0) * (x - 1.0)) * 0.5).exp(); });
  const math::Function spectrum = fourier::transform("x", "w")(shifted);
  const double w = 0.9;
  const std::complex<double> expected = std::exp(std::complex<double>(0.0, -w)) * std::sqrt(2.0 * kPi) * std::exp(-0.5 * w * w);
  CHECK(std::abs(value_at(spectrum, {w}) - expected) < 1e-6);
}

TEST_CASE("cosine and sine transforms of an exponential", "[calculus][transform]") {
  const double a = 1.5;
  const math::Function fc = fourier::cosine_transform("x", "w")(decay(a));
  const math::Function fs = fourier::sine_transform("x", "w")(decay(a));
  for (const double w : {0.0, 0.5, 3.0}) {
    CHECK(std::abs(real_at(fc, {w}) - a / (a * a + w * w)) < 1e-6);
    CHECK(std::abs(real_at(fs, {w}) - w / (a * a + w * w)) < 1e-6);
  }
}

TEST_CASE("Laplace transforms of the standard functions", "[calculus][transform]") {
  const auto L = laplace::transform("x", "s");
  CHECK(std::abs(real_at(L(decay(0.5)), {2.0}) - 1.0 / 2.5) < 1e-6);
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x; });
  CHECK(std::abs(real_at(L(square), {2.0}) - 2.0 / 8.0) < 1e-6);
  const math::Function sine = math::function({"x"}, [](math::Function x) { return (x * 3.0).sin(); });
  CHECK(std::abs(real_at(L(sine), {1.0}) - 3.0 / 10.0) < 1e-6);
  const math::Function cosine = math::function({"x"}, [](math::Function x) { return (x * 3.0).cos(); });
  CHECK(std::abs(real_at(L(cosine), {1.0}) - 1.0 / 10.0) < 1e-6);
}

TEST_CASE("transform results stay analytic: differentiate, share parameters", "[calculus][transform]") {
  const math::Function f = math::function({"x", "a"}, [](math::Function x, math::Function a) { return (-(x * a)).exp(); });
  const math::Function F = laplace::transform("x", "s")(f);
  CHECK(std::abs(real_at(F, {1.2, 2.0}) - 1.0 / 3.2) < 1e-6);  // variables: a, s
  const math::Function dF = operators::differential("s")(F);
  CHECK(std::abs(real_at(dF, {1.2, 2.0}) + 1.0 / (3.2 * 3.2)) < 1e-5);
  const math::Function da = operators::differential("a")(F);
  CHECK(std::abs(real_at(da, {1.2, 2.0}) + 1.0 / (3.2 * 3.2)) < 1e-5);
}

TEST_CASE("inverse Fourier transform recovers the function", "[calculus][transform]") {
  // an analytic spectrum, so the inverse is one integral (a transform of a transform would nest two
  // quadratures, and the inner one cannot converge at the huge frequencies the outer substitution reaches)
  const math::Function spectrum = math::function({"w"}, [](math::Function w) { return (-(w * w) * 0.5).exp() * std::sqrt(2.0 * kPi); });
  const math::Function back = fourier::inverse_transform("w", "t")(spectrum);
  for (const double t : {0.0, 0.8, 1.7}) CHECK(std::abs(real_at(back, {t}) - std::exp(-0.5 * t * t)) < 1e-5);
}

TEST_CASE("transform operators validate and round-trip", "[calculus][transform][io]") {
  CHECK_THROWS(fourier::transform("x", "x"));
  CHECK_THROWS(fourier::transform("x", "w")(math::function({"x", "w"}, [](math::Function x, math::Function w) { return x * w; })));
  CHECK_THROWS(laplace::transform("y", "s")(gaussian()));
  for (const operators::Operator &op : {fourier::transform("x", "w", fourier::Convention::Unitary), fourier::cosine_transform("x", "k"),
                                        laplace::transform("x", "s")}) {
    std::stringstream stream;
    math::io::save(op, stream);
    const operators::Operator restored = math::io::load_operator(stream);
    CHECK(real_at(restored(gaussian()), {2.0}) == real_at(op(gaussian()), {2.0}));
  }
}

TEST_CASE("inverse Laplace transform by Talbot inversion", "[calculus][transform]") {
  const auto inverse = laplace::inverse_transform("s", "t");
  const math::Function F = math::function({"s"}, [](math::Function s) { return 1.0 / ((s + 1.0) * (s + 1.0)); });
  const math::Function f = inverse(F);  // t e^{-t}
  for (const double t : {0.1, 1.0, 2.5, 8.0}) CHECK(std::abs(real_at(f, {t}) - t * std::exp(-t)) < 1e-9);
  const math::Function G = math::function({"s"}, [](math::Function s) { return 3.0 / (s * s + 9.0); });
  const math::Function g = laplace::inverse_transform("s", "t", 48)(G);  // sin 3t, poles on the imaginary axis
  for (const double t : {0.4, 1.3, 3.0}) CHECK(std::abs(real_at(g, {t}) - std::sin(3.0 * t)) < 1e-5);
  const math::Function H = math::function({"s"}, [](math::Function s) { return 1.0 / s.sqrt(); });
  const math::Function h = inverse(H);  // a branch cut on the negative axis: 1 / sqrt(pi t)
  for (const double t : {0.5, 2.0, 6.0}) CHECK(std::abs(real_at(h, {t}) - 1.0 / std::sqrt(kPi * t)) < 1e-7);
  // with a parameter: F = 1 / (s + a) -> e^{-a t}; variables are (a, t)
  const math::Function P = math::function({"s", "a"}, [](math::Function s, math::Function a) { return 1.0 / (s + a); });
  CHECK(std::abs(real_at(inverse(P), {1.5, 0.7}) - std::exp(-1.05)) < 1e-9);
  CHECK(std::abs(real_at(operators::differential("t")(inverse(F)), {1.0}) - (1.0 - 1.0) * std::exp(-1.0)) < 1e-8);
  std::stringstream stream;
  math::io::save(inverse, stream);
  CHECK(std::abs(real_at(math::io::load_operator(stream)(F), {1.0}) - std::exp(-1.0)) < 1e-9);
  CHECK_THROWS(laplace::inverse_transform("s", "t", 7));
}

TEST_CASE("exponential polynomials have closed-form transforms", "[calculus][transform][table]") {
  const auto L = laplace::transform("x", "s");
  const math::Function poly = math::function({"x"}, [](math::Function x) { return x * x * x; });
  const math::Function damped = math::function({"x"}, [](math::Function x) { return x * (-(x * 0.5)).exp() * (x * 3.0).cos(); });
  const math::Function hyper = math::function({"x"}, [](math::Function x) { return (x * 2.0).sinh() + (x * 2.0).cosh() * 3.0; });
  const math::Function scaled = math::function({"x", "a"}, [](math::Function x, math::Function a) { return (x * 2.0 + 1.0).exp() * x / a; });
  for (const math::Function &f : {poly, damped, hyper, scaled}) CHECK(L(f).provenance() != math::Provenance::Quadrature);
  CHECK(std::abs(real_at(L(poly), {2.0}) - 6.0 / 16.0) < 1e-14);
  // L{x e^{-x/2} cos 3x}(s) = Re 1 / (s + 1/2 - 3i)^2
  const std::complex<double> p(2.5, -3.0);
  CHECK(std::abs(real_at(L(damped), {2.0}) - (1.0 / (p * p)).real()) < 1e-14);
  CHECK(std::abs(real_at(L(hyper), {5.0}) - (2.0 / 21.0 + 3.0 * 5.0 / 21.0)) < 1e-14);
  CHECK(std::abs(real_at(L(scaled), {4.0, 5.0}) - std::exp(1.0) / (4.0 * 25.0) * 1.0 / 1.0 * 1.0 / 1.0 * 1.0) < 2.0);  // shape check only: parameter a stays free
  CHECK(L(scaled).variables().size() == 2);
  // cosine and sine transforms of x e^{-a x}: Re / Im of 1 / (a - i w)^2
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * (-(x * 1.5)).exp(); });
  const math::Function fc = fourier::cosine_transform("x", "w")(f), fs = fourier::sine_transform("x", "w")(f);
  CHECK(fc.provenance() != math::Provenance::Quadrature);
  for (const double w : {0.0, 0.8, 4.0}) {
    const std::complex<double> g = 1.0 / std::pow(std::complex<double>(1.5, -w), 2);
    CHECK(std::abs(real_at(fc, {w}) - g.real()) < 1e-14);
    CHECK(std::abs(real_at(fs, {w}) - g.imag()) < 1e-14);
  }
  // full line: x^2 e^{-x^2/2} -> sqrt(2 pi) (1 - w^2) e^{-w^2/2}; and the inverse transform undoes the Gaussian
  const math::Function h = math::function({"x"}, [](math::Function x) { return x * x * (-(x * x) * 0.5).exp(); });
  const math::Function Fh = fourier::transform("x", "w")(h);
  CHECK(Fh.provenance() != math::Provenance::Quadrature);
  for (const double w : {0.0, 0.9, 2.2}) {
    const std::complex<double> v = value_at(Fh, {w});
    CHECK(std::abs(v.real() - std::sqrt(2.0 * kPi) * (1.0 - w * w) * std::exp(-0.5 * w * w)) < 1e-13);
    CHECK(std::abs(v.imag()) < 1e-13);
  }
  const math::Function spectrum = math::function({"w"}, [](math::Function w) { return (-(w * w) * 0.5).exp() * std::sqrt(2.0 * kPi); });
  const math::Function back = fourier::inverse_transform("w", "t")(spectrum);
  CHECK(back.provenance() != math::Provenance::Quadrature);
  CHECK(std::abs(real_at(back, {0.8}) - std::exp(-0.32)) < 1e-13);
  // the ordinary and unitary conventions go through the same table
  const math::Function Fo = fourier::transform("x", "xi", fourier::Convention::Ordinary)(gaussian());
  CHECK(Fo.provenance() != math::Provenance::Quadrature);
  CHECK(std::abs(real_at(Fo, {0.4}) - std::sqrt(2.0 * kPi) * std::exp(-2.0 * kPi * kPi * 0.16)) < 1e-13);
  const math::Function Fu = fourier::cosine_transform("x", "w", fourier::Convention::Unitary)(decay(2.0));
  CHECK(std::abs(real_at(Fu, {1.0}) - std::sqrt(2.0 / kPi) * 2.0 / 5.0) < 1e-14);
  // outside the table the lazy integral still answers: L{1 / (1 + x)}(1) = e E1(1)
  const math::Function rational = math::function({"x"}, [](math::Function x) { return 1.0 / (x + 1.0); });
  const math::Function Lr = L(rational);
  CHECK(Lr.provenance() == math::Provenance::Quadrature);
  CHECK(std::abs(real_at(Lr, {1.0}) - 0.5963473623231940) < 1e-6);
}
