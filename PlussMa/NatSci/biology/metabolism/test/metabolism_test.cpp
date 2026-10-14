#include "metabolism_reference.h"
#include "pluss/subsp/natsci/biology.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace mt = pluss::subsp::natsci::biology::metabolism;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mk(const std::vector<double> &v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(F64); }
double at(const Series &s, std::int64_t i) { return s.select(0, i).item<double>(); }

// Toy network (hand-solvable):  R1: -> A  [0,10];  R2: A -> B  [0,8];  R3: A -> C [0,3];  R4: B ->  [0,100] (objective);  R5: C -> [0,100]
mt::Network toy() {
  //            R1  R2  R3  R4  R5
  const std::vector<double> s = {1, -1, -1, 0, 0,   // A
                                 0, 1, 0, -1, 0,    // B
                                 0, 0, 1, 0, -1};   // C
  mt::Network n;
  n.stoichiometry = mk(s, {3, 5});
  n.lower = mk({0, 0, 0, 0, 0}, {5});
  n.upper = mk({10, 8, 3, 100, 100}, {5});
  return n;
}

void test_toy_network() {
  const auto net = toy();
  const Series obj = mk({0, 0, 0, 1, 0}, {5});
  const auto sol = mt::flux_balance(net, obj);
  expect(std::abs(sol.objective.item<double>() - 8.0) < 1e-7, "toy optimum is the R2 capacity");
  // shadow prices d objective / d b for S v = b: A is not scarce (R2's capacity binds) -> 0; one extra unit of
  // B consumption demand removes one unit of v4 = v2 - b_B -> -1; C is balanced by free R5 -> 0
  expect(std::abs(sol.shadow_price.select(0, 0).item<double>()) < 1e-6, "shadow price of A");
  expect(std::abs(sol.shadow_price.select(0, 1).item<double>() + 1.0) < 1e-6, "shadow price of B");
  expect(std::abs(sol.shadow_price.select(0, 2).item<double>()) < 1e-6, "shadow price of C");
  // FVA at the optimum: v1 = 10 - ... total A made = v2 + v3 = 10 with v2 = 8 forced -> v3 in [2,2]? A is made at <=10: v2=8 so v1 in [8,10], v3 in [0,2]
  const auto fva = mt::flux_variability(net, obj, 1.0);
  const std::vector<double> mn = {8, 8, 0, 8, 0}, mx = {10, 8, 2, 8, 2};
  for (int j = 0; j < 5; ++j) {
    expect(std::abs(at(fva.minimum, j) - mn[static_cast<std::size_t>(j)]) < 1e-6, "toy FVA minimum " + std::to_string(j));
    expect(std::abs(at(fva.maximum, j) - mx[static_cast<std::size_t>(j)]) < 1e-6, "toy FVA maximum " + std::to_string(j));
  }
  // knockouts: R2 -> 0 objective; R3 -> 8; R1 -> 0
  const auto ko = mt::single_knockouts(net, obj);
  const std::vector<double> want = {0, 0, 8, 0, 8}; // R4 is the objective reaction itself
  for (int j = 0; j < 5; ++j)
    expect(std::abs(at(ko.objective, j) - want[static_cast<std::size_t>(j)]) < 1e-6, "toy knockout " + std::to_string(j));
  // sweep of the uptake R1: objective = min(v1, 8)
  const auto sw = mt::sweep_flux(net, obj, 0, mk({1, 4, 8, 10}, {4}));
  const std::vector<double> sw_want = {1, 4, 8, 8};
  for (int k = 0; k < 4; ++k)
    expect(std::abs(at(sw.objective, k) - sw_want[static_cast<std::size_t>(k)]) < 1e-6, "toy sweep " + std::to_string(k));
}

void test_e_coli_core_against_cobra() {
  namespace ref = metabolism_reference;
  mt::Network net;
  net.stoichiometry = mk(ref::S, {ref::m, ref::r});
  net.lower = mk(ref::lower, {ref::r});
  net.upper = mk(ref::upper, {ref::r});
  const Series obj = mk(ref::objective, {ref::r});
  const auto sol = mt::flux_balance(net, obj);
  expect(sol.status.item<double>() == 0.0 || sol.status.item<double>() == 4.0, "e_coli_core: optimal status");
  std::cout << "e_coli_core optimum " << sol.objective.item<double>() << " vs COBRApy " << ref::optimum << "\n";
  expect(std::abs(sol.objective.item<double>() - ref::optimum) < 1e-6 * (1 + ref::optimum), "e_coli_core optimum " + std::to_string(sol.objective.item<double>()) + " vs COBRApy " + std::to_string(ref::optimum));
  // independent steady-state / bound check on the returned flux
  const Series resid = sol.flux.matmul(net.stoichiometry.transpose(0, 1)).abs().max();
  expect(resid.item<double>() < 1e-6, "S v = 0 residual " + std::to_string(resid.item<double>()));
  expect(sol.flux.sub(net.upper).max().item<double>() < 1e-6 && net.lower.sub(sol.flux).max().item<double>() < 1e-6, "flux within bounds");

  for (const double frac : {1.0, 0.9}) {
    
    const auto fva = mt::flux_variability(net, obj, frac);
    const auto &rmin = frac == 1.0 ? ref::fva_min : ref::fva90_min;
    const auto &rmax = frac == 1.0 ? ref::fva_max : ref::fva90_max;
    double worst = 0;
    for (int j = 0; j < ref::r; ++j)
      worst = std::max({worst, std::abs(at(fva.minimum, j) - rmin[static_cast<std::size_t>(j)]), std::abs(at(fva.maximum, j) - rmax[static_cast<std::size_t>(j)])});
    std::cout << "e_coli_core FVA(fraction " << frac << ") worst |diff| vs COBRApy over " << 2 * ref::r << " bounds: " << worst << "\n";
    expect(worst < 1e-8, "e_coli_core FVA(" + std::to_string(frac) + ") vs COBRApy, worst abs " + std::to_string(worst));
  }

  const auto ko = mt::single_knockouts(net, obj);
  double worst = 0;
  for (int j = 0; j < ref::r; ++j)
    worst = std::max(worst, std::abs(at(ko.objective, j) - ref::knockout_growth[static_cast<std::size_t>(j)]));
  std::cout << "e_coli_core single knockouts (" << ref::r << " LPs, one batched call) worst |diff| vs COBRApy: " << worst << "\n";
  expect(worst < 1e-8, "e_coli_core single knockouts vs COBRApy, worst abs " + std::to_string(worst));

  const auto sw = mt::sweep_flux(net, obj, ref::glucose_exchange, mk(ref::sweep_values, {5}));
  for (int k = 0; k < 5; ++k)
    expect(std::abs(at(sw.objective, k) - ref::sweep_growth[static_cast<std::size_t>(k)]) < 1e-6, "glucose-uptake sweep " + std::to_string(k));
}

// Network with an explicit thermodynamic loop:  R1: -> A [0,10];  R2: A -> B [0,100];  R3: B -> A [0,100];  R4: B -> [0,100] (objective)
// Plain FVA lets R2 reach 100 and R3 reach 90 by cycling A <-> B; loopless ranges are R2 = 10, R3 = 0 (analytic).
void test_loopless_toy_network() {
  mt::Network net;
  //            R1  R2  R3  R4
  net.stoichiometry = mk({1, -1, 1, 0,   // A
                          0, 1, -1, -1}, // B
                         {2, 4});
  net.lower = mk({0, 0, 0, 0}, {4});
  net.upper = mk({10, 100, 100, 100}, {4});
  const Series obj = mk({0, 0, 0, 1}, {4});
  const auto plain = mt::flux_variability(net, obj, 1.0);
  expect(std::abs(at(plain.maximum, 1) - 100.0) < 1e-7 && std::abs(at(plain.maximum, 2) - 90.0) < 1e-7, "plain FVA exposes the A<->B loop");
  const auto ll = mt::loopless_flux_variability(net, obj, 1.0);
  const std::vector<double> want_min = {10, 10, 0, 10}, want_max = {10, 10, 0, 10};
  for (int j = 0; j < 4; ++j) {
    expect(std::abs(at(ll.minimum, j) - want_min[static_cast<std::size_t>(j)]) < 1e-7, "toy loopless minimum " + std::to_string(j));
    expect(std::abs(at(ll.maximum, j) - want_max[static_cast<std::size_t>(j)]) < 1e-7, "toy loopless maximum " + std::to_string(j));
  }
  // the exact loop-law MILP (Schellenberger et al. 2011) must agree with the CycleFreeFlux screen
  for (int j = 1; j <= 2; ++j) {
    const auto range = mt::loopless_range_exact(net, obj, j, 1.0);
    expect(std::abs(range.first - want_min[static_cast<std::size_t>(j)]) < 1e-6 && std::abs(range.second - want_max[static_cast<std::size_t>(j)]) < 1e-6,
           "exact loop-law MILP range of reaction " + std::to_string(j) + ": [" + std::to_string(range.first) + "," + std::to_string(range.second) + "]");
  }
}

void test_gene_rules_parser() {
  // (g1 and g2) or g3, no association, nested, mixed case / symbol operators
  mt::GeneRules rules({"(g1 and g2) or g3", "", "g1 AND (g2 OR g3)", "g1 & g2 | g3", "g3"});
  expect(rules.gene_count() == 3, "three genes parsed");
  // present = [g1,g2,g3]; rows are gene-presence patterns
  const Series present = mk({1, 1, 1,  0, 1, 0,  1, 0, 0,  0, 0, 1}, {4, 3});
  const Series act = rules.active(present); // [4, 5]
  const std::vector<std::vector<double>> want = {{1, 1, 1, 1, 1},   // all present
                                                 {0, 1, 0, 0, 0},   // only g2
                                                 {0, 1, 0, 0, 0},   // only g1: (g1&g2)|g3 -> 0, g1&(g2|g3) -> 0, g1&g2|g3 -> 0
                                                 {1, 1, 0, 1, 1}};  // only g3
  for (int k = 0; k < 4; ++k)
    for (int j = 0; j < 5; ++j)
      expect(act.select(0, k).select(0, j).item<double>() == want[static_cast<std::size_t>(k)][static_cast<std::size_t>(j)],
             "GPR evaluation lane " + std::to_string(k) + " rule " + std::to_string(j));
  bool threw = false;
  try {
    mt::GeneRules bad({"(g1 and"});
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  expect(threw, "malformed rule is rejected");
}

mt::Network e_coli() {
  namespace ref = metabolism_reference;
  mt::Network net;
  net.stoichiometry = mk(ref::S, {ref::m, ref::r});
  net.lower = mk(ref::lower, {ref::r});
  net.upper = mk(ref::upper, {ref::r});
  return net;
}

void test_gene_knockouts_against_cobra() {
  namespace ref = metabolism_reference;
  const auto net = e_coli();
  const Series obj = mk(ref::objective, {ref::r});
  std::vector<std::string> rules;
  for (const char *rule : ref::gene_rules)
    rules.emplace_back(rule);
  std::vector<std::string> genes;
  for (const char *g : ref::gene_names)
    genes.emplace_back(g);
  const mt::GeneRules gr(rules, genes);
  expect(gr.gene_count() == static_cast<std::int64_t>(genes.size()), "no unknown genes in the rules");
  const auto single = mt::single_gene_knockouts(net, gr, obj);
  double worst = 0;
  for (std::size_t g = 0; g < genes.size(); ++g)
    worst = std::max(worst, std::abs(at(single.objective, static_cast<std::int64_t>(g)) - ref::gene_knockout_growth[g]));
  std::cout << "e_coli_core " << genes.size() << " single-gene deletions (one batched call) worst |diff| vs COBRApy: " << worst << "\n";
  expect(worst < 1e-6, "single gene knockouts vs COBRApy");
  std::vector<double> mask(40 * genes.size(), 0.0);
  for (int k = 0; k < 40; ++k) {
    mask[static_cast<std::size_t>(k) * genes.size() + static_cast<std::size_t>(ref::double_pairs[static_cast<std::size_t>(2 * k)])] = 1.0;
    mask[static_cast<std::size_t>(k) * genes.size() + static_cast<std::size_t>(ref::double_pairs[static_cast<std::size_t>(2 * k + 1)])] = 1.0;
  }
  const auto dbl = mt::gene_knockouts(net, gr, obj, mk(mask, {40, static_cast<std::int64_t>(genes.size())}));
  double worst2 = 0;
  for (int k = 0; k < 40; ++k)
    worst2 = std::max(worst2, std::abs(at(dbl.objective, k) - ref::double_knockout_growth[static_cast<std::size_t>(k)]));
  std::cout << "e_coli_core 40 double-gene deletions worst |diff| vs COBRApy: " << worst2 << "\n";
  expect(worst2 < 1e-6, "double gene knockouts vs COBRApy");
}

void test_loopless() {
  namespace ref = metabolism_reference;
  const auto net = e_coli();
  const Series obj = mk(ref::objective, {ref::r});
  // CycleFreeFlux on the same starting flux COBRApy used: same objective, same exchanges and directions, same minimal L1 norm
  const Series start = mk(ref::start_flux, {ref::r});
  const Series ll = mt::loopless_solution(net, obj, start);
  const Series boundary = mk(ref::boundary, {ref::r}), internal = boundary.neg().add(1.0);
  expect(ll.sub(start).mul(boundary).abs().max().item<double>() < 1e-5, "exchange fluxes preserved (box slack 1e-7 relative)");
  expect(ll.mul(start).ge(-1e-9).all().item<bool>(), "flux directions preserved");
  expect(ll.abs().sub(start.abs()).max().item<double>() < 1e-5, "no flux grows");
  expect(std::abs(ll.mul(obj).sum().item<double>() - start.mul(obj).sum().item<double>()) < 1e-5, "objective preserved");
  const double l1 = ll.abs().mul(internal).sum().item<double>(), l1_ref = mk(ref::loopless_flux, {ref::r}).abs().mul(internal).sum().item<double>();
  std::cout << "CycleFreeFlux internal L1 norm: " << l1 << " (COBRApy " << l1_ref << ", start " << start.abs().mul(internal).sum().item<double>() << ")\n";
  expect(std::abs(l1 - l1_ref) < 1e-5 * (1 + l1_ref), "minimal L1 norm equals COBRApy's");

  // a flux that really contains a loop (FVA maximum of FRD7 = 994.9, cycling through SUCDi/FRD7): cleaned entry by entry like COBRApy
  const Series loopy = mk(ref::loopy_start, {ref::r});
  const Series cleaned = mt::loopless_solution(net, obj, loopy);
  double worst_clean = 0;
  for (int j = 0; j < ref::r; ++j)
    worst_clean = std::max(worst_clean, std::abs(at(cleaned, j) - ref::loopy_cleaned[static_cast<std::size_t>(j)]));
  std::cout << "CycleFreeFlux on a looping flux (internal L1 " << loopy.abs().mul(internal).sum().item<double>() << " -> " << cleaned.abs().mul(internal).sum().item<double>()
            << "), worst |diff| vs COBRApy over " << ref::r << " fluxes: " << worst_clean << "\n";
  expect(worst_clean < 1e-5, "loop removal matches COBRApy entry by entry");
  expect(at(cleaned, 43) < 1e-6, "FRD7 loop removed");

  const auto fva = mt::flux_variability(net, obj, 1.0);
  const auto lfva = mt::loopless_flux_variability(net, obj, 1.0);
  double worst = 0;
  for (int j = 0; j < ref::r; ++j) {
    worst = std::max({worst, std::abs(at(lfva.minimum, j) - ref::ll_fva_min[static_cast<std::size_t>(j)]), std::abs(at(lfva.maximum, j) - ref::ll_fva_max[static_cast<std::size_t>(j)])});
    expect(at(lfva.minimum, j) >= at(fva.minimum, j) - 1e-6 && at(lfva.maximum, j) <= at(fva.maximum, j) + 1e-6, "loopless range lies inside the FVA range");
  }
  for (int j = 0; j < ref::r; ++j) {
    const double a = at(lfva.minimum, j), b = ref::ll_fva_min[static_cast<std::size_t>(j)], c2 = at(lfva.maximum, j), d = ref::ll_fva_max[static_cast<std::size_t>(j)];
    if (std::abs(a - b) > 1e-5 || std::abs(c2 - d) > 1e-5)
      std::cout << "  rxn " << j << " mine [" << a << "," << c2 << "] cobra [" << b << "," << d << "] plain FVA [" << at(fva.minimum, j) << "," << at(fva.maximum, j) << "]\n";
  }
  std::cout << "e_coli_core loopless FVA worst |diff| vs COBRApy cycleFreeFlux over " << 2 * ref::r << " bounds: " << worst << "\n";
  expect(worst < 1e-5, "loopless FVA vs COBRApy");
}
} // namespace

int main() {
  try {
    test_toy_network();
    test_e_coli_core_against_cobra();
    test_loopless_toy_network();
    test_gene_rules_parser();
    test_gene_knockouts_against_cobra();
    test_loopless();
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "metabolism tests passed\n";
  return 0;
}
