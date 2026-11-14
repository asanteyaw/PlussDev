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


// the hung three-bar truss: supports at the top, the loaded joint at (0, 0)
static structure::Truss hung() {
  return structure::truss({grid({-3, 4, 0, 4, 3, 4, 0, 0}, 2), {0, 1, 2}, {3, 3, 3}}, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const auto truss = hung();
  const structure::Fix supports{{0, 0, 1, 1, 2, 2}, {0, 1, 0, 1, 0, 1}, {}};
  const auto load = grid({0, 0, 0, 0, 0, 0, 0, -10000}, 2);
  const auto run = [&](const char *label, const structure::SolveOptions &o) {
    const auto r = structure::solve(truss, load, supports, o);
    out.writeln(label, ": iterations ", r.iterations, ", residual ", r.residual_norm.item<double>(), ", converged ", r.converged.item<bool>(), ", breakdown ", r.breakdown.item<bool>());
  };
  structure::SolveOptions defaults;
  run("defaults                       ", defaults);
  structure::SolveOptions absolute = defaults;
  absolute.cg.relative_tolerance = 0.0;
  absolute.cg.absolute_tolerance = 1e-3;
  run("absolute tolerance 1e-3 only   ", absolute);
  structure::SolveOptions every = defaults;
  every.cg.check_interval = 1;
  run("check_interval 1               ", every);
  structure::SolveOptions curvature = defaults;
  curvature.cg.curvature_tolerance = 1e30;
  curvature.fail_on_nonconvergence = false;
  run("curvature_tolerance 1e30       ", curvature);

  // large-displacement controls on a cantilever bent to alpha = 1.2
  const int members = 16;
  const auto frame = beam(members, false);
  std::vector<double> nodal(3 * (members + 1), 0.0);
  nodal[3 * members + 1] = -4999.8;
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const auto bend = [&](const char *label, const structure::NonlinearOptions &o) {
    const auto r = structure::solve(frame, grid(nodal, 3), wall, o);
    out.writeln(label, ": converged ", r.converged.item<bool>(), ", load factor ", r.load_factor, ", iterations ", r.iterations, ", cutbacks ", r.cutbacks);
  };
  structure::NonlinearOptions base;
  base.load_steps = 20;
  base.fail_on_nonconvergence = false;
  bend("defaults                      ", base);
  structure::NonlinearOptions one_search = base;
  one_search.max_line_search = 1;
  bend("max_line_search 1             ", one_search);
  structure::NonlinearOptions gentle = base;
  gentle.line_search_shrink = 0.9;
  bend("line_search_shrink 0.9        ", gentle);
  structure::NonlinearOptions strict = base;
  strict.armijo = 0.5;
  bend("armijo 0.5                    ", strict);
  structure::NonlinearOptions stalled = base;
  stalled.max_iterations = 1;
  stalled.max_cutbacks = 1000;
  stalled.min_load_step = 1e-6;
  bend("1 iteration, min step 1e-6    ", stalled);
  stalled.min_load_step = 0.1;
  bend("1 iteration, min step 0.1     ", stalled);
}
