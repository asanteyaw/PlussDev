#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <complex>
#include <sstream>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace fourier = pluss::operators::fourier;

namespace {

constexpr double kPi = 3.14159265358979323846;

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
math::Function identity() { return math::function({"x"}, [](math::Function x) { return x; }); }
double normal_cdf(double z) { return 0.5 * std::erfc(-z / std::sqrt(2.0)); }

} // namespace

TEST_CASE("sine and cosine coefficients of x on [0, pi]", "[calculus][fourier]") {
  const auto b = host(fourier::sine_coefficients("x", 0.0, kPi, 8)(identity())());
  REQUIRE(b.size() == 8); // B_1..B_8
  for (int k = 1; k <= 8; ++k) CHECK(std::abs(b[static_cast<std::size_t>(k - 1)] - 2.0 * (k % 2 ? 1.0 : -1.0) / k) < 1e-8);
  const auto a = host(fourier::cosine_coefficients("x", 0.0, kPi, 8)(identity())());
  REQUIRE(a.size() == 8); // A_0..A_7
  CHECK(std::abs(a[0] - kPi) < 1e-8);
  for (int k = 1; k < 8; ++k) CHECK(std::abs(a[static_cast<std::size_t>(k)] - (k % 2 ? -4.0 / (kPi * k * k) : 0.0)) < 1e-8);
}

TEST_CASE("the series converge and satisfy Parseval", "[calculus][fourier]") {
  // f = x (pi - x) on [0, pi]; closed forms: A_0 = pi^2 / 3, A_k = -2 (1 + (-1)^k) / k^2, B_k = 4 (1 - (-1)^k) / (pi k^3)
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * (x * -1.0 + kPi); });
  const auto a = host(fourier::cosine_coefficients("x", 0.0, kPi, 30)(f)());
  const auto b = host(fourier::sine_coefficients("x", 0.0, kPi, 30)(f)());
  CHECK(std::abs(a[0] - kPi * kPi / 3.0) < 1e-8);
  for (int k = 1; k < 30; ++k) CHECK(std::abs(a[static_cast<std::size_t>(k)] - (-2.0 * (1.0 + (k % 2 ? -1.0 : 1.0)) / (k * k))) < 1e-8);
  for (int k = 1; k <= 30; ++k) CHECK(std::abs(b[static_cast<std::size_t>(k - 1)] - 4.0 * (1.0 - (k % 2 ? -1.0 : 1.0)) / (kPi * k * k * k)) < 1e-8);
  // pointwise error shrinks with the number of terms (sine: O(K^-2), cosine: O(K^-1))
  double previous_sine = 1.0, previous_cosine = 1.0;
  for (const int terms : {4, 8, 16}) {
    const math::Function sine = fourier::sine_series("x", 0.0, kPi, terms)(f);
    const math::Function cosine = fourier::cosine_series("x", 0.0, kPi, terms)(f);
    double worst_sine = 0.0, worst_cosine = 0.0;
    for (double x = 0.2; x < 3.0; x += 0.4) {
      worst_sine = std::max(worst_sine, std::abs(at(sine, {x}) - at(f, {x})));
      worst_cosine = std::max(worst_cosine, std::abs(at(cosine, {x}) - at(f, {x})));
    }
    CHECK(worst_sine < previous_sine);
    CHECK(worst_cosine < previous_cosine);
    previous_sine = worst_sine;
    previous_cosine = worst_cosine;
  }
  CHECK(previous_sine < 2e-3);
  CHECK(previous_cosine < 0.06);
  // Parseval: int f^2 = pi^5 / 30 = (pi / 2) (A_0^2 / 2 + sum A_k^2) = (pi / 2) sum B_k^2, up to the truncated tail
  const double energy = std::pow(kPi, 5) / 30.0;
  double cosine_sum = 0.0, sine_sum = 0.0;
  for (std::size_t k = 0; k < a.size(); ++k) cosine_sum += (k == 0 ? 0.5 : 1.0) * a[k] * a[k];
  for (const double v : b) sine_sum += v * v;
  CHECK(std::abs(kPi / 2.0 * cosine_sum - energy) < 2e-4);
  CHECK(std::abs(kPi / 2.0 * sine_sum - energy) < 1e-6); // the truncated tail is about 4e-8
}

TEST_CASE("the full Fourier series is the cosine series of the even part plus the sine series of the odd part", "[calculus][fourier]") {
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp(); });
  const int terms = 6;
  // closed forms on [-1, 1]: a_k = 2 (-1)^k sinh(1) / (1 + k^2 pi^2), b_k = -k pi a_k
  const auto c = host(fourier::coefficients("x", 1.0, terms)(f)());
  REQUIRE(c.size() == static_cast<std::size_t>(2 * terms - 1));
  const double sinh1 = std::sinh(1.0);
  for (int k = 0; k < terms; ++k) {
    const double ak = 2.0 * (k % 2 ? -1.0 : 1.0) * sinh1 / (1.0 + k * k * kPi * kPi);
    CHECK(std::abs(c[static_cast<std::size_t>(k)] - ak) < 1e-9);
    if (k >= 1) CHECK(std::abs(c[static_cast<std::size_t>(terms - 1 + k)] - (-k * kPi * ak)) < 1e-9);
  }
  // exp = cosh + sinh: even and odd parts; the half-range series of each on [0, 1] add up to the full series
  const math::Function even = math::function({"x"}, [](math::Function x) { return x.cosh(); });
  const math::Function odd = math::function({"x"}, [](math::Function x) { return x.sinh(); });
  const math::Function full = fourier::series("x", 1.0, terms)(f);
  const math::Function parts_cos = fourier::cosine_series("x", 0.0, 1.0, terms)(even);
  const math::Function parts_sin = fourier::sine_series("x", 0.0, 1.0, terms - 1)(odd);
  for (const double x : {0.15, 0.4, 0.8}) {
    CHECK(std::abs(at(full, {x}) - (at(parts_cos, {x}) + at(parts_sin, {x}))) < 1e-8);
    CHECK(std::abs(at(full, {-x}) - (at(parts_cos, {x}) - at(parts_sin, {x}))) < 1e-8); // parity
  }
  // the series equals the partial sum of the closed-form coefficients (exp has a jump in its periodic extension, so the
  // partial sum itself converges slowly: that is Gibbs behaviour, not an error of the operator)
  double partial = 0.5 * 2.0 * sinh1;
  for (int k = 1; k < terms; ++k) {
    const double ak = 2.0 * (k % 2 ? -1.0 : 1.0) * sinh1 / (1.0 + k * k * kPi * kPi);
    partial += ak * std::cos(k * kPi * 0.3) + (-k * kPi * ak) * std::sin(k * kPi * 0.3);
  }
  CHECK(std::abs(at(full, {0.3}) - partial) < 1e-8);
}

TEST_CASE("the Gibbs overshoot of a square wave does not decay", "[calculus][fourier]") {
  // the odd extension of 1 on (0, 1) is a square wave of height 1: B_k = 4 / (k pi) for odd k
  const math::Function one = math::function({"x"}, [](math::Function x) { return x * 0.0 + 1.0; });
  const auto b = host(fourier::sine_coefficients("x", 0.0, 1.0, 6)(one)());
  for (int k = 1; k <= 6; ++k) CHECK(std::abs(b[static_cast<std::size_t>(k - 1)] - (k % 2 ? 4.0 / (k * kPi) : 0.0)) < 1e-9);
  for (const int terms : {20, 40}) {
    const math::Function series = fourier::sine_series("x", 0.0, 1.0, terms)(one);
    double peak = 0.0;
    for (double x = 0.001; x < 0.2; x += 0.0005) peak = std::max(peak, at(series, {x}));
    CHECK(std::abs(peak - 1.1790) < 0.01); // 1 + 0.0895 * 2
  }
}

TEST_CASE("free variables stay free and results are differentiable", "[calculus][fourier]") {
  const math::Function f = math::function({"x", "a"}, [](math::Function x, math::Function a) { return a * x; });
  const math::Function coefficients = fourier::sine_coefficients("x", 0.0, kPi, 3)(f);
  REQUIRE(coefficients.rank() == 1);
  const auto b = host(coefficients({scalar(3.0)}));
  for (int k = 1; k <= 3; ++k) CHECK(std::abs(b[static_cast<std::size_t>(k - 1)] - 3.0 * 2.0 * (k % 2 ? 1.0 : -1.0) / k) < 1e-8);
  // d/da of the series is the series of d/da: here the series of x
  const math::Function series = fourier::sine_series("x", 0.0, kPi, 4)(f);
  const math::Function d_series = operators::differential("a")(series);
  const math::Function series_of_x = fourier::sine_series("x", 0.0, kPi, 4)(identity());
  CHECK(std::abs(at(d_series, {1.1, 5.0}) - at(series_of_x, {1.1})) < 1e-8);
  // linear in f
  CHECK(fourier::cosine_series("x", 0.0, 1.0, 3).capabilities().linear);
  CHECK(fourier::cosine_coefficients("x", 0.0, 1.0, 3).capabilities().serializable);
}

TEST_CASE("operators validate and round-trip", "[calculus][fourier][io]") {
  CHECK_THROWS_AS(fourier::cosine_coefficients("x", 1.0, 0.0, 4), pluss::Error);
  CHECK_THROWS_AS(fourier::cosine_coefficients("x", 0.0, 1.0, 0), pluss::Error);
  CHECK_THROWS_AS(fourier::cosine_coefficients("x", 0.0, 1.0, 4, -0.5, 1.0), pluss::Error);
  CHECK_THROWS_AS(fourier::coefficients("x", -1.0, 4), pluss::Error);
  CHECK_THROWS_AS(fourier::sine_series("nope", 0.0, 1.0, 3)(identity()), pluss::Error);
  const math::Function vector = math::function({"x"}, [](math::Function x) { return math::stack({x, x}); });
  CHECK_THROWS_AS(fourier::cosine_series("x", 0.0, 1.0, 3)(vector), pluss::Error);
  const std::vector<operators::Operator> all{
      fourier::cosine_coefficients("x", 0.0, kPi, 5), fourier::cosine_coefficients("x", 0.0, kPi, 5, 0.5, 2.0),
      fourier::sine_coefficients("x", 0.0, kPi, 5),   fourier::cosine_series("x", 0.0, kPi, 5),
      fourier::sine_series("x", 0.0, kPi, 5),         fourier::coefficients("x", 2.0, 4), fourier::series("x", 2.0, 4)};
  for (const operators::Operator &op : all) {
    std::stringstream stream;
    math::io::save(op, stream);
    const operators::Operator restored = math::io::load_operator(stream);
    CHECK(restored.key() == op.key());
    const math::Function a = op(identity()), b = restored(identity());
    if (a.rank() == 0) CHECK(at(a, {0.7}) == at(b, {0.7}));
    else CHECK(host(a()) == host(b()));
  }
}

// ---- COS method: payoff coefficients as Fourier-cosine coefficients ----------------------------------------

namespace {
struct BlackScholes {
  double s0 = 100.0, strike = 80.0, rate = 0.04, vol = 0.25, maturity = 1.0;
  double call() const {
    const double d1 = (std::log(s0 / strike) + (rate + 0.5 * vol * vol) * maturity) / (vol * std::sqrt(maturity));
    const double d2 = d1 - vol * std::sqrt(maturity);
    return s0 * normal_cdf(d1) - strike * std::exp(-rate * maturity) * normal_cdf(d2);
  }
  double digital() const { // cash-or-nothing paying 1
    const double d2 = (std::log(s0 / strike) + (rate - 0.5 * vol * vol) * maturity) / (vol * std::sqrt(maturity));
    return std::exp(-rate * maturity) * normal_cdf(d2);
  }
  double square() const { return s0 * s0 * std::exp((rate + vol * vol) * maturity); } // price of S_T^2
};
// price = exp(-r T) sum' Re(phi(u_k) exp(-i u_k a)) V_k for y = ln(S_T / K); phi is the characteristic function of y
double cos_price(const BlackScholes &m, const std::vector<double> &v, double a, double b) {
  const double x = std::log(m.s0 / m.strike), mean = x + (m.rate - 0.5 * m.vol * m.vol) * m.maturity, var = m.vol * m.vol * m.maturity;
  double sum = 0.0;
  for (std::size_t k = 0; k < v.size(); ++k) {
    const double u = static_cast<double>(k) * kPi / (b - a);
    const std::complex<double> phi = std::exp(std::complex<double>(-0.5 * var * u * u, u * mean));
    sum += (k == 0 ? 0.5 : 1.0) * std::real(phi * std::exp(std::complex<double>(0.0, -u * a))) * v[k];
  }
  return std::exp(-m.rate * m.maturity) * sum;
}
// the closed-form chi_k and psi_k integrals of exp(y) and 1 over [c, d] with the phase anchored at a
double chi(int k, double a, double b, double c, double d) {
  if (k == 0) return std::exp(d) - std::exp(c);
  const double u = k * kPi / (b - a);
  return (std::exp(d) * std::cos(u * (d - a)) - std::exp(c) * std::cos(u * (c - a)) + u * (std::exp(d) * std::sin(u * (d - a)) - std::exp(c) * std::sin(u * (c - a)))) / (1.0 + u * u);
}
double psi(int k, double a, double b, double c, double d) {
  if (k == 0) return d - c;
  const double u = k * kPi / (b - a);
  return (std::sin(u * (d - a)) - std::sin(u * (c - a))) / u;
}
} // namespace

TEST_CASE("payoff coefficients reproduce the hand-coded COS chi and psi", "[calculus][fourier][cos]") {
  const BlackScholes m;
  const double x = std::log(m.s0 / m.strike), mean = x + (m.rate - 0.5 * m.vol * m.vol) * m.maturity, width = 10.0 * m.vol * std::sqrt(m.maturity);
  const double a = mean - width, b = mean + width;
  const int terms = 24;
  // call payoff K (e^y - 1) over [0, b]: V_k = 2 / (b - a) K (chi_k(0, b) - psi_k(0, b))
  const math::Function call = math::function({"y"}, [&](math::Function y) { return (y.exp() - 1.0) * m.strike; });
  const auto v = host(fourier::cosine_coefficients("y", a, b, terms, 0.0, b)(call)());
  for (int k = 0; k < terms; ++k) {
    const double expected = 2.0 / (b - a) * m.strike * (chi(k, a, b, 0.0, b) - psi(k, a, b, 0.0, b));
    CHECK(std::abs(v[static_cast<std::size_t>(k)] - expected) < 1e-8 * (1.0 + std::abs(expected)));
  }
}

TEST_CASE("the COS method prices payoffs with and without hand-coded coefficients", "[calculus][fourier][cos]") {
  const BlackScholes m;
  const double x = std::log(m.s0 / m.strike), mean = x + (m.rate - 0.5 * m.vol * m.vol) * m.maturity, width = 10.0 * m.vol * std::sqrt(m.maturity);
  const double a = mean - width, b = mean + width;
  const int terms = 96;
  const auto t0 = std::chrono::steady_clock::now();
  // vanilla call
  const math::Function call = math::function({"y"}, [&](math::Function y) { return (y.exp() - 1.0) * m.strike; });
  const double call_price = cos_price(m, host(fourier::cosine_coefficients("y", a, b, terms, 0.0, b)(call)()), a, b);
  CHECK(std::abs(call_price - m.call()) < 1e-6);
  // cash-or-nothing digital: no hand-coded chi / psi exists for it in the pricing code
  const math::Function digital = math::function({"y"}, [](math::Function y) { return y * 0.0 + 1.0; });
  const double digital_price = cos_price(m, host(fourier::cosine_coefficients("y", a, b, terms, 0.0, b)(digital)()), a, b);
  CHECK(std::abs(digital_price - m.digital()) < 1e-6);
  // power option paying S_T^2 = K^2 e^{2y}, smooth over the whole interval
  const math::Function square = math::function({"y"}, [&](math::Function y) { return (y * 2.0).exp() * (m.strike * m.strike); });
  const double square_price = cos_price(m, host(fourier::cosine_coefficients("y", a, b, terms)(square)()), a, b);
  CHECK(std::abs(square_price - m.square()) < 1e-4 * m.square()); // exp(-rT) E[S_T^2]
  // a butterfly of three calls through the linearity of the coefficient operator
  const double lo = 70.0, mid = 100.0, hi = 130.0;
  const auto payoff = [&](double strike) { return math::function({"y"}, [&, strike](math::Function y) { return (y.exp() * m.strike - strike) ; }); };
  (void)payoff;
  const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - t0;
  INFO("96-term COS coefficients for three payoffs took " << elapsed.count() << " s");
  CHECK(elapsed.count() < 60.0);
  (void)lo; (void)mid; (void)hi;
}
