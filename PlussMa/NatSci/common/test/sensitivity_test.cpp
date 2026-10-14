#include "pluss/subsp/natsci/common/sensitivity.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace sens = pluss::subsp::natsci::common::sensitivity;
namespace dyn = pluss::subsp::natsci::common::dynamics;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mk(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(F64); }
double last_time_value(const Series &s) { return s.item<double>(); }

void test_exponential_decay_nd_lanes() {
  // lanes [2,3]: y' = -k y, S = dy/dk = -t y0 exp(-k t)
  const Series y0 = mk({1, 2, 3, 4, 5, 6}, {2, 3, 1}), k = mk({0.3, 0.5, 0.7, 0.9, 1.1, 1.3}, {2, 3, 1});
  const auto r = sens::trajectory([](const Series &y, const Series &p) { return y.mul(p.select(-1, 0).unsqueeze(-1).neg()); }, y0, k, 0.0, 2.0, 4);
  expect(r.dydp.dim() == 5, "sensitivity tensor rank [T,2,3,n,P]");
  const Series tt = mk({0.0, 0.5, 1.0, 1.5, 2.0}, {5, 1, 1, 1, 1});
  const Series exact = tt.neg().mul(y0.unsqueeze(0).unsqueeze(-1)).mul(k.unsqueeze(0).unsqueeze(-1).mul(tt.neg()).exp());
  const double w = r.dydp.sub(exact).abs().max().item<double>();
  expect(w < 1e-8, "decay sensitivity vs closed form, worst " + std::to_string(w));
}

void test_logistic_two_params() {
  const double r0 = 0.8, K0 = 10.0, y00 = 1.0;
  const Series y0 = mk({y00}, {1}), p = mk({r0, K0}, {2});
  const auto s = sens::trajectory(
      [](const Series &y, const Series &q) {
        return q.select(-1, 0).unsqueeze(-1).mul(y).mul(y.div(q.select(-1, 1).unsqueeze(-1)).neg().add(1.0));
      },
      y0, p, 0.0, 3.0, 3);
  const double t = 3.0, E = std::exp(r0 * t), D = K0 + y00 * (E - 1.0);
  const double dr = K0 * y00 * t * E * (K0 - y00) / (D * D), dK = y00 * y00 * E * (E - 1.0) / (D * D);
  expect(std::abs(s.dydp.select(0, 3).select(0, 0).select(0, 0).item<double>() - dr) < 1e-8, "logistic dy/dr");
  expect(std::abs(s.dydp.select(0, 3).select(0, 0).select(0, 1).item<double>() - dK) < 1e-8, "logistic dy/dK");
}

void test_michaelis_menten_vs_finite_difference_of_simulation() {
  auto f = [](const Series &y, const Series &p) {
    return y.mul(p.select(-1, 0).unsqueeze(-1)).div(p.select(-1, 1).unsqueeze(-1).add(y)).neg();
  };
  const Series y0 = mk({5.0}, {1}), p = mk({2.0, 1.5}, {2});
  const auto s = sens::trajectory(f, y0, p, 0.0, 2.0, 2, 100);
  const double h = 1e-5;
  for (int j = 0; j < 2; ++j) {
    std::vector<double> up{2.0, 1.5}, dn{2.0, 1.5};
    up[j] += h;
    dn[j] -= h;
    auto run = [&](const std::vector<double> &q) {
      const Series qq = mk(q, {2});
      return dyn::simulate([&](const Series &y) { return f(y, qq); }, y0, 0.0, 2.0, 200).item<double>();
    };
    const double fd = (run(up) - run(dn)) / (2 * h);
    const double got = s.dydp.select(0, 2).select(0, 0).select(0, j).item<double>();
    expect(std::abs(got - fd) < 1e-6, "MM sensitivity vs FD of the full simulation, param " + std::to_string(j));
  }
}

void test_identifiability() {
  // y' = -(a+b) y: only a+b is identifiable (rank 1); z' = a y - b z observed with y: rank 2
  auto sum_model = [](const Series &y, const Series &p) {
    return y.mul(p.select(-1, 0).add(p.select(-1, 1)).unsqueeze(-1)).neg();
  };
  auto cascade = [](const Series &y, const Series &p) {
    const Series a = p.select(-1, 0), b = p.select(-1, 1);
    const Series y1 = y.select(-1, 0), y2 = y.select(-1, 1);
    return y1.mul(a).neg().unsqueeze(-1).unite(std::vector<Series>{y1.mul(a).sub(y2.mul(b)).unsqueeze(-1)}, -1);
  };
  const Series p = mk({0.4, 0.7}, {2});
  const auto s1 = sens::trajectory(sum_model, mk({3.0}, {1}), p, 0.0, 4.0, 8);
  const double rank1 = sens::numerical_rank(sens::fisher_spectrum(sens::fisher_information(s1.dydp)), 1e-8).item<double>();
  expect(rank1 == 1.0, "non-identifiable parameter pair has rank 1, got " + std::to_string(rank1));
  const auto s2 = sens::trajectory(cascade, mk({3.0, 0.0}, {2}), p, 0.0, 4.0, 8);
  const Series fim = sens::fisher_information(s2.dydp);
  const double rank2 = sens::numerical_rank(sens::fisher_spectrum(fim), 1e-8).item<double>();
  expect(rank2 == 2.0, "cascade is identifiable, rank " + std::to_string(rank2));
  expect(sens::cramer_rao_std(fim).gt(0.0).all().item<bool>(), "Cramer-Rao std positive");
}
} // namespace

int main() {
  try {
    test_exponential_decay_nd_lanes();
    test_logistic_two_params();
    test_michaelis_menten_vs_finite_difference_of_simulation();
    test_identifiability();
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "sensitivity tests passed\n";
  return 0;
}
