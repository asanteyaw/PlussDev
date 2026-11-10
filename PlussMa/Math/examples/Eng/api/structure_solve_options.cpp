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
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  // a cantilever ladder truss of 40 bays: long, so the stiffness solve needs a couple of hundred iterations
  const int bays = 40;
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int level = 0; level < 2; ++level)
    for (int i = 0; i <= bays; ++i) {
      position.push_back(i);
      position.push_back(level);
    }
  const auto top = [&](int i) { return bays + 1 + i; };
  for (int i = 0; i < bays; ++i) {
    from.push_back(i); to.push_back(i + 1);
    from.push_back(top(i)); to.push_back(top(i + 1));
    from.push_back(i); to.push_back(top(i + 1));
  }
  for (int i = 0; i <= bays; ++i) { from.push_back(i); to.push_back(top(i)); }
  const auto bars = static_cast<std::int64_t>(from.size());
  const auto ladder = structure::truss({grid(position, 2), from, to}, {vec(std::vector<double>(bars, 1e-3)), vec(std::vector<double>(bars, 200e9))});
  std::vector<double> load(4 * (bays + 1), 0.0);
  load[2 * bays + 1] = -1000.0;
  const structure::Fix wall{{0, 0, top(0), top(0)}, {0, 1, 0, 1}, {}};
  const auto tip = [&](const structure::StaticResult &r) { return r.displacement.select(-2, bays).select(-1, 1).item<double>(); };

  structure::SolveOptions tightest;
  tightest.cg.relative_tolerance = 1e-14;
  tightest.cg.absolute_tolerance = 1e-30;
  const double reference = tip(structure::solve(ladder, grid(load, 2), wall, tightest));
  out.writeln("tip deflection (m), tightest solve: ", reference);

  for (double tolerance : {1e-10, 1e-3}) {
    structure::SolveOptions options;
    options.cg.relative_tolerance = tolerance;
    options.cg.absolute_tolerance = 1e-30;
    const auto r = structure::solve(ladder, grid(load, 2), wall, options);
    out.writeln("cg.relative_tolerance ", tolerance, ": iterations ", r.iterations, ", residual ", r.residual_norm.item<double>(), ", tip error ", std::abs(tip(r) - reference));
  }

  // cg.max_iterations and fail_on_nonconvergence: both states
  structure::SolveOptions short_budget;
  short_budget.cg.max_iterations = 20;
  short_budget.fail_on_nonconvergence = false;
  const auto cut = structure::solve(ladder, grid(load, 2), wall, short_budget);
  out.writeln("budget of 20 iterations, failure off: converged ", cut.converged.item<bool>(), ", residual ", cut.residual_norm.item<double>());
  short_budget.fail_on_nonconvergence = true;
  try {
    (void)structure::solve(ladder, grid(load, 2), wall, short_budget);
  } catch (const std::exception &e) {
    std::cout << "budget of 20 iterations, failure on: " << e.what() << "\n";
  }
}
