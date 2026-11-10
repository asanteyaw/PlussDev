#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::ma::eng::structure;
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
  // a 1 m steel rod hanging from a pin at its top node, released at rest at 1 rad
  const int members = 8;
  const double gravity = 9.81, theta0 = 1.0;
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(0.0);
    position.push_back(-1.0 * i / members);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  const auto rod = structure::frame({grid(position, 2), from, to}, {vec(std::vector<double>(members, 4e-4)), vec(std::vector<double>(members, 1.333e-8)), vec(std::vector<double>(members, 200e9)), vec(std::vector<double>(members, 7850.0))});
  const structure::Fix pin{{0, 0}, {0, 1}, {}};
  std::vector<double> down(3 * (members + 1), 0.0);
  for (int i = 0; i <= members; ++i) down[3 * i + 1] = -gravity;
  const auto weight = flat(structure::mass(rod, grid(down, 3)));

  const int steps = 120;
  const double dt = 0.005;
  std::vector<double> times, history;
  for (int i = 0; i <= steps; ++i) {
    times.push_back(dt * i);
    history.insert(history.end(), weight.begin(), weight.end());
  }
  std::vector<double> start(3 * (members + 1), 0.0);
  for (int i = 0; i <= members; ++i) {
    const double x = 1.0 * i / members;
    start[3 * i] = x * std::sin(theta0);
    start[3 * i + 1] = x * (1.0 - std::cos(theta0));
    start[3 * i + 2] = theta0;
  }
  structure::MotionState initial;
  initial.displacement = grid(start, 3);

  structure::NonlinearMotionOptions options;   // the default spectral radius is 0.9
  const auto r = structure::motion(rod, pluss::series::make(history).reshape({steps + 1, members + 1, 3}), vec(times), pin, initial, options);
  out.writeln("default spectral radius ", options.spectral_radius, ": every step converged ", r.converged.all().item<bool>(), ", at most ", r.iterations.max().item<double>(), " Newton iterations per step");
  out.writeln("tip rotation at 0, 0.3 and 0.6 s (rad): ", flat(r.displacement.select(-2, members).select(-1, 2))[0], " ", flat(r.displacement.select(-2, members).select(-1, 2))[60], " ", flat(r.displacement.select(-2, members).select(-1, 2))[120]);
  out.writeln("shapes: displacement ", r.displacement.sizes(), ", kinetic ", r.kinetic.sizes());

  // spectral_radius 0.5 damps the high modes more
  structure::NonlinearMotionOptions damped;
  damped.spectral_radius = 0.5;
  const auto d = structure::motion(rod, pluss::series::make(history).reshape({steps + 1, members + 1, 3}), vec(times), pin, initial, damped);
  out.writeln("spectral_radius 0.5: tip rotation at 0.6 s (rad) ", flat(d.displacement.select(-2, members).select(-1, 2))[120]);

  // with mass and stiffness damping as well
  structure::NonlinearMotionOptions rayleigh;
  rayleigh.stiffness_damping = 1e-4;
  const auto q = structure::motion(rod, pluss::series::make(history).reshape({steps + 1, members + 1, 3}), vec(times), pin, initial, rayleigh);
  out.writeln("stiffness_damping 1e-4: tip rotation at 0.6 s (rad) ", flat(q.displacement.select(-2, members).select(-1, 2))[120]);
}
