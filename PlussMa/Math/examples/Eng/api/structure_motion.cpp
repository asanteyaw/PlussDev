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
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;
  const int steps = 200;
  const double dt = 0.001;
  std::vector<double> times, history;
  for (int i = 0; i <= steps; ++i) {
    times.push_back(dt * i);
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = -100.0;   // a suddenly applied 100 N at the tip
    history.insert(history.end(), load.begin(), load.end());
  }
  const auto load = pluss::series::make(history).reshape({steps + 1, members + 1, 3});

  const auto r = structure::motion(frame, load, vec(times), wall);
  const auto u = flat(r.displacement);
  double peak = 0.0;
  for (int i = 0; i <= steps; ++i) peak = std::min(peak, u[i * 3 * (members + 1) + tip]);
  out.writeln("sudden load, no damping: peak tip deflection (mm) ", 1000.0 * peak, " (static ", -1000.0 * 100.0 * 8.0 / (3 * 200e9 * 8.333e-8), "), every step converged ", r.converged.all().item<bool>());
  out.writeln("shapes: displacement ", r.displacement.sizes(), ", kinetic ", r.kinetic.sizes(), ", iterations per step ", r.iterations.sizes(), ", reaction at the wall at the last step ", flat(r.reaction.select(-3, steps).select(-2, 0))[1]);

  // initial state: released from the first mode shape with Rayleigh damping set for 2 percent at the first mode
  const auto modes = structure::modes(frame, wall);
  const double omega = flat(modes.omega)[0];
  structure::MotionState start;
  start.displacement = modes.shape.select(0, 0).mul(0.01);
  structure::MotionOptions options;
  options.stiffness_damping = 2.0 * 0.02 / omega;
  const auto none = pluss::series::zeros({steps + 1, members + 1, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto d = structure::motion(frame, none, vec(times), wall, start, options);
  const auto kinetic = flat(d.kinetic), strain = flat(d.strain_energy);
  out.writeln("damped free vibration: energy at the start and at 0.2 s (J) ", kinetic[0] + strain[0], " ", kinetic[steps] + strain[steps]);

  // mass damping and numerical damping
  structure::MotionOptions mass;
  mass.mass_damping = 2.0 * 0.02 * omega;
  const auto m = structure::motion(frame, none, vec(times), wall, start, mass);
  out.writeln("mass damping, same ratio at mode 1: energy at 0.2 s (J) ", flat(m.kinetic)[steps] + flat(m.strain_energy)[steps]);
  structure::MotionOptions numerical;
  numerical.spectral_radius = 0.5;
  const auto n = structure::motion(frame, load, vec(times), wall, {}, numerical);
  const auto damped = flat(n.displacement);
  double damped_peak = 0.0;
  for (int i = 0; i <= steps; ++i) damped_peak = std::min(damped_peak, damped[i * 3 * (members + 1) + tip]);
  out.writeln("spectral_radius 0.5: peak tip deflection (mm) ", 1000.0 * damped_peak);
}
