#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::subsp::eng::structure;
using namespace pluss::tils;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a straight steel beam of length 2 m along x, with density, divided into equal members
static structure::Frame beam(int members, bool with_density = true) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(2.0 * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  structure::Sections sections{vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))};
  if (with_density) sections.density = vec(std::vector<double>(members, 7850.0));
  return structure::frame({grid(position, 2), from, to}, sections);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;
  const std::vector<double> frequencies = {10, 40.5, 100, 253.8};
  std::vector<double> loads;
  for (std::size_t f = 0; f < frequencies.size(); ++f) {
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = 10.0;
    loads.insert(loads.end(), load.begin(), load.end());
  }
  const auto load = pluss::series::make(loads).reshape({(std::int64_t)frequencies.size(), members + 1, 3});

  structure::HarmonicOptions stiffness;
  stiffness.stiffness_damping = 1e-4;   // zeta_n = b omega_n / 2, 0.002 at the first mode
  const auto r = structure::harmonic(frame, load, vec(frequencies), wall, stiffness);
  out.writeln("stiffness damping: tip amplitude (mm) ", flat(r.displacement.select(-2, members).select(-1, 1).abs().mul(1000.0)), ", converged ", r.converged.all().item<bool>());
  out.writeln("shapes: displacement ", r.displacement.sizes(), ", reaction ", r.reaction.sizes(), ", relative residual ", flat(r.relative_residual));
  out.writeln("iterations ", r.iterations, ", operator evaluations ", r.operator_evaluations, ", restarts ", r.restarts);

  // mass damping: zeta_n = a / (2 omega_n), the same 0.002 at the first mode, but much heavier damping at low frequency and none at high frequency
  const double omega1 = flat(structure::modes(frame, wall).omega)[0];
  structure::HarmonicOptions mass;
  mass.mass_damping = 2.0 * 0.002 * omega1;
  const auto m = structure::harmonic(frame, load, vec(frequencies), wall, mass);
  out.writeln("mass damping:      tip amplitude (mm) ", flat(m.displacement.select(-2, members).select(-1, 1).abs().mul(1000.0)));

  // velocity and acceleration
  out.writeln("|velocity| / (omega |u|) at the tip: ", flat(r.velocity.select(-2, members).select(-1, 1).abs().div(r.displacement.select(-2, members).select(-1, 1).abs()).div(vec(frequencies))));

  // undamped: the amplitude grows without bound as the frequency approaches the natural frequency
  const std::vector<std::pair<const char *, double>> offsets = {{"1e-2", 1e-2}, {"1e-3", 1e-3}};
  for (const auto &offset : offsets) {
    const auto u = structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1 * (1.0 + offset.second)}), wall);
    out.writeln("no damping, ", offset.first, " above omega_1: tip amplitude (mm) ", 1000.0 * flat(u.displacement.select(-2, members).select(-1, 1).abs())[0]);
  }

  // exactly at the natural frequency the dynamic stiffness is singular: the solve reports that it did not converge
  structure::HarmonicOptions undamped;
  undamped.fail_on_nonconvergence = false;
  const auto exact = structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1}), wall, undamped);
  out.writeln("undamped at omega_1, failure off: converged ", exact.converged.all().item<bool>());
  try {
    undamped.fail_on_nonconvergence = true;
    (void)structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1}), wall, undamped);
  } catch (const std::exception &e) {
    std::cout << "undamped at omega_1, failure on: " << e.what() << "\n";
  }
}
