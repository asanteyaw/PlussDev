// Hill-type muscle-tendon unit (De Groote-Fregly form) against an independent numpy/scipy re-implementation whose constants are parsed from OpenSim's
// source, the model's defining key points, exact inverse relations, and ODE steady states.
#include "pluss/subsp/natsci/biology/biomechanics/muscle.h"
#include "muscle_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace mu = pluss::subsp::natsci::biology::biomechanics::muscle;
namespace ref = muscle_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double max_abs(const Series &a, const std::vector<double> &b) {
  const auto av = a.contiguous().to_vec<double>();
  expect(av.size() == b.size(), "size mismatch");
  double w = 0.0;
  for (std::size_t i = 0; i < av.size(); ++i)
    w = std::max(w, std::abs(av[i] - b[i]));
  return w;
}
mu::Params params() {
  mu::Params p;
  const auto &q = ref::params;
  p.max_isometric_force = q[0]; p.optimal_fiber_length = q[1]; p.tendon_slack_length = q[2]; p.pennation_at_optimal = q[3];
  p.max_contraction_velocity = q[4]; p.activation_time = q[5]; p.deactivation_time = q[6]; p.tendon_strain_at_one = q[7];
  p.passive_strain_at_one = q[8]; p.fiber_damping = q[9]; p.width_scale = q[10]; p.activation_smoothing = q[11];
  return p;
}
} // namespace

int main() {
  try {
    // ---- curves vs the independent implementation (constants parsed from the OpenSim header)
    expect(max_abs(mu::active_force_length(vec(ref::lgrid)), ref::fal) < 1e-13, "active force-length");
    expect(max_abs(mu::passive_force_length(vec(ref::lgrid), 0.6), ref::fpe) < 1e-13, "passive force-length");
    expect(max_abs(mu::force_velocity(vec(ref::vgrid)), ref::fv) < 1e-13, "force-velocity");
    expect(max_abs(mu::tendon_force_length(vec(ref::tgrid), 0.049), ref::ft) < 1e-13, "tendon force-length");
    expect(max_abs(mu::tendon_stiffness(vec(ref::tgrid), 0.049), ref::dft) < 1e-12, "tendon stiffness");
    // ---- defining key points of the model
    expect(std::abs(mu::active_force_length(vec({1.0})).item<double>() - 1.0) < 1e-12, "f_act(1) = 1");
    expect(std::abs(mu::force_velocity(vec({0.0})).item<double>() - 1.0) < 1e-12 && std::abs(mu::force_velocity(vec({-1.0})).item<double>()) < 1e-12, "f_v(0) = 1, f_v(-1) = 0");
    expect(std::abs(mu::tendon_force_length(vec({1.049}), 0.049).item<double>() - 1.0) < 1e-12 && std::abs(mu::tendon_force_length(vec({1.0}), 0.049).item<double>()) < 1e-12, "tendon: unit force at strain 0.049, zero at slack length");
    expect(std::abs(mu::passive_force_length(vec({1.6}), 0.6).item<double>() - 1.0) < 1e-12 && std::abs(mu::passive_force_length(vec({0.2}), 0.6).item<double>()) < 1e-12, "passive: unit force at strain 0.6, zero at 0.2");
    expect(mu::force_velocity(vec({1.0})).item<double>() < 1.8 && mu::force_velocity(vec({1.0})).item<double>() > 1.7, "f_v(1) is the documented ~1.794 range maximum");
    // ---- exact inverse relations and monotonicity
    {
      const Series v = vec({-0.95, -0.5, 0.0, 0.4, 0.99}), f = vec({0.05, 0.3, 0.9, 1.0, 1.7, 4.0});
      expect(mu::force_velocity_inverse(mu::force_velocity(v)).sub(v).abs().max().item<double>() < 1e-12, "f_v inverse round trip");
      expect(mu::tendon_force_length(mu::tendon_length_from_force(f, 0.049), 0.049).sub(f).abs().max().item<double>() < 1e-12, "tendon inverse round trip");
      const Series v2 = vec({-1.0, -0.8, -0.4, 0.0, 0.5, 1.0});
      expect(mu::force_velocity(v2).slice(0, 1, 6, 1).sub(mu::force_velocity(v2).slice(0, 0, 5, 1)).min().item<double>() > 0.0, "force-velocity increases with velocity");
      // tendon stiffness is the derivative of the tendon curve
      const Series lt = vec({0.95, 1.0, 1.03}), h = vec({1e-6, 1e-6, 1e-6});
      expect(mu::tendon_force_length(lt.add(h)).sub(mu::tendon_force_length(lt.sub(h))).div(2e-6).sub(mu::tendon_stiffness(lt)).abs().max().item<double>() < 1e-6, "tendon stiffness = d f_T / d l_T");
      // widening the active curve keeps its peak at 1 and lowers it elsewhere
      const Series l = vec({0.7, 1.3});
      expect(mu::active_force_length(l, 2.0).sub(mu::active_force_length(l, 1.0)).min().item<double>() > 0.0 && std::abs(mu::active_force_length(vec({1.0}), 2.0).item<double>() - 1.0) < 1e-12, "width_scale > 1 widens the curve");
    }
    // ---- isometric equilibrium on a grid of activations and muscle-tendon lengths vs scipy brentq
    {
      const mu::Params p = params();
      const std::size_t n = ref::eq.size() / 5;
      std::vector<double> a, len;
      for (std::size_t i = 0; i < n; ++i) { a.push_back(ref::eq[5 * i]); len.push_back(ref::eq[5 * i + 1]); }
      const auto e = mu::isometric_equilibrium(vec(a), vec(len), p);
      const auto ft = e.norm_tendon_force.contiguous().to_vec<double>(), ln = e.norm_fiber_length.contiguous().to_vec<double>(), lt = e.tendon_length.contiguous().to_vec<double>();
      for (std::size_t i = 0; i < n; ++i) {
        expect(std::abs(ft[i] - ref::eq[5 * i + 2]) < 1e-10 && std::abs(ln[i] - ref::eq[5 * i + 3]) < 1e-10 && std::abs(lt[i] - ref::eq[5 * i + 4]) < 1e-10, "isometric equilibrium vs brentq at index " + std::to_string(i));
      }
      expect(e.residual.max().item<double>() < 1e-12, "equilibrium residual is at rounding level");
      // force transmitted to the tendon is F_max times the normalised tendon force
      expect(e.force.sub(vec(ft).mul(ref::params[0])).abs().max().item<double>() < 1e-9, "tendon force in newtons");
    }
    // ---- dynamics: RK4 on Series vs scipy DOP853, with a time-varying excitation and muscle-tendon length
    {
      const mu::Params p = params();
      const auto excitation = [](double t) { return vec({0.05 + 0.375 * (1.0 + std::tanh((t - 0.15) / 0.02)) - 0.25 * (1.0 + std::tanh((t - 0.40) / 0.03))}); };
      const auto length = [](double t) { return vec({0.31 + 0.012 * std::sin(2.0 * M_PI * 2.0 * t)}); };
      const auto velocity = [](double t) { return vec({0.012 * 2.0 * M_PI * 2.0 * std::cos(2.0 * M_PI * 2.0 * t)}); };
      const auto path = mu::simulate(p, excitation, length, velocity, vec({ref::a0f0[0]}), vec({ref::a0f0[1]}), 0.0, 0.6, 1200);
      for (std::size_t k = 0; k < 12; ++k) {
        const double t = ref::traj[3 * k];
        const auto idx = static_cast<std::size_t>(std::llround(t / 0.0005));
        expect(std::abs(path.times[idx] - t) < 1e-9, "sample time on the grid");
        expect(std::abs(path.activation[idx].item<double>() - ref::traj[3 * k + 1]) < 5e-5 && std::abs(path.norm_tendon_force[idx].item<double>() - ref::traj[3 * k + 2]) < 5e-4,
               "muscle state at t = " + std::to_string(t) + ": a " + std::to_string(path.activation[idx].item<double>()) + " vs " + std::to_string(ref::traj[3 * k + 1]) + ", f_T " + std::to_string(path.norm_tendon_force[idx].item<double>()) + " vs " + std::to_string(ref::traj[3 * k + 2]));
      }
    }
    // ---- steady state: constant excitation and length converge to the isometric equilibrium
    {
      const mu::Params p = params();
      const double e0 = 0.6, len0 = 0.33;
      const auto path = mu::simulate(p, [&](double) { return vec({e0}); }, [&](double) { return vec({len0}); }, [](double) { return vec({0.0}); }, vec({0.3}), vec({0.5}), 0.0, 1.5, 3000);
      const auto eq = mu::isometric_equilibrium(vec({e0}), vec({len0}), p);
      expect(std::abs(path.activation.back().item<double>() - e0) < 1e-6 && std::abs(path.norm_tendon_force.back().item<double>() - eq.norm_tendon_force.item<double>()) < 1e-5,
             "dynamic muscle converges to the isometric equilibrium: " + std::to_string(path.norm_tendon_force.back().item<double>()) + " vs " + std::to_string(eq.norm_tendon_force.item<double>()));
    }
    // ---- activation dynamics: rise faster than decay, fixed points at e = a, batched lanes
    {
      const mu::Params p = params();
      const Series a = vec({0.2, 0.2, 0.7}), e = vec({0.9, 0.2, 0.1});
      const auto r = mu::activation_rate(a, e, p).contiguous().to_vec<double>();
      expect(r[0] > 0.0 && std::abs(r[1]) < 1e-15 && r[2] < 0.0, "activation sign structure");
      // closed form at one point: a = 0.2, e = 0.9
      const double z = 0.5 + 1.5 * 0.2, f = 0.5 * std::tanh(0.1 * (0.9 - 0.2));
      const double expected = ((f + 0.5) / (0.015 * z) + (-f + 0.5) * z / 0.060) * (0.9 - 0.2);
      expect(std::abs(r[0] - expected) < 1e-12, "activation rate matches the closed form");
      expect(r[0] / (0.9 - 0.2) > std::abs(r[2] / (0.1 - 0.7)), "rise (tau_a = 15 ms) is faster than decay (tau_d = 60 ms) at comparable gaps");
    }
    std::cout << "muscle tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
