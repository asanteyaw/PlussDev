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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 16;
  const auto frame = beam(members, false);
  const double load = 4999.8;   // alpha = P L^2 / (E I) = 1.2 for the 2 m beam: the elastica gives a tip deflection of 0.3490 L
  std::vector<double> nodal(3 * (members + 1), 0.0);
  nodal[3 * members + 1] = -load;
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};

  structure::NonlinearOptions options;
  options.load_steps = 20;
  const auto r = structure::solve(frame, grid(nodal, 3), wall, options);
  out.writeln("tip down / L ", -flat(r.displacement)[3 * members + 1] / 2.0, " (elastica 0.3490), inward / L ", -flat(r.displacement)[3 * members] / 2.0, " (elastica 0.0764)");
  out.writeln("converged ", r.converged.item<bool>(), ", load factor ", r.load_factor, ", steps ", r.steps, ", cutbacks ", r.cutbacks, ", Newton iterations ", r.iterations);
  out.writeln("residual ", r.residual_norm.item<double>(), ", strain energy (J) ", r.energy.item<double>());
  out.writeln("end actions of the member at the wall, in its own turned axes (Ni Vi Mi): ", flat(r.end_force)[0], " ", flat(r.end_force)[1], " ", flat(r.end_force)[2]);

  // a tighter tolerance and a different number of steps
  structure::NonlinearOptions tight = options;
  tight.relative_tolerance = 1e-12;
  tight.load_steps = 5;
  const auto s = structure::solve(frame, grid(nodal, 3), wall, tight);
  out.writeln("tolerance 1e-12, 5 steps: tip down / L ", -flat(s.displacement)[3 * members + 1] / 2.0, ", iterations ", s.iterations, ", cutbacks ", s.cutbacks);

  // iterations capped at 1 per step with the failure switched off, then on
  structure::NonlinearOptions capped = options;
  capped.max_iterations = 1;
  capped.max_cutbacks = 2;
  capped.fail_on_nonconvergence = false;
  const auto t = structure::solve(frame, grid(nodal, 3), wall, capped);
  out.writeln("one iteration per step, failure off: load factor reached ", t.load_factor, ", converged ", t.converged.item<bool>());
  capped.fail_on_nonconvergence = true;
  try {
    (void)structure::solve(frame, grid(nodal, 3), wall, capped);
  } catch (const std::exception &e) {
    std::cout << "one iteration per step, failure on: " << e.what() << "\n";
  }
}
