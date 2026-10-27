#include "pk_reference.h"
#include "pluss/subsp/natsci/biology.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace pk = pluss::subsp::natsci::biology::pharmacokinetics;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
constexpr int B = 3, N = 3, T = 5;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mk(const double *p, std::vector<std::int64_t> shape) {
  std::int64_t n = 1;
  for (auto s : shape)
    n *= s;
  return pluss::make_series(std::vector<double>(p, p + n), shape).to(F64);
}
double max_diff(const Series &a, const double *ref) {
  const Series f = a.reshape({a.numel()});
  double w = 0;
  for (std::int64_t i = 0; i < f.size(0); ++i)
    w = std::max(w, std::abs(f.select(0, i).item<double>() - ref[i]));
  return w;
}
Series build_k() { return pk::rate_matrix(mk(pk_reference::T, {B, N, N}), mk(pk_reference::el, {B, N})); }

void test_response_infusion_auc_steady() {
  const Series k = build_k(), x0 = mk(pk_reference::x0, {B, N}), u = mk(pk_reference::u, {B, N}), t = mk(pk_reference::t, {T});
  expect(max_diff(pk::response(k, x0, t), pk_reference::resp) < 1e-11, "bolus response vs scipy expm");
  expect(max_diff(pk::infusion_response(k, x0, u, t), pk_reference::infusion) < 1e-11, "infusion response vs scipy expm");
  expect(max_diff(pk::auc_infinity(k, x0), pk_reference::auc) < 1e-11, "AUC vs scipy solve");
  expect(max_diff(pk::steady_state_trough(k, x0, 6.0), pk_reference::ss) < 1e-10, "steady-state trough vs scipy");
  // mass balance: total amount only decreases (elimination) and is bounded by the dose
  const Series total = pk::response(k, x0, t).sum(-1, false); // [B, T]
  expect(total.select(-1, 0).sub(x0.sum(-1, false)).abs().max().item<double>() < 1e-12, "t=0 returns the dose");
  expect(total.slice(-1, 1, T, 1).sub(total.slice(-1, 0, T - 1, 1)).max().item<double>() < 1e-12, "total amount non-increasing");
}

void test_bateman_and_macro() {
  const Series tt = mk(pk_reference::bat_t, {4});
  expect(max_diff(pk::bateman(tt, 100.0, 1.3, 0.4), pk_reference::bat) < 1e-12, "bateman vs closed form");
  // ka -> ke limit is continuous
  const Series a = pk::bateman(tt, 100.0, 0.4, 0.4), b = pk::bateman(tt, 100.0, 0.4 + 1e-6, 0.4);
  expect(a.sub(b).abs().max().item<double>() < 1e-3, "bateman ka=ke limit");
  // the ODE solution of the same model agrees: 2-compartment gut->central via the matrix exponential
  const Series transfer = pluss::make_series(std::vector<double>{0, 1.3, 0, 0}, {2, 2}).to(F64);
  const Series k = pk::rate_matrix(transfer, pluss::series::make({0.0, 0.4}).to(F64));
  const Series x = pk::response(k, pluss::series::make({100.0, 0.0}).to(F64), tt).select(-1, 1);
  expect(x.sub(pk::bateman(tt, 100.0, 1.3, 0.4)).abs().max().item<double>() < 1e-10, "expm vs bateman");
  // macro constants are the eigenvalues of K for a 2-compartment model: tr K = -(a+b), det K = a b
  const Series k10 = pluss::series::make({0.3}).to(F64), k12 = pluss::series::make({0.5}).to(F64), k21 = pluss::series::make({0.2}).to(F64);
  const auto m = pk::two_compartment_macro(k10, k12, k21);
  const Series km = pk::rate_matrix(pluss::make_series(std::vector<double>{0, 0.5, 0.2, 0}, {2, 2}).to(F64),
                                    pluss::series::make({0.3, 0.0}).to(F64));
  const double tr = km.select(0, 0).select(0, 0).item<double>() + km.select(0, 1).select(0, 1).item<double>();
  const double det = km.select(0, 0).select(0, 0).item<double>() * km.select(0, 1).select(0, 1).item<double>() -
                     km.select(0, 0).select(0, 1).item<double>() * km.select(0, 1).select(0, 0).item<double>();
  expect(std::abs(m.alpha.host<double>().front() + m.beta.host<double>().front() + tr) < 1e-12 &&
             std::abs(m.alpha.host<double>().front() * m.beta.host<double>().front() - det) < 1e-12,
         "macro constants vs tr/det of K");
  expect(m.alpha.host<double>().front() > m.beta.host<double>().front(), "alpha > beta");
}

void test_autodiff_through_expm() {
  // d x_total(t)/d k_el via reverse mode vs central difference (one batched call each)
  auto tel = pluss::subsp::math::diff::trace(mk(pk_reference::el, {B, N}), /*tracks_slope=*/true);
  const Series el = tel.value();
  const Series t = pluss::series::make({3.0}).to(F64), x0 = mk(pk_reference::x0, {B, N});
  const Series tm = mk(pk_reference::T, {B, N, N});
  const Series y = pk::response(pk::rate_matrix(tm, el), x0, t).sum();
  y.reverse();
  const Series g = el.sensitivity();
  const double h = 1e-6;
  double worst = 0;
  for (int b = 0; b < B; ++b)
    for (int i = 0; i < N; ++i) {
      std::vector<double> e(pk_reference::el, pk_reference::el + B * N);
      e[b * N + i] += h;
      const double up = pk::response(pk::rate_matrix(tm, pluss::make_series(e, {B, N}).to(F64)), x0, t).sum().item<double>();
      e[b * N + i] -= 2 * h;
      const double dn = pk::response(pk::rate_matrix(tm, pluss::make_series(e, {B, N}).to(F64)), x0, t).sum().item<double>();
      worst = std::max(worst, std::abs(g.select(0, b).select(0, i).item<double>() - (up - dn) / (2 * h)));
    }
  expect(worst < 1e-6, "expm autodiff vs finite difference, worst " + std::to_string(worst));
}
} // namespace

int main() {
  try {
    test_response_infusion_auc_steady();
    test_bateman_and_macro();
    test_autodiff_through_expm();
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "pharmacokinetics tests passed\n";
  return 0;
}
