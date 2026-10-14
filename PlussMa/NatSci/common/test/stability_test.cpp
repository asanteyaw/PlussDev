#include "pluss/subsp/natsci/common/stability.h"
#include "stability_reference.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace dyn = pluss::subsp::natsci::common::dynamics;
namespace stab = pluss::subsp::natsci::common::stability;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series batch(const double *p, std::int64_t b, std::int64_t n) {
  return pluss::make_series(std::vector<double>(p, p + b * n * n), {b, n, n}).to(F64);
}

template <std::int64_t N> void check_dimension(const double *mats, const double *counts, const double *margins, const double *polys) {
  const std::int64_t b = 400;
  const Series a = batch(mats, b, N);
  const Series poly = stab::characteristic_polynomial(a);
  expect(poly.size(0) == b && poly.size(1) == N + 1, "polynomial shape");
  const Series want = pluss::make_series(std::vector<double>(polys, polys + b * (N + 1)), {b, N + 1}).to(F64);
  const Series scale = want.abs().amax(-1, true).clamp(1.0, 1e300);
  expect(poly.sub(want).abs().div(scale).max().item<double>() < 1e-9, "characteristic polynomial vs numpy.poly, n = " + std::to_string(N));
  const auto r = stab::routh(poly);
  std::int64_t checked = 0, wrong = 0;
  for (std::int64_t i = 0; i < b; ++i) {
    if (margins[i] < 1e-6)
      continue; // too close to the imaginary axis to classify in floating point
    ++checked;
    if (std::llround(r.unstable.select(0, i).item<double>()) != std::llround(counts[i]))
      ++wrong;
  }
  expect(checked > 300 && wrong == 0, "Routh-Hurwitz count of right-half-plane eigenvalues vs numpy.linalg.eigvals, n = " + std::to_string(N) +
                                          " (wrong " + std::to_string(wrong) + " of " + std::to_string(checked) + ")");
  // the same count through the convenience wrapper
  expect(stab::unstable_modes(a).unstable.sub(r.unstable).abs().max().item<double>() == 0.0, "unstable_modes == routh(characteristic_polynomial)");
}

void test_matrices() {
  check_dimension<2>(stability_reference::mats_2, stability_reference::count_2, stability_reference::margin_2, stability_reference::poly_2);
  check_dimension<3>(stability_reference::mats_3, stability_reference::count_3, stability_reference::margin_3, stability_reference::poly_3);
  check_dimension<4>(stability_reference::mats_4, stability_reference::count_4, stability_reference::margin_4, stability_reference::poly_4);
  check_dimension<6>(stability_reference::mats_6, stability_reference::count_6, stability_reference::margin_6, stability_reference::poly_6);
}

void test_marginal_and_known() {
  // pure imaginary eigenvalues (a centre): flagged marginal, no root in the open right half plane
  const Series centre = pluss::series::make({{0.0, -1.0}, {1.0, 0.0}}).to(F64);
  const auto c = stab::unstable_modes(centre);
  expect(c.marginal.item<double>() == 1.0 && c.unstable.item<double>() == 0.0, "centre is marginal with no unstable modes");
  // diag(-1, -2, 3): exactly one unstable mode, not marginal
  const Series diag = pluss::series::make({{-1.0, 0.0, 0.0}, {0.0, -2.0, 0.0}, {0.0, 0.0, 3.0}}).to(F64);
  const auto d = stab::unstable_modes(diag);
  expect(d.unstable.item<double>() == 1.0 && d.marginal.item<double>() == 0.0, "diag(-1,-2,3) has one unstable mode");
  // lane-wise bisection: root of x^2 - 2 per lane (different brackets)
  const Series lo = pluss::series::make({0.0, 1.0}).to(F64), hi = pluss::series::make({3.0, 2.0}).to(F64);
  const Series root = pluss::subsp::math::roots::bisect([](const Series &x) { return x.square().sub(2.0); }, lo, hi).root;
  expect(root.sub(std::sqrt(2.0)).abs().max().item<double>() < 1e-12, "bisect finds sqrt(2) in every lane");
}
} // namespace

int main() {
  const auto run = [](const char *name, void (*fn)()) {
    try {
      fn();
    } catch (const std::exception &e) {
      std::cerr << name << ": " << e.what() << "\n";
      std::exit(1);
    }
  };
  run("matrices", test_matrices);
  run("marginal_and_known", test_marginal_and_known);
  std::cout << "NatSci stability checks passed\n";
}
