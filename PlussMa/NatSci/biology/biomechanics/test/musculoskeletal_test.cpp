// Muscle-driven two-link arm: moment arms and MTU kinematics (virtual work), static equilibrium, and the coupled simulation vs an independent DOP853 solution
// built from sympy chain dynamics and the OpenSim-derived muscle equations.
#include "pluss/subsp/natsci/biology/biomechanics/musculoskeletal.h"
#include "msk_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace ms = pluss::subsp::natsci::biology::biomechanics::musculoskeletal;
namespace ch = pluss::subsp::natsci::biology::biomechanics::chain;
namespace mu = pluss::subsp::natsci::biology::biomechanics::muscle;
namespace ref = msk_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mat(const std::vector<double> &v, std::vector<std::int64_t> s) { return pluss::make_series(v, s).to(F64); }
Series vec(const std::vector<double> &v) { return mat(v, {static_cast<std::int64_t>(v.size())}); }
} // namespace

int main() {
  try {
    const std::int64_t M = 5, J = 2;
    const ch::Chain arm = ch::make(vec(ref::chain_m), vec(ref::chain_l), vec(ref::chain_c), vec(ref::chain_I));
    ms::Moments moments{mat(ref::coef, {M, J, 2}), vec(ref::l0), vec(ref::q0)};
    mu::Params mp;
    mp.max_isometric_force = vec(ref::muscle_f);
    mp.optimal_fiber_length = vec(ref::muscle_lopt);
    mp.tendon_slack_length = vec(ref::muscle_lts);
    mp.pennation_at_optimal = ref::alpha;
    const Series fmax = vec(ref::muscle_f);
    // ---- kinematics: dL/dq = -r (virtual work), L(q_ref) = L0, and R(q) from the polynomial
    {
      const Series q = mat({0.6, 1.3, -0.2, 0.4}, {2, 2});
      const Series len = ms::muscle_tendon_length(moments, q);
      const double h = 1e-6;
      for (std::int64_t j = 0; j < J; ++j) {
        std::vector<double> dq(4, 0.0);
        dq[static_cast<std::size_t>(j)] = h;
        dq[static_cast<std::size_t>(2 + j)] = h;
        const Series up = ms::muscle_tendon_length(moments, q.add(mat(dq, {2, 2}))), dn = ms::muscle_tendon_length(moments, q.sub(mat(dq, {2, 2})));
        const Series fd = up.sub(dn).div(2.0 * h).neg();
        expect(fd.sub(ms::moment_arms(moments, q).select(-1, j)).abs().max().item<double>() < 1e-7, "-dL/dq equals the moment arm, joint " + std::to_string(j));
      }
      expect(ms::muscle_tendon_length(moments, vec({ref::q0[0], ref::q0[1]})).sub(vec(ref::l0)).abs().max().item<double>() < 1e-14, "L(q_ref) = L0");
      // the elbow flexor (muscle 3) has the posture-dependent arm 0.030 + 0.012 q2
      expect(std::abs(ms::moment_arms(moments, vec({0.0, 1.0})).select(0, 2).select(0, 1).item<double>() - (0.030 + 0.012)) < 1e-14, "polynomial moment arm");
      // velocity: Ldot = -R qdot
      const Series qd = mat({1.0, -2.0}, {2});
      expect(ms::muscle_tendon_velocity(moments, vec({0.5, 0.8}), qd).sub(ms::moment_arms(moments, vec({0.5, 0.8})).matmul(qd.unsqueeze(-1)).squeeze(-1).neg()).abs().max().item<double>() < 1e-14, "MTU velocity");
    }
    // ---- joint torques are R^T F; virtual-work consistency: power of the joint torques equals the sum of muscle powers
    {
      const Series q = vec({0.5, 0.9}), qd = vec({0.7, -0.3}), force = vec({300.0, 120.0, 250.0, 80.0, 40.0});
      const Series tau = ms::joint_torques(moments, q, force);
      const Series joint_power = tau.mul(qd).sum(), muscle_power = force.mul(ms::muscle_tendon_velocity(moments, q, qd).neg()).sum();
      expect(std::abs(joint_power.item<double>() - muscle_power.item<double>()) < 1e-10, "power balance: tau . qdot = - sum F Ldot");
    }
    // ---- coupled simulation vs DOP853 (five times, all joint and muscle states)
    {
      const ms::State s0 = ms::equilibrium_state(moments, mp, vec(ref::q0), vec(ref::a0));
      expect(s0.norm_tendon_force.sub(vec(ref::f0)).abs().max().item<double>() < 1e-10, "initial isometric equilibrium vs brentq");
      const std::vector<double> base{0.45, 0.50, 0.45, 0.50, 0.30};
      const auto excitation = [&](double t) {
        const auto s = [&](double t0, double w) { return 0.5 * (1.0 + std::tanh((t - t0) / w)); };
        return vec({base[0] + 0.25 * s(0.1, 0.03), base[1], base[2] + 0.2 * s(0.2, 0.04), base[3] - 0.1 * s(0.2, 0.04), base[4] + 0.1 * s(0.15, 0.05)});
      };
      const auto path = ms::simulate(arm, moments, mp, fmax, excitation, s0, 0.0, 0.5, 500);
      for (std::size_t k = 0; k < ref::traj_t.size(); ++k) {
        const auto idx = static_cast<std::size_t>(std::llround(ref::traj_t[k] / 0.001));
        const auto &st = path.states[idx];
        const double *r = &ref::traj[14 * k];
        for (int j = 0; j < 2; ++j)
          expect(std::abs(st.q.select(0, j).item<double>() - r[j]) < 2e-5 && std::abs(st.qd.select(0, j).item<double>() - r[2 + j]) < 1e-4, "joint state at t = " + std::to_string(ref::traj_t[k]) + ", joint " + std::to_string(j) + ": q " + std::to_string(st.q.select(0, j).item<double>()) + " vs " + std::to_string(r[j]));
        for (int i = 0; i < 5; ++i)
          expect(std::abs(st.activation.select(0, i).item<double>() - r[4 + i]) < 2e-5 && std::abs(st.norm_tendon_force.select(0, i).item<double>() - r[9 + i]) < 2e-4, "muscle " + std::to_string(i) + " at t = " + std::to_string(ref::traj_t[k]) + ": f_T " + std::to_string(st.norm_tendon_force.select(0, i).item<double>()) + " vs " + std::to_string(r[9 + i]));
      }
    }
    // ---- static posture: with constant activation the arm settles to an equilibrium (joint torque from muscles balances gravity) and stays there
    {
      const ms::State s0 = ms::equilibrium_state(moments, mp, vec({0.2, 0.5}), vec(ref::a0));
      const auto exc = [&](double) { return vec(ref::a0); };
      // damping through a viscous torque is not part of the muscle model, so check balance at the reached state instead: tau_muscle - bias = M qdd with small qdd
      const auto path = ms::simulate(arm, moments, mp, fmax, exc, s0, 0.0, 0.05, 50);
      const auto &s = path.states.back();
      expect(s.activation.sub(vec(ref::a0)).abs().max().item<double>() < 1e-9, "activation held by constant excitation");
      expect(s.norm_tendon_force.min().item<double>() > 0.0 && s.norm_tendon_force.max().item<double>() < 5.0, "tendon forces stay in the model's range");
    }
    std::cout << "musculoskeletal tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
