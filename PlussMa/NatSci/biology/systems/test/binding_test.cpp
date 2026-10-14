#include "binding_reference.h"
#include "pluss/subsp/natsci/biology.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace bd = pluss::subsp::natsci::biology::systems::binding;
namespace dyn = pluss::subsp::natsci::biology::dynamics;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
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
double rel_diff(const Series &a, const double *ref) {
  const Series f = a.reshape({a.numel()});
  double w = 0;
  for (std::int64_t i = 0; i < f.size(0); ++i)
    w = std::max(w, std::abs(f.select(0, i).item<double>() - ref[i]) / (std::abs(ref[i]) + 1e-300));
  return w;
}

void test_receptor_ligand() {
  const double w = rel_diff(bd::receptor_ligand(mk(binding_reference::rl_R, {12}), mk(binding_reference::rl_L, {12}),
                                                  dyn::Coeff(mk(binding_reference::rl_Kd, {12}))),
                            binding_reference::rl);
  expect(w < 1e-12, "receptor_ligand vs brentq, worst rel " + std::to_string(w));
}

void test_competitive() {
  const Series r = mk(binding_reference::c_R, {8});
  const Series l = mk(binding_reference::c_A, {8}).unsqueeze(-1).unite(std::vector<Series>{mk(binding_reference::c_B, {8}).unsqueeze(-1)}, -1);
  const Series kd = mk(binding_reference::c_Ka, {8}).unsqueeze(-1).unite(std::vector<Series>{mk(binding_reference::c_Kb, {8}).unsqueeze(-1)}, -1);
  const auto s = bd::competitive(r, l, kd);
  const double w = rel_diff(s.occupancy, binding_reference::c_occ);
  expect(w < 1e-8, "competitive occupancy vs brentq, worst rel " + std::to_string(w));
  const Series total = s.free_receptor.add(s.occupancy.sum(-1, false).mul(r));
  expect(total.sub(r).div(r).abs().max().item<double>() < 1e-9, "receptor conserved");
}

void test_mwc() {
  const Series s = mk(binding_reference::mwc_s, {13});
  const double w = rel_diff(bd::mwc(s, 2.0, 0.05, 500.0, 4), binding_reference::mwc);
  expect(w < 1e-12, "mwc vs explicit enumeration, worst rel " + std::to_string(w));
  const Series lang = pluss::subsp::natsci::biology::systems::kinetics::hill_activation(s, 1.0, 2.0, 1.0);
  expect(bd::mwc(s, 2.0, 1.0, 17.0, 4).sub(lang).abs().max().item<double>() < 1e-13, "mwc(c=1) == langmuir");
}

void test_adair() {
  const Series lb = pluss::series::make({std::log(5.0), std::log(40.0), std::log(100.0)}).to(F64);
  const double w = rel_diff(bd::adair(mk(binding_reference::ad_R, {9}), mk(binding_reference::ad_L, {9}), lb), binding_reference::ad);
  expect(w < 1e-8, "adair vs brentq, worst rel " + std::to_string(w));
}

void test_large_batch() {
  const Series r = pluss::series::linspace(-2.0, 1.0, 20, F64).exp().reshape({20, 1, 1, 1}).expand({20, 20, 20, 20});
  const Series l = pluss::series::linspace(-2.0, 2.0, 20, F64).exp().reshape({1, 20, 1, 1}).expand({20, 20, 20, 20});
  const Series k = pluss::series::linspace(-3.0, 1.0, 20, F64).exp().reshape({1, 1, 20, 1}).expand({20, 20, 20, 20});
  const Series rl = bd::receptor_ligand(r, l, dyn::Coeff(k));
  const Series resid = rl.mul(k).sub(r.sub(rl).mul(l.sub(rl))).abs().div(r.mul(l)).max();
  expect(resid.item<double>() < 1e-10, "4-D lane grid satisfies mass action");
}
} // namespace

int main() {
  try {
    test_receptor_ligand();
    test_competitive();
    test_mwc();
    test_adair();
    test_large_batch();
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "binding tests passed\n";
  return 0;
}
