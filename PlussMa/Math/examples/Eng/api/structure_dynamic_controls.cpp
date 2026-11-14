#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <algorithm>
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
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;

  // EigOptions: the absolute tolerance on its own
  structure::EigOptions defaults;
  defaults.count = 3;
  structure::EigOptions absolute = defaults;
  absolute.relative_tolerance = 0.0;
  absolute.absolute_tolerance = 1e-3;
  for (const auto &run : {std::make_pair("defaults                ", defaults), std::make_pair("absolute tolerance 1e-3", absolute)}) {
    const auto r = structure::modes(frame, wall, run.second);
    out.writeln(run.first, ": iterations ", r.iterations, ", residuals ", flat(r.residual), ", converged ", flat(r.converged));
  }

  // MotionOptions: a starved inner solve, with the failure switch in both states
  const int steps = 20;
  std::vector<double> times, history;
  for (int i = 0; i <= steps; ++i) {
    times.push_back(0.001 * i);
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = -100.0;
    history.insert(history.end(), load.begin(), load.end());
  }
  const auto load = pluss::series::make(history).reshape({steps + 1, members + 1, 3});
  structure::MotionOptions starved;
  starved.solve.max_iterations = 2;
  starved.fail_on_nonconvergence = false;
  const auto lenient = structure::motion(frame, load, vec(times), wall, {}, starved);
  const auto flags = flat(lenient.converged);
  const auto converged_steps = std::count_if(flags.begin(), flags.end(), [](double c) { return c > 0.5; });
  out.writeln("inner solve limited to 2 iterations, failure off: converged steps ", converged_steps, " of ", steps + 1);
  starved.fail_on_nonconvergence = true;
  try {
    (void)structure::motion(frame, load, vec(times), wall, {}, starved);
  } catch (const std::exception &e) {
    std::cout << "inner solve limited to 2 iterations, failure on: " << e.what() << "\n";
  }

  // HarmonicOptions.solve: the GMRES controls
  std::vector<double> loads;
  for (int f = 0; f < 3; ++f) {
    std::vector<double> l(3 * (members + 1), 0.0);
    l[tip] = 10.0;
    loads.insert(loads.end(), l.begin(), l.end());
  }
  const auto harmonic_load = pluss::series::make(loads).reshape({3, members + 1, 3});
  structure::HarmonicOptions defaults_h;
  defaults_h.stiffness_damping = 1e-4;
  structure::HarmonicOptions small_restart = defaults_h;
  small_restart.solve.restart = 4;
  small_restart.fail_on_nonconvergence = false;
  structure::HarmonicOptions loose = defaults_h;
  loose.solve.relative_tolerance = 1e-2;
  structure::HarmonicOptions capped = defaults_h;
  capped.solve.max_iterations = 3;
  capped.fail_on_nonconvergence = false;
  for (const auto &run : {std::make_pair("defaults         ", defaults_h), std::make_pair("restart 4        ", small_restart), std::make_pair("tolerance 1e-2   ", loose), std::make_pair("max_iterations 3 ", capped)}) {
    const auto r = structure::harmonic(frame, harmonic_load, vec({10.0, 40.5, 100.0}), wall, run.second);
    out.writeln(run.first, ": iterations ", r.iterations, ", operator evaluations ", r.operator_evaluations, ", restarts ", r.restarts, ", converged ", r.converged.all().item<bool>(), ", resonance amplitude (mm) ", 1000.0 * flat(r.displacement.select(-2, members).select(-1, 1).abs())[1]);
  }
}
