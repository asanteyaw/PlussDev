// Fixed-width forward-mode scalars for objectives written as long scalar recurrences.
//
// A GARCH-type likelihood or any state-space filter is a loop of a few scalar operations repeated once per
// observation. Recording such a loop on the reverse-mode tape costs several microseconds per operation, so a
// 6,000-step fit spends minutes in autodiff. `Fwd<N>` carries a value and N exact partial derivatives in plain
// doubles instead: no tape, no allocation, and one forward pass yields the whole gradient. Use it when the
// number of parameters N is small (up to a few dozen); for large parameter counts reverse mode is cheaper.
//
//   using F = optim::Fwd<5>;
//   auto nll = [&](const std::array<F, 5>& p) -> F { ... loop using p[0] .. p[4], sqrt(), log() ... };
//   auto problem = optim::problem().objective_analytic(optim::fwd_objective<5>(nll)).build();
#pragma once

#include <array>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

#include "pluss/subsp/math/optim.h"

namespace pluss::subsp::math::optim {

template <int N>
struct Fwd {
  double v{0.0};
  std::array<double, N> d{};

  Fwd() = default;
  Fwd(double value) : v(value) {}  // NOLINT: constants convert implicitly
  static Fwd variable(double value, int index) {
    Fwd out(value);
    out.d[static_cast<std::size_t>(index)] = 1.0;
    return out;
  }
  double value() const noexcept { return v; }
  const std::array<double, N>& gradient() const noexcept { return d; }

  Fwd& operator+=(const Fwd& o) { v += o.v; for (int i = 0; i < N; ++i) d[i] += o.d[i]; return *this; }
  Fwd& operator-=(const Fwd& o) { v -= o.v; for (int i = 0; i < N; ++i) d[i] -= o.d[i]; return *this; }
};

template <int N> Fwd<N> operator-(Fwd<N> a) { a.v = -a.v; for (int i = 0; i < N; ++i) a.d[i] = -a.d[i]; return a; }
template <int N> Fwd<N> operator+(Fwd<N> a, const Fwd<N>& b) { a += b; return a; }
template <int N> Fwd<N> operator-(Fwd<N> a, const Fwd<N>& b) { a -= b; return a; }
template <int N> Fwd<N> operator*(const Fwd<N>& a, const Fwd<N>& b) {
  Fwd<N> r; r.v = a.v * b.v;
  for (int i = 0; i < N; ++i) r.d[i] = a.d[i] * b.v + a.v * b.d[i];
  return r;
}
template <int N> Fwd<N> operator/(const Fwd<N>& a, const Fwd<N>& b) {
  Fwd<N> r; r.v = a.v / b.v;
  for (int i = 0; i < N; ++i) r.d[i] = (a.d[i] - r.v * b.d[i]) / b.v;
  return r;
}
// mixed arithmetic with plain doubles
template <int N> Fwd<N> operator+(Fwd<N> a, double b) { a.v += b; return a; }
template <int N> Fwd<N> operator+(double a, Fwd<N> b) { b.v += a; return b; }
template <int N> Fwd<N> operator-(Fwd<N> a, double b) { a.v -= b; return a; }
template <int N> Fwd<N> operator-(double a, const Fwd<N>& b) { return Fwd<N>(a) - b; }
template <int N> Fwd<N> operator*(Fwd<N> a, double b) { a.v *= b; for (int i = 0; i < N; ++i) a.d[i] *= b; return a; }
template <int N> Fwd<N> operator*(double a, Fwd<N> b) { return b * a; }
template <int N> Fwd<N> operator/(Fwd<N> a, double b) { return a * (1.0 / b); }
template <int N> Fwd<N> operator/(double a, const Fwd<N>& b) { return Fwd<N>(a) / b; }

namespace fwd_detail {
template <int N> Fwd<N> chain(const Fwd<N>& a, double value, double slope) {
  Fwd<N> r; r.v = value;
  for (int i = 0; i < N; ++i) r.d[i] = a.d[i] * slope;
  return r;
}
}  // namespace fwd_detail

template <int N> Fwd<N> sqrt(const Fwd<N>& a) { const double s = std::sqrt(a.v); return fwd_detail::chain(a, s, 0.5 / s); }
template <int N> Fwd<N> log(const Fwd<N>& a) { return fwd_detail::chain(a, std::log(a.v), 1.0 / a.v); }
template <int N> Fwd<N> exp(const Fwd<N>& a) { const double e = std::exp(a.v); return fwd_detail::chain(a, e, e); }
template <int N> Fwd<N> tanh(const Fwd<N>& a) { const double t = std::tanh(a.v); return fwd_detail::chain(a, t, 1.0 - t * t); }
template <int N> Fwd<N> sin(const Fwd<N>& a) { return fwd_detail::chain(a, std::sin(a.v), std::cos(a.v)); }
template <int N> Fwd<N> cos(const Fwd<N>& a) { return fwd_detail::chain(a, std::cos(a.v), -std::sin(a.v)); }
template <int N> Fwd<N> abs(const Fwd<N>& a) { return a.v >= 0.0 ? a : -a; }
template <int N> Fwd<N> square(const Fwd<N>& a) { return fwd_detail::chain(a, a.v * a.v, 2.0 * a.v); }
template <int N> Fwd<N> pow(const Fwd<N>& a, double p) { return fwd_detail::chain(a, std::pow(a.v, p), p * std::pow(a.v, p - 1.0)); }

// Adapts a forward-mode objective to the analytic objective interface. The N parameters are the flattened
// values of the parameter blocks in order. A non-finite value yields a NaN gradient so the solvers treat the
// point as a non-finite region and back off.
template <int N, class Fn>
std::function<std::pair<Series, ParameterPack<Series>>(const ParameterPack<Series>&)> fwd_objective(Fn fn) {
  return [fn = std::move(fn)](const ParameterPack<Series>& params) -> std::pair<Series, ParameterPack<Series>> {
    if (params.total_numel() != N) {
      ::pluss::throw_error("optim::fwd_objective: the objective was built for " + std::to_string(N) +
                           " parameters but the problem has " + std::to_string(params.total_numel()));
    }
    const std::vector<double> x = flatten_values(params).to(pluss::ScalarType::Float64).to(pluss::pCPU).host<double>();
    std::array<Fwd<N>, N> p;
    for (int i = 0; i < N; ++i) p[i] = Fwd<N>::variable(x[static_cast<std::size_t>(i)], i);
    const Fwd<N> f = fn(p);
    std::vector<double> g(static_cast<std::size_t>(N));
    const bool finite = std::isfinite(f.v);
    for (int i = 0; i < N; ++i) g[static_cast<std::size_t>(i)] = finite ? f.d[i] : std::nan("");
    Series grad = pluss::make_series(g, {static_cast<std::int64_t>(N)}).to(pluss::ScalarType::Float64);
    const auto platform = params.empty() ? pluss::pCPU : params[0].value.platform();
    return {pluss::make_series(f.v).to(pluss::ScalarType::Float64).to(platform),
            unflatten_values(params.layout(), grad.to(platform))};
  };
}

}  // namespace pluss::subsp::math::optim
