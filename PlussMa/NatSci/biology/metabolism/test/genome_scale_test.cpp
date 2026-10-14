// Genome-scale check on E. coli iJO1366 (1805 metabolites, 2583 reactions, 1367 genes) against COBRApy.
// FBA is solved exactly by the simplex; sampled FVA ranges and sampled gene deletions are warm-started from that optimum.
// Slow (minutes): registered with the CTest label "slow" (exclude with `ctest -LE slow`).
#include "ijo1366_reference.h"
#include "pluss/subsp/natsci/biology.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace mt = pluss::subsp::natsci::biology::metabolism;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
Series mk(const std::vector<double> &v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(F64); }
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
double seconds_since(std::chrono::steady_clock::time_point t) { return std::chrono::duration<double>(std::chrono::steady_clock::now() - t).count(); }
} // namespace

int main() {
  namespace ref = ijo1366_reference;
  try {
    std::vector<double> dense(static_cast<std::size_t>(ref::m) * static_cast<std::size_t>(ref::r), 0.0);
    for (std::size_t k = 0; k < ref::s_val.size(); ++k)
      dense[static_cast<std::size_t>(ref::s_row[k]) * static_cast<std::size_t>(ref::r) + static_cast<std::size_t>(ref::s_col[k])] = ref::s_val[k];
    mt::Network net;
    net.stoichiometry = mk(dense, {ref::m, ref::r});
    net.lower = mk(ref::lower, {ref::r});
    net.upper = mk(ref::upper, {ref::r});
    const Series obj = mk(ref::objective, {ref::r});

    auto t0 = std::chrono::steady_clock::now();
    const auto wt = mt::flux_balance(net, obj);
    std::cout << "iJO1366 FBA (exact simplex): objective " << wt.objective.item<double>() << " vs COBRApy " << ref::optimum << "  [" << seconds_since(t0) << " s]\n" << std::flush;
    expect(wt.status.item<double>() == 0.0, "FBA optimal");
    expect(std::abs(wt.objective.item<double>() - ref::optimum) < 1e-8 * (1 + ref::optimum), "genome-scale FBA optimum vs COBRApy");

    // sampled FVA: the first 8 of the sampled reactions (16 LPs) as a sub-network objective sweep warm-started from the optimum
    const int nf = 8;
    std::vector<double> rows;
    t0 = std::chrono::steady_clock::now();
    double worst_fva = 0;
    {
      // each lane minimises / maximises one reaction subject to growth >= z*
      pluss::subsp::math::linprog::Problem p;
      const std::int64_t L = 2 * nf;
      std::vector<double> cm(static_cast<std::size_t>(L * ref::r), 0.0);
      for (int k = 0; k < nf; ++k) {
        cm[static_cast<std::size_t>(k) * static_cast<std::size_t>(ref::r) + static_cast<std::size_t>(ref::fva_index[static_cast<std::size_t>(k)])] = 1.0;
        cm[static_cast<std::size_t>(nf + k) * static_cast<std::size_t>(ref::r) + static_cast<std::size_t>(ref::fva_index[static_cast<std::size_t>(k)])] = -1.0;
      }
      p.c = mk(cm, {L, ref::r});
      p.a_eq = net.stoichiometry;
      p.b_eq = pluss::series::zeros({L, ref::m}, pluss::Config{}.with_dtype(F64));
      p.a_ub = obj.neg().unsqueeze(0);
      p.b_ub = pluss::series::zeros({L, 1}, pluss::Config{}.with_dtype(F64)).add(-wt.objective.item<double>());
      p.lower = net.lower;
      p.upper = net.upper;
      std::vector<std::int64_t> basis = wt.basis[0];
      std::vector<char> upper = wt.at_upper[0];
      basis.push_back(static_cast<std::int64_t>(upper.size()));
      upper.push_back(0);
      p.warm_basis = basis;
      p.warm_at_upper = upper;
      const auto sol = pluss::subsp::math::linprog::solve(p, mt::exact_options());
      expect(sol.status.eq(0.0).all().item<bool>(), "FVA lanes optimal");
      int warm = 0, pivots_max = sol.iterations;
      for (const char w : sol.warm_started)
        warm += w;
      std::cout << "  warm starts engaged in " << warm << "/" << L << " lanes (max pivots " << pivots_max << ")\n";
      expect(warm == L, "every FVA lane must be warm-started from the wild-type basis");
      const auto obj_v = sol.objective.to_vec<double>();
      for (int k = 0; k < nf; ++k) {
        const double mn = obj_v[static_cast<std::size_t>(k)], mx = -obj_v[static_cast<std::size_t>(nf + k)];
        worst_fva = std::max({worst_fva, std::abs(mn - ref::fva_min[static_cast<std::size_t>(k)]), std::abs(mx - ref::fva_max[static_cast<std::size_t>(k)])});
      }
    }
    std::cout << "iJO1366 FVA, " << nf << " sampled reactions (" << 2 * nf << " LPs, warm-started): worst |diff| vs COBRApy " << worst_fva << "  [" << seconds_since(t0) << " s]\n" << std::flush;
    expect(worst_fva < 1e-6, "genome-scale FVA vs COBRApy");

    // sampled gene deletions through the GPR rules, warm-started from the wild-type basis
    std::vector<std::string> rules, genes;
    for (const char *rule : ref::gene_rules)
      rules.emplace_back(rule);
    for (const char *g : ref::gene_names)
      genes.emplace_back(g);
    const mt::GeneRules gr(rules, genes);
    const int ng = 12;
    std::vector<double> mask(static_cast<std::size_t>(ng) * genes.size(), 0.0);
    for (int k = 0; k < ng; ++k)
      mask[static_cast<std::size_t>(k) * genes.size() + static_cast<std::size_t>(ref::gene_index[static_cast<std::size_t>(k)])] = 1.0;
    t0 = std::chrono::steady_clock::now();
    const auto ko = mt::gene_knockouts(net, gr, obj, mk(mask, {ng, static_cast<std::int64_t>(genes.size())}));
    double worst_ko = 0;
    for (int k = 0; k < ng; ++k)
      worst_ko = std::max(worst_ko, std::abs(ko.objective.select(0, k).item<double>() - ref::gene_growth[static_cast<std::size_t>(k)]));
    std::cout << "iJO1366 " << ng << " single-gene deletions (warm-started dual simplex): worst |diff| vs COBRApy " << worst_ko << "  [" << seconds_since(t0) << " s]\n";
    expect(worst_ko < 1e-6, "genome-scale gene deletions vs COBRApy");
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "genome-scale tests passed\n";
  return 0;
}
