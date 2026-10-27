// Matrix-population demography against the R package popbio (its own functions generated the reference), exact special cases and identities.
#include "pluss/subsp/natsci/biology/population/demography.h"
#include "demography_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace dg = pluss::subsp::natsci::biology::population::demography;
namespace st = pluss::subsp::natsci::biology::population::structured;
namespace ref = demography_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mat(const std::vector<double> &v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(F64); }
Series vec(const std::vector<double> &v) { return mat(v, {static_cast<std::int64_t>(v.size())}); }
double rel_diff(const Series &a, const std::vector<double> &b) {
  const auto av = a.contiguous().host<double>();
  expect(av.size() == b.size(), "size mismatch");
  double w = 0.0;
  for (std::size_t i = 0; i < av.size(); ++i)
    w = std::max(w, std::abs(av[i] - b[i]) / (1e-12 + std::abs(b[i])));
  return w;
}
} // namespace

int main() {
  try {
    // ---- the three popbio matrices: eigen quantities, damping, R0, generation time, fundamental matrix
    struct Case { const std::vector<double> *a, *lam, *stable, *repro, *sens, *elas, *damp, *damp_exact, *r0, *gen, *n, *nvar, *eta, *etavar; std::int64_t k; };
    const Case cases[3] = {
        {&ref::m1_A, &ref::m1_lambda, &ref::m1_stable, &ref::m1_repro, &ref::m1_sens, &ref::m1_elas, &ref::m1_damping, &ref::m1_damping_exact, &ref::m1_R0, &ref::m1_gen, &ref::m1_N, &ref::m1_Nvar, &ref::m1_eta, &ref::m1_etavar, 3},
        {&ref::m2_A, &ref::m2_lambda, &ref::m2_stable, &ref::m2_repro, &ref::m2_sens, &ref::m2_elas, &ref::m2_damping, &ref::m2_damping_exact, &ref::m2_R0, &ref::m2_gen, &ref::m2_N, &ref::m2_Nvar, &ref::m2_eta, &ref::m2_etavar, 4},
        {&ref::m3_A, &ref::m3_lambda, &ref::m3_stable, &ref::m3_repro, &ref::m3_sens, &ref::m3_elas, &ref::m3_damping, &ref::m3_damping_exact, &ref::m3_R0, &ref::m3_gen, &ref::m3_N, &ref::m3_Nvar, &ref::m3_eta, &ref::m3_etavar, 5}};
    for (int c = 0; c < 3; ++c) {
      const auto &cs = cases[c];
      const std::string tag = "matrix " + std::to_string(c + 1) + ": ";
      const Series a = mat(*cs.a, {cs.k, cs.k});
      const st::Eigen e = st::dominant(a);
      expect(e.converged, tag + "power iteration converged");
      expect(rel_diff(e.lambda, *cs.lam) < 1e-9, tag + "lambda");
      expect(rel_diff(e.right, *cs.stable) < 1e-8, tag + "stable stage");
      expect(rel_diff(e.left.div(e.left.select(-1, 0)), *cs.repro) < 1e-8, tag + "reproductive value");
      expect(rel_diff(st::sensitivity(e), *cs.sens) < 1e-8, tag + "sensitivities");
      expect(rel_diff(st::elasticity(a, e), *cs.elas) < 1e-8, tag + "elasticities");
      expect(rel_diff(dg::damping_ratio(a), *cs.damp_exact) < 1e-8, tag + "damping ratio vs |eigenvalue| ratio from R eigen()");
      expect(rel_diff(dg::damping_ratio(a), *cs.damp) < 2e-5, tag + "damping ratio vs popbio damping.ratio (which rounds the moduli to 5 decimals)");
      const Series mask = dg::first_row_fertility_mask(cs.k, a);
      expect(rel_diff(dg::net_reproductive_rate(a, mask), *cs.r0) < 1e-8, tag + "net reproductive rate");
      expect(rel_diff(dg::generation_time(a, mask), *cs.gen) < 1e-8, tag + "generation time");
      const auto fm = dg::fundamental_matrix(a.sub(a.mul(mask)));
      expect(rel_diff(fm.n, *cs.n) < 1e-9 && rel_diff(fm.variance, *cs.nvar) < 1e-8, tag + "fundamental matrix and variance");
      expect(rel_diff(fm.expected_lifetime, *cs.eta) < 1e-9 && rel_diff(fm.lifetime_variance, *cs.etavar) < 1e-8, tag + "expected lifetime and its variance");
    }
    // ---- imprimitive Leslie matrix: tied moduli give damping ratio 1
    {
      const Series a = mat({0, 0, 2, 0.3, 0, 0, 0, 0.6, 0}, {3, 3});
      expect(std::abs(dg::damping_ratio(a).item<double>() - 1.0) < 1e-6, "imprimitive matrix: damping ratio 1");
    }
    // ---- batched lanes: several matrices in one call agree with one-at-a-time
    {
      const Series stack = mat([] { std::vector<double> v = ref::m1_A; v.insert(v.end(), ref::m1_A.begin(), ref::m1_A.end()); for (double &x : v) x *= 1.0; return v; }(), {2, 3, 3});
      const auto d = dg::damping_ratio(stack).contiguous().host<double>();
      expect(std::abs(d[0] - d[1]) < 1e-12 && std::abs(d[0] - ref::m1_damping_exact[0]) < 1e-8, "lane-batched damping ratio");
    }
    // ---- LTRE
    {
      const Series trt = mat(ref::ltre_trt, {3, 3}), rf = mat(ref::ltre_ref, {3, 3});
      const Series c = dg::ltre_fixed(trt, rf);
      expect(rel_diff(c, ref::ltre_contrib) < 1e-8, "fixed LTRE contributions vs popbio LTRE");
      const double total = c.sum().item<double>(), diff = ref::ltre_lambda_trt[0] - ref::ltre_lambda_ref[0];
      expect(std::abs(total - diff) < 1e-2 * std::abs(diff), "contributions sum to the change in lambda (midpoint rule): " + std::to_string(total) + " vs " + std::to_string(diff));
    }
    // ---- lambda sensitivity to lower-level parameters (chain rule) vs finite differences of lambda itself
    {
      const auto build = [](const Series &th) { // th [..., 4] = fertility, s1, s2, stasis
        const Series f = th.select(-1, 0), s1 = th.select(-1, 1), s2 = th.select(-1, 2), g = th.select(-1, 3);
        const Series zero = f.mul(0.0);
        const auto row = [&](const std::vector<Series> &c) { std::vector<Series> r; for (std::size_t i = 1; i < c.size(); ++i) r.push_back(c[i].unsqueeze(-1)); return c[0].unsqueeze(-1).unite(r, c[0].dim()); };
        const Series r0 = row({zero.add(0.2), zero, f.mul(2.0)}), r1 = row({s1, zero.add(0.3), zero}), r2 = row({zero, s2, g});
        return r0.unsqueeze(-2).unite(std::vector<Series>{r1.unsqueeze(-2), r2.unsqueeze(-2)}, r0.dim() - 1);
      };
      const Series theta = vec({1.7, 0.5, 0.45, 0.85});
      const Series grad = dg::lambda_gradient(build, theta);
      std::vector<double> fd;
      for (std::int64_t p = 0; p < 4; ++p) {
        std::vector<double> up = theta.host<double>(), dn = up;
        const double h = 1e-6 * up[static_cast<std::size_t>(p)];
        up[static_cast<std::size_t>(p)] += h;
        dn[static_cast<std::size_t>(p)] -= h;
        fd.push_back((st::dominant(build(vec(up))).lambda.item<double>() - st::dominant(build(vec(dn))).lambda.item<double>()) / (2.0 * h));
      }
      expect(rel_diff(grad, fd) < 1e-5, "d lambda / d theta by the chain rule vs finite differences of lambda: " + std::to_string(rel_diff(grad, fd)));
    }
    // ---- transient amplification: the 2-norm bound is attained by the top right singular vector and never exceeded
    {
      const Series a = mat(ref::m2_A, {4, 4});
      const auto amp = dg::amplification(a, 12);
      expect(amp.maximum.item<double>() >= amp.first_step.item<double>() - 1e-12, "maximum amplification >= first-step amplification");
      const Series lam = st::dominant(a).lambda;
      const std::int64_t t = static_cast<std::int64_t>(std::llround(amp.time_of_maximum.item<double>()));
      Series power = a.div(lam);
      for (std::int64_t i = 1; i < t; ++i)
        power = power.matmul(a.div(lam));
      const auto dec = pluss::subsp::linearalg::svd(power, false);
      const Series v = dec.vh.select(0, 0); // top right singular vector
      const double attained = power.matmul(v.unsqueeze(-1)).square().sum().sqrt().item<double>();
      expect(std::abs(attained - amp.maximum.item<double>()) < 1e-9 * amp.maximum.item<double>(), "the top right singular vector attains the maximum amplification");
      const Series rnd = mat({0.3, 0.9, 0.1, 0.5}, {4, 1});
      const double ratio = power.matmul(rnd).square().sum().sqrt().item<double>() / rnd.square().sum().sqrt().item<double>();
      expect(ratio <= amp.maximum.item<double>() + 1e-12, "no start vector exceeds the bound");
    }

    // ---- life table: Euler-Lotka root; identity with the Leslie matrix built from the same l_x and f_x (ages shifted by one)
    {
      const Series fert = vec({0.0, 0.0, 1.2, 2.5, 3.1, 1.4}), surv = vec({0.55, 0.7, 0.8, 0.75, 0.6});
      const Series leslie = st::leslie(fert, surv);
      const Series l = surv.cumprod(-1);
      const Series lx = pluss::series::ones({1}, pluss::Config{}.with_dtype(F64)).unite(std::vector<Series>{l}, 0);
      const Series ages = vec({1, 2, 3, 4, 5, 6});
      const auto lt = dg::life_table(lx, fert, ages);
      const double lam = st::dominant(leslie).lambda.item<double>();
      expect(std::abs(lt.lambda.item<double>() - lam) < 1e-12, "Euler-Lotka lambda equals the Leslie dominant eigenvalue: " + std::to_string(lt.lambda.item<double>()) + " vs " + std::to_string(lam));
      expect(std::abs(lt.r0.item<double>() - st::net_reproductive_rate(fert, surv).item<double>()) < 1e-12, "R0 sum l m matches the Leslie net reproductive rate");
      const Series resid = lx.mul(fert).mul(ages.mul(lt.r).neg().exp()).sum().sub(1.0);
      expect(std::abs(resid.item<double>()) < 1e-13, "Euler-Lotka equation holds at r");
      // the stable age distribution is the Leslie right eigenvector; Fisher's reproductive value is proportional to the left eigenvector
      const st::Eigen e = st::dominant(leslie);
      expect(e.right.sub(lt.stable_age).abs().max().item<double>() < 1e-9, "stable age distribution is the Leslie right eigenvector");
      const Series v_leslie = e.left.div(e.left.select(-1, 0)), v_fisher = lt.reproductive_value.div(lt.reproductive_value.select(-1, 0));
      expect(v_leslie.sub(v_fisher).abs().max().item<double>() < 1e-9, "Fisher reproductive value is the Leslie left eigenvector");
      // batched lanes
      const Series lanes_l = lx.unsqueeze(0).expand({3, 6}), lanes_m = fert.unsqueeze(0).mul(vec({1.0, 1.1, 0.9}).unsqueeze(-1));
      const auto lt3 = dg::life_table(lanes_l, lanes_m, ages);
      expect(std::abs(lt3.lambda.select(0, 0).item<double>() - lam) < 1e-12 && lt3.lambda.select(0, 1).item<double>() > lam && lt3.lambda.select(0, 2).item<double>() < lam, "lane-batched life table is monotone in fecundity");
    }
    // ---- continuous Euler-Lotka with exponential survival and fecundity: int m0 e^{-c a} e^{-mu a} e^{-r a} da = m0 / (c + mu + r) = 1  =>  r = m0 - c - mu
    {
      const double m0 = 3.0, c = 0.4, mu = 0.35, h = 0.005;
      const std::int64_t n = 4000; // ages up to 20
      const Series a = pluss::series::arange(0, n, 1, F64).mul(h).add(0.5 * h);
      const auto lt = dg::life_table(a.mul(-mu).exp(), a.mul(-c).exp().mul(m0), a, 60, a.mul(0.0).add(h));
      expect(std::abs(lt.r.item<double>() - (m0 - c - mu)) < 1e-4, "continuous Euler-Lotka rate: " + std::to_string(lt.r.item<double>()) + " vs " + std::to_string(m0 - c - mu));
      expect(std::abs(lt.r0.item<double>() - m0 / (c + mu)) < 1e-4, "R0 = m0 / (c + mu)");
      expect(std::abs(lt.cohort_generation_time.item<double>() - 1.0 / (c + mu)) < 1e-4, "cohort generation time 1 / (c + mu)");
    }
    // ---- Tuljapurkar approximation vs popbio, and exact scalar cases
    {
      const Series env = mat(ref::env_A, {4, 3, 3}), p = vec(ref::env_p);
      const auto tj = dg::tuljapurkar(env, p, true);
      expect(std::abs(tj.log_lambda_s.item<double>() - ref::env_approx_popbio[0]) < 1e-9, "Tuljapurkar approximation vs popbio stoch.growth.rate: " + std::to_string(tj.log_lambda_s.item<double>()) + " vs " + std::to_string(ref::env_approx_popbio[0]));
      // 1 x 1 matrices: S = 1 and the approximation is ln(mean a) - Var(a) / (2 mean(a)^2)
      const Series scalars = vec({2.0, 0.5, 1.3}).reshape({3, 1, 1}), q = vec({0.2, 0.5, 0.3});
      const auto ts = dg::tuljapurkar(scalars, q, false);
      const double mean = 0.2 * 2.0 + 0.5 * 0.5 + 0.3 * 1.3, var = 0.2 * 4.0 + 0.5 * 0.25 + 0.3 * 1.69 - mean * mean;
      expect(std::abs(ts.log_lambda_s.item<double>() - (std::log(mean) - var / (2.0 * mean * mean))) < 1e-12, "scalar Tuljapurkar approximation");
    }
    // ---- simulated stochastic growth: exact scalar answer sum p ln a, and the popbio simulation / approximation
    {
      dg::Environment scalar{vec({2.0, 0.5, 1.3}).reshape({3, 1, 1}), vec({0.2, 0.5, 0.3}), pluss::series::undefined()};
      const auto sg = dg::simulate_growth(scalar, 800, 500, 17);
      const double exact = 0.2 * std::log(2.0) + 0.5 * std::log(0.5) + 0.3 * std::log(1.3);
      expect(std::abs(sg.mean - exact) < 5.0 * sg.standard_error + 1e-12, "scalar simulation matches sum p ln a: " + std::to_string(sg.mean) + " +- " + std::to_string(sg.standard_error) + " vs " + std::to_string(exact));
      dg::Environment env{mat(ref::env_A, {4, 3, 3}), vec(ref::env_p), pluss::series::undefined()};
      const auto sim = dg::simulate_growth(env, 1500, 400, 23);
      expect(std::abs(sim.mean - ref::env_sim_popbio[0]) < 6e-3, "simulated ln lambda_s vs popbio simulation: " + std::to_string(sim.mean) + " vs " + std::to_string(ref::env_sim_popbio[0]));
      expect(std::abs(sim.mean - ref::env_approx_popbio[0]) < 1.5e-2, "simulation near the small-noise approximation");
      // Markov environments with an identity transition freeze the environment: growth rate is a mixture of ln lambda_k with the start probabilities
      dg::Environment frozen{env.matrices, env.prob, pluss::series::eye(4, pluss::Config{}.with_dtype(F64))};
      const auto fr = dg::simulate_growth(frozen, 400, 800, 5);
      double mix = 0.0;
      for (std::int64_t k = 0; k < 4; ++k)
        mix += ref::env_p[static_cast<std::size_t>(k)] * std::log(st::dominant(env.matrices.select(0, k)).lambda.item<double>());
      expect(std::abs(fr.mean - mix) < 8.0 * fr.standard_error + 5e-3, "frozen Markov environment: " + std::to_string(fr.mean) + " vs mixture " + std::to_string(mix));
    }
    // ---- quasi-extinction against the exact reflection-principle recursion of a symmetric random walk of ln N (a in {1/2, 2}, p = 1/2)
    {
      dg::Environment walk{vec({0.5, 2.0}).reshape({2, 1, 1}), vec({0.5, 0.5}), pluss::series::undefined()};
      const std::int64_t steps = 20;
      const auto paths = dg::simulate_paths(walk, vec({8.0}), steps, 20000, 31);
      const Series q = dg::quasi_extinction_probability(paths, 1.0); // N <= 1 means a net decline of 3 doublings
      // exact: probability a simple +-1 walk started at 0 reaches -3 within t steps (DP with an absorbing barrier)
      std::vector<double> dp(8, 0.0);
      std::vector<double> exact(static_cast<std::size_t>(steps + 1), 0.0);
      dp[3] = 1.0; // position 0 is index 3 (barrier at index 0 = -3), up to +4
      double absorbed = 0.0;
      for (std::int64_t t = 1; t <= steps; ++t) {
        std::vector<double> nx(8, 0.0);
        for (int i = 1; i < 8; ++i) {
          if (dp[static_cast<std::size_t>(i)] == 0.0) continue;
          nx[static_cast<std::size_t>(i - 1)] += 0.5 * dp[static_cast<std::size_t>(i)];
          if (i + 1 < 8) nx[static_cast<std::size_t>(i + 1)] += 0.5 * dp[static_cast<std::size_t>(i)];
          else nx[static_cast<std::size_t>(i)] += 0.5 * dp[static_cast<std::size_t>(i)];
        }
        absorbed += nx[0];
        nx[0] = 0.0;
        dp = nx;
        exact[static_cast<std::size_t>(t)] = absorbed;
      }
      // the truncated upper boundary only affects paths that first climb 4 levels: negligible bias for 20 steps (checked by tolerance)
      const auto qv = q.contiguous().host<double>();
      double worst = 0.0;
      for (std::size_t t = 0; t < qv.size(); ++t)
        worst = std::max(worst, std::abs(qv[t] - exact[t]));
      expect(worst < 0.02, "quasi-extinction probability vs the exact walk recursion, worst " + std::to_string(worst));
      expect(qv.front() == 0.0 && qv.back() > 0.3, "extinction probability starts at 0 and grows");
    }
    std::cout << "demography tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
