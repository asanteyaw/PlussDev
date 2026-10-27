// Reaction-diffusion engine, Turing analysis and standard kinetics against analytic results, numpy eigenvalues and MATLAB pdepe.
#include "pluss/subsp/natsci/biology/spatial/reaction_diffusion.h"
#include "pluss/subsp/natsci/biology/spatial/turing.h"
#include "spatial_pdepe_reference.h"
#include "spatial_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace sp = pluss::subsp::natsci::biology::spatial;
namespace tu = pluss::subsp::natsci::biology::spatial::turing;
namespace dyn = pluss::subsp::natsci::biology::dynamics;
namespace ref = spatial_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double rel(double a, double b) { return std::abs(a - b) / (1e-12 + std::abs(b)); }
} // namespace

int main() {
  try {
    const double pi = M_PI;
    // ---- Laplacian: second-order convergence for each boundary type on analytic functions
    {
      const auto err = [&](sp::Boundary bc, double lower_value, double upper_value, int variant, std::int64_t n) {
        sp::Grid g = sp::box({n}, {1.0}, bc);
        g.axes[0].lower_value = lower_value;
        g.axes[0].upper_value = upper_value;
        std::vector<double> u(static_cast<std::size_t>(n)), exact(static_cast<std::size_t>(n));
        for (std::int64_t i = 0; i < n; ++i) {
          const double x = (static_cast<double>(i) + 0.5) / static_cast<double>(n);
          if (variant == 0) { u[static_cast<std::size_t>(i)] = std::cos(3.0 * pi * x); exact[static_cast<std::size_t>(i)] = -9.0 * pi * pi * std::cos(3.0 * pi * x); }   // Neumann, du/dn = 0
          if (variant == 1) { u[static_cast<std::size_t>(i)] = std::sin(2.0 * pi * x); exact[static_cast<std::size_t>(i)] = -4.0 * pi * pi * std::sin(2.0 * pi * x); }   // periodic
          if (variant == 2) { u[static_cast<std::size_t>(i)] = std::sin(pi * x) + 1.0 + x; exact[static_cast<std::size_t>(i)] = -pi * pi * std::sin(pi * x); }          // Dirichlet u(0) = 1, u(1) = 2
        }
        const Series l = sp::laplacian(vec(u), g);
        const auto lv = l.host<double>();
        double worst = 0.0;
        for (std::size_t i = 2; i + 2 < lv.size(); ++i) // away from the boundary cells (their truncation error has one factor of h more weight)
          worst = std::max(worst, std::abs(lv[i] - exact[i]));
        return worst;
      };
      for (int variant = 0; variant < 3; ++variant) {
        const sp::Boundary bc = variant == 0 ? sp::Boundary::Neumann : variant == 1 ? sp::Boundary::Periodic : sp::Boundary::Dirichlet;
        const double lo = variant == 2 ? 1.0 : 0.0, hi = variant == 2 ? 2.0 : 0.0;
        const double e1 = err(bc, lo, hi, variant, 64), e2 = err(bc, lo, hi, variant, 128);
        expect(e1 / e2 > 3.8, "Laplacian converges at second order, variant " + std::to_string(variant) + " ratio " + std::to_string(e1 / e2));
      }
    }
    // ---- heat equation: Neumann cell-centred eigenmodes cos(n pi x / L) decay exactly at D (4/h^2) sin^2(n pi / 2N) (and -> D (n pi / L)^2)
    {
      const std::int64_t n = 100;
      const double length = 2.0, d = 0.3, t = 0.5;
      const sp::Grid g = sp::box({n}, {length}, sp::Boundary::Neumann);
      std::vector<double> u(static_cast<std::size_t>(n));
      const int mode = 3;
      for (std::int64_t i = 0; i < n; ++i)
        u[static_cast<std::size_t>(i)] = std::cos(mode * pi * (static_cast<double>(i) + 0.5) * g.axes[0].spacing() / length);
      const Series u0 = vec(u).unsqueeze(0); // one species
      const Series out = sp::integrate(u0, vec({d}), g, nullptr, 0.0, t, 2000);
      const double h = g.axes[0].spacing(), lam = d * 4.0 / (h * h) * std::pow(std::sin(mode * pi / (2.0 * static_cast<double>(n))), 2.0);
      const auto ov = out.contiguous().host<double>();
      double worst = 0.0;
      for (std::size_t i = 0; i < ov.size(); ++i)
        worst = std::max(worst, std::abs(ov[i] - u[i] * std::exp(-lam * t)));
      expect(worst < 1e-9, "heat eigenmode decays at the exact discrete rate, worst " + std::to_string(worst));
      expect(rel(lam, d * std::pow(mode * pi / length, 2.0)) < 5e-3, "discrete rate approaches D (n pi / L)^2");
    }
    // ---- Turing analysis of random 2x2 systems against numpy eigenvalues
    {
      const std::size_t n = ref::tu_da.size();
      std::size_t checked = 0, turing_cases = 0;
      for (std::size_t i = 0; i < n; ++i) {
        const tu::Jacobian2 j{vec({ref::tu_J[4 * i]}), vec({ref::tu_J[4 * i + 1]}), vec({ref::tu_J[4 * i + 2]}), vec({ref::tu_J[4 * i + 3]})};
        const Series da = vec({ref::tu_da[i]}), db = vec({ref::tu_db[i]});
        for (std::size_t q = 0; q < 4; ++q) {
          const double got = tu::growth_rate(j, da, db, vec({ref::tu_k2_samples[q]})).host<double>().front();
          expect(std::abs(got - ref::tu_growth_samples[4 * i + q]) < 1e-9 * (1.0 + std::abs(got)), "growth rate vs numpy eigvals, case " + std::to_string(i));
        }
        const auto c = tu::turing_conditions(j, da, db);
        const bool stable0 = c.stable_without_diffusion.host<bool>().front(), turing = c.turing_unstable.host<bool>().front();
        const double margin = ref::tu_maxgrowth[i];
        expect(stable0 == (ref::tu_stable0[i] > 0.5) || std::abs(ref::tu_growth_samples[4 * i]) < 1e-9, "stability without diffusion, case " + std::to_string(i));
        if (std::abs(margin) > 1e-6 && ref::tu_stable0[i] > 0.5) {
          expect(turing == (ref::tu_turing[i] > 0.5), "Turing instability flag, case " + std::to_string(i));
          if (turing) {
            ++turing_cases;
            expect(rel(c.k2_lower.host<double>().front(), ref::tu_edges[2 * i]) < 3e-3 && rel(c.k2_upper.host<double>().front(), ref::tu_edges[2 * i + 1]) < 3e-3, "unstable band edges vs sign changes of numpy growth, case " + std::to_string(i));
            const double kc2 = c.k2_critical.host<double>().front();
            expect(kc2 > c.k2_lower.host<double>().front() && kc2 < c.k2_upper.host<double>().front(), "k_c^2 lies inside the unstable band");
          }
        }
        ++checked;
      }
      expect(checked == n && turing_cases >= 20, "enough Turing cases exercised: " + std::to_string(turing_cases));
    }
    // ---- steady states and Jacobians of the standard kinetics (numerical Jacobian via central differences)
    {
      const auto check = [&](const std::function<Series(const Series &)> &f, double u, double v, const tu::Jacobian2 &j, const std::string &name) {
        const Series state = vec({u, v});
        expect(f(state).abs().max().item<double>() < 1e-12, name + ": steady state");
        const Series jn = dyn::jacobian(f, state, 1e-6); // [2, 2]: row = output
        expect(std::abs(jn.select(0, 0).select(0, 0).host<double>().front() - j.fu.host<double>().front()) < 1e-6 && std::abs(jn.select(0, 0).select(0, 1).host<double>().front() - j.fv.host<double>().front()) < 1e-6 &&
                   std::abs(jn.select(0, 1).select(0, 0).host<double>().front() - j.gu.host<double>().front()) < 1e-6 && std::abs(jn.select(0, 1).select(0, 1).host<double>().front() - j.gv.host<double>().front()) < 1e-6,
               name + ": analytic Jacobian vs central differences");
      };
      const tu::Brusselator br{1.5, 3.0};
      const auto sb = br.steady_state();
      check([&](const Series &u) { return br.reaction(u, 0); }, sb.first, sb.second, br.jacobian(vec({0.0})), "Brusselator");
      const tu::Schnakenberg sc{0.1, 0.9};
      const auto ss = sc.steady_state();
      check([&](const Series &u) { return sc.reaction(u, 0); }, ss.first, ss.second, sc.jacobian(vec({0.0})), "Schnakenberg");
      for (std::size_t i = 0; i < ref::gs_cases.size() / 2; ++i) {
        const tu::GrayScott gs{ref::gs_cases[2 * i], ref::gs_cases[2 * i + 1]};
        if (std::isnan(ref::gs_states[4 * i]))
          continue;
        for (int root = 0; root < 2; ++root) {
          const auto st = gs.nontrivial_steady_state(root == 0);
          expect(std::abs(st.first - ref::gs_states[4 * i + 2 * static_cast<std::size_t>(root)]) < 1e-12 && std::abs(st.second - ref::gs_states[4 * i + 2 * static_cast<std::size_t>(root) + 1]) < 1e-12, "Gray-Scott steady state vs fsolve");
          check([&](const Series &u) { return gs.reaction(u, 0); }, st.first, st.second, gs.jacobian_at(st.first, st.second, vec({0.0})), "Gray-Scott");
        }
      }
    }

    // ---- PDE linear growth: a perturbation of the Brusselator steady state grows at Re lambda(k_d^2) of the discrete Laplacian eigenvalue
    {
      const tu::Brusselator br{1.0, 1.9};
      const double du = 1.0, dv = 8.0, length = 20.0;
      const std::int64_t n = 64;
      const sp::Grid g = sp::box({n}, {length}, sp::Boundary::Neumann);
      const double h = g.axes[0].spacing();
      const auto j = br.jacobian(vec({0.0}));
      const auto cond = tu::turing_conditions(j, vec({du}), vec({dv}));
      expect(cond.turing_unstable.host<bool>().front(), "Brusselator a=1, b=1.9, D=(1,8) is Turing unstable");
      int best_m = 1;
      double best = -1e9;
      for (int m = 1; m <= 12; ++m) {
        const double kd2 = 4.0 / (h * h) * std::pow(std::sin(m * pi / (2.0 * static_cast<double>(n))), 2.0);
        const double gr = tu::growth_rate(j, vec({du}), vec({dv}), vec({kd2})).host<double>().front();
        if (gr > best) { best = gr; best_m = m; }
      }
      expect(best > 0.0, "some mode on the grid is unstable");
      std::vector<double> mode(static_cast<std::size_t>(n)), u0(static_cast<std::size_t>(2 * n));
      const auto st = br.steady_state();
      const double eps = 1e-8;
      for (std::int64_t i = 0; i < n; ++i) {
        const double c = std::cos(best_m * pi * (static_cast<double>(i) + 0.5) * h / length);
        mode[static_cast<std::size_t>(i)] = c;
        u0[static_cast<std::size_t>(i)] = st.first + eps * c;
        u0[static_cast<std::size_t>(n + i)] = st.second + eps * 0.3 * c;
      }
      const Series state0 = pluss::make_series(u0, {2, n}).to(F64);
      const sp::Reaction f = [&](const Series &u) { return br.reaction(u, 1); };
      const Series s5 = sp::integrate(state0, vec({du, dv}), g, f, 0.0, 5.0, 1250);
      const Series s10 = sp::integrate(s5, vec({du, dv}), g, f, 5.0, 10.0, 1250);
      const auto amp = [&](const Series &s) { // projection of u - u* onto the mode
        const auto v = s.contiguous().host<double>();
        double num = 0.0, den = 0.0;
        for (std::int64_t i = 0; i < n; ++i) { num += (v[static_cast<std::size_t>(i)] - st.first) * mode[static_cast<std::size_t>(i)]; den += mode[static_cast<std::size_t>(i)] * mode[static_cast<std::size_t>(i)]; }
        return num / den;
      };
      const double measured = std::log(amp(s10) / amp(s5)) / 5.0;
      expect(rel(measured, best) < 5e-3, "PDE growth rate " + std::to_string(measured) + " vs dispersion " + std::to_string(best));
    }
    // ---- morphogen gradient: D u'' - k u = 0, u(0) = 1, du/dx(L) = 0  =>  cosh((L - x)/lambda) / cosh(L/lambda)
    {
      const double d = 0.1, k = 1.0, length = 2.0, lam = std::sqrt(d / k);
      sp::Grid g = sp::box({100}, {length}, sp::Boundary::Neumann);
      g.axes[0].lower = sp::Boundary::Dirichlet;
      g.axes[0].lower_value = 1.0;
      const sp::Reaction f = [&](const Series &u) { return u.mul(-k); };
      const Series out = sp::integrate(vec(std::vector<double>(100, 0.0)).unsqueeze(0), vec({d}), g, f, 0.0, 20.0, 14000);
      const auto ov = out.contiguous().host<double>();
      double worst = 0.0;
      for (std::size_t i = 0; i < ov.size(); ++i) {
        const double x = (static_cast<double>(i) + 0.5) * g.axes[0].spacing();
        worst = std::max(worst, std::abs(ov[i] - std::cosh((length - x) / lam) / std::cosh(length / lam)));
      }
      expect(worst < 5e-4, "morphogen steady state vs cosh solution, worst " + std::to_string(worst));
    }
    // ---- Fisher-KPP: front speed 2 sqrt(r D) approached from below as c - 3 / (2 lambda t) (Bramson 1983 logarithmic shift -3/2 log t for the normalised equation, as quoted in arXiv:1510.03329)
    {
      const double r = 1.0, d = 1.0, length = 120.0;
      const std::int64_t n = 600;
      const sp::Grid g = sp::box({n}, {length}, sp::Boundary::Neumann);
      const double h = g.axes[0].spacing();
      std::vector<double> u0(static_cast<std::size_t>(n));
      for (std::int64_t i = 0; i < n; ++i)
        u0[static_cast<std::size_t>(i)] = (static_cast<double>(i) + 0.5) * h < 10.0 ? 1.0 : 0.0;
      const sp::Reaction f = [&](const Series &u) { return u.mul(u.mul(-1.0).add(1.0)).mul(r); };
      const auto front = [&](const Series &s) {
        const auto v = s.contiguous().host<double>();
        for (std::size_t i = 1; i < v.size(); ++i)
          if (v[i] < 0.5 && v[i - 1] >= 0.5)
            return (static_cast<double>(i) - 0.5 + (v[i - 1] - 0.5) / (v[i - 1] - v[i])) * h;
        return -1.0;
      };
      const Series s30 = sp::integrate(vec(u0).unsqueeze(0), vec({d}), g, f, 0.0, 30.0, 3000);
      const Series s40 = sp::integrate(s30, vec({d}), g, f, 30.0, 40.0, 1000);
      const double speed = (front(s40) - front(s30)) / 10.0;
      const double lambda = std::sqrt(r / d), expected = 2.0 * std::sqrt(r * d) - 1.5 / lambda * std::log(40.0 / 30.0) / 10.0;
      expect(rel(speed, expected) < 0.01, "Fisher-KPP front speed " + std::to_string(speed) + " vs " + std::to_string(expected));
    }
    // ---- 1-D Gray-Scott vs MATLAB pdepe
    {
      namespace pr = spatial_pdepe_reference;
      const std::int64_t n = 250;
      const double length = 2.5;
      const sp::Grid g = sp::box({n}, {length}, sp::Boundary::Neumann);
      const double h = g.axes[0].spacing();
      const tu::GrayScott gs{0.035, 0.065};
      std::vector<double> init(static_cast<std::size_t>(2 * n));
      for (std::int64_t i = 0; i < n; ++i) {
        const double x = (static_cast<double>(i) + 0.5) * h, bump = std::exp(-std::pow((x - 1.25) / 0.1, 2.0));
        init[static_cast<std::size_t>(i)] = 1.0 - 0.5 * bump;
        init[static_cast<std::size_t>(n + i)] = 0.25 * bump;
      }
      Series state = pluss::make_series(init, {2, n}).to(F64);
      const sp::Reaction f = [&](const Series &u) { return gs.reaction(u, 1); };
      const Series diff = vec({0.16, 0.08});
      const std::vector<const std::vector<double> *> us{&pr::u_t2, &pr::u_t3, &pr::u_t4}, vs{&pr::v_t2, &pr::v_t3, &pr::v_t4};
      double worst = 0.0;
      for (std::size_t q = 0; q < 3; ++q) {
        const double t0 = pr::times[q], t1 = pr::times[q + 1];
        state = sp::integrate(state, diff, g, f, t0, t1, static_cast<std::int64_t>((t1 - t0) / 3e-4) + 1);
        const auto sv = state.contiguous().host<double>();
        for (std::size_t p = 0; p < pr::probes.size(); ++p) {
          const double pos = std::clamp(pr::probes[p] / h - 0.5, 0.0, static_cast<double>(n - 1) - 1e-9);
          const std::size_t i0 = static_cast<std::size_t>(pos);
          const double w = pos - static_cast<double>(i0);
          const double u = sv[i0] * (1.0 - w) + sv[i0 + 1] * w, v = sv[static_cast<std::size_t>(n) + i0] * (1.0 - w) + sv[static_cast<std::size_t>(n) + i0 + 1] * w;
          worst = std::max({worst, std::abs(u - (*us[q])[p]), std::abs(v - (*vs[q])[p])});
        }
      }
      expect(worst < 3e-3, "Gray-Scott 1-D vs pdepe, worst probe difference " + std::to_string(worst));
    }
    std::cout << "spatial tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
