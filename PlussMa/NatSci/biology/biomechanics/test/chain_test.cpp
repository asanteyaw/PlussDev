// Planar serial chain dynamics (recursive Newton-Euler) against a sympy Lagrangian derivation and an independent DOP853 trajectory.
#include "pluss/subsp/natsci/biology/biomechanics/chain.h"
#include "chain_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace ch = pluss::subsp::natsci::biology::biomechanics::chain;
namespace ref = chain_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mat(const std::vector<double> &v, std::vector<std::int64_t> s) { return pluss::make_series(v, s).to(F64); }
Series vec(const std::vector<double> &v) { return mat(v, {static_cast<std::int64_t>(v.size())}); }
double max_abs(const Series &a, const std::vector<double> &b) {
  const auto av = a.contiguous().host<double>();
  expect(av.size() == b.size(), "size mismatch " + std::to_string(av.size()) + " vs " + std::to_string(b.size()));
  double w = 0.0;
  for (std::size_t i = 0; i < av.size(); ++i)
    w = std::max(w, std::abs(av[i] - b[i]));
  return w;
}
} // namespace

int main() {
  try {
    const ch::Chain c = ch::make(vec(ref::m), vec(ref::l), vec(ref::c), vec(ref::I));
    const std::size_t cases = ref::cases.size() / 11;
    std::vector<double> q, qd, qdd, f;
    for (std::size_t i = 0; i < cases; ++i) {
      for (int k = 0; k < 3; ++k) { q.push_back(ref::cases[11 * i + k]); qd.push_back(ref::cases[11 * i + 3 + k]); qdd.push_back(ref::cases[11 * i + 6 + k]); }
      f.push_back(ref::cases[11 * i + 9]);
      f.push_back(ref::cases[11 * i + 10]);
    }
    const Series Q = mat(q, {static_cast<std::int64_t>(cases), 3}), QD = mat(qd, {static_cast<std::int64_t>(cases), 3}), QDD = mat(qdd, {static_cast<std::int64_t>(cases), 3}), F = mat(f, {static_cast<std::int64_t>(cases), 2});
    // all eight states in one batched call
    expect(max_abs(ch::inverse_dynamics(c, Q, QD, QDD, F), ref::tau) < 1e-9, "inverse dynamics with an end-point force vs the sympy Lagrangian: " + std::to_string(max_abs(ch::inverse_dynamics(c, Q, QD, QDD, F), ref::tau)));
    expect(max_abs(ch::mass_matrix(c, Q), ref::mass_matrix) < 1e-10, "mass matrix");
    {
      const Series m = ch::mass_matrix(c, Q);
      expect(m.sub(m.transpose(-1, -2)).abs().max().item<double>() < 1e-12, "mass matrix is symmetric");
    }
    expect(max_abs(ch::bias_forces(c, Q, QD), ref::bias) < 1e-9, "Coriolis + centrifugal + gravity vs sympy");
    {
      const Series tau = ch::inverse_dynamics(c, Q, QD, QDD, F);
      expect(ch::forward_dynamics(c, Q, QD, tau, F).sub(QDD).abs().max().item<double>() < 1e-9, "forward dynamics inverts inverse dynamics");
    }
    {
      const auto e = ch::energy(c, Q, QD);
      std::vector<double> t, v;
      for (std::size_t i = 0; i < cases; ++i) { t.push_back(ref::energy[2 * i]); v.push_back(ref::energy[2 * i + 1]); }
      expect(max_abs(e.kinetic, t) < 1e-10 && max_abs(e.potential, v) < 1e-10, "kinetic and potential energy vs sympy");
    }
    // static equilibrium of a hanging chain: zero torque at q = (-pi/2, 0, 0) with the weight on the base only
    {
      const Series qs = vec({-M_PI / 2.0, 0.0, 0.0}), z = vec({0.0, 0.0, 0.0});
      const auto tau = ch::inverse_dynamics(c, qs, z, z).contiguous().host<double>();
      for (double x : tau)
        expect(std::abs(x) < 1e-12, "a vertically hanging chain needs no joint torque");
      // a horizontal chain: the shoulder torque balances the total moment of the weights about the base
      const Series horiz = vec({0.0, 0.0, 0.0});
      const auto t2 = ch::inverse_dynamics(c, horiz, z, z).contiguous().host<double>();
      double moment = 0.0, cum = 0.0;
      for (int i = 0; i < 3; ++i) { moment += ref::m[static_cast<std::size_t>(i)] * 9.80665 * (cum + ref::c[static_cast<std::size_t>(i)]); cum += ref::l[static_cast<std::size_t>(i)]; }
      expect(std::abs(t2[0] - moment) < 1e-10, "shoulder torque of the horizontal chain = sum m g x_com");
    }
    // trajectory under PD feedback vs DOP853 (RK4 with dt = 0.5 ms; the distal link has I = 0.004 kg m^2, so the PD loop is fast (~80 rad/s))
    {
      const Series qref = vec(ref::qref);
      const auto torque = [&](double, const Series &qq, const Series &dq) { return qq.sub(qref).mul(-ref::kp).sub(dq.mul(ref::kd)); };
      const auto path = ch::simulate(c, torque, vec({ref::y0[0], ref::y0[1], ref::y0[2]}), vec({0.0, 0.0, 0.0}), 0.0, 2.0, 4000);
      for (std::size_t k = 0; k < ref::traj_t.size(); ++k) {
        const auto idx = static_cast<std::size_t>(std::llround(ref::traj_t[k] / 0.0005));
        for (int j = 0; j < 3; ++j) {
          expect(std::abs(path.q[idx].select(0, j).item<double>() - ref::traj[6 * k + static_cast<std::size_t>(j)]) < 2e-6 && std::abs(path.qd[idx].select(0, j).item<double>() - ref::traj[6 * k + 3 + static_cast<std::size_t>(j)]) < 5e-6,
                 "PD-controlled trajectory at t = " + std::to_string(ref::traj_t[k]) + " joint " + std::to_string(j) + ": q " + std::to_string(path.q[idx].select(0, j).item<double>()) + " vs " + std::to_string(ref::traj[6 * k + static_cast<std::size_t>(j)]) + ", qd " + std::to_string(path.qd[idx].select(0, j).item<double>()) + " vs " + std::to_string(ref::traj[6 * k + 3 + static_cast<std::size_t>(j)]));
        }
      }
    }
    // free motion conserves energy: no torque, no external force
    {
      const auto path = ch::simulate(c, [](double, const Series &qq, const Series &) { return qq.mul(0.0); }, vec({0.7, -0.4, 0.3}), vec({0.5, -1.0, 2.0}), 0.0, 3.0, 1500);
      const double e0 = ch::energy(c, path.q.front(), path.qd.front()).total().item<double>(), e1 = ch::energy(c, path.q.back(), path.qd.back()).total().item<double>();
      expect(std::abs(e1 - e0) < 2e-5 * (1.0 + std::abs(e0)), "free swing conserves energy: " + std::to_string(e0) + " -> " + std::to_string(e1));
    }
    // lanes: a batch of initial conditions equals the individual runs
    {
      const auto torque = [](double, const Series &qq, const Series &dq) { return qq.mul(-5.0).sub(dq.mul(1.0)); };
      const Series q0 = mat({0.3, 0.1, -0.2, 0.9, -0.4, 0.5}, {2, 3}), v0 = mat({0, 0, 0, 0.5, 0.5, 0.5}, {2, 3});
      const auto batch = ch::simulate(c, torque, q0, v0, 0.0, 0.5, 100);
      const auto one = ch::simulate(c, torque, q0.select(0, 1), v0.select(0, 1), 0.0, 0.5, 100);
      expect(batch.q.back().select(0, 1).sub(one.q.back()).abs().max().item<double>() < 1e-12, "lane-batched simulation equals the single run");
    }
    std::cout << "chain tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
