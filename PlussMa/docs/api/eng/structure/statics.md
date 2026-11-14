<!-- generated from statics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Static analysis

This branch solves steady-load equilibrium. `solve` has four forms: a truss, a frame, a frame with uniform member loads, and a frame under large displacements.

## Choosing A Surface

Use the linear forms while displacements are small, and the nonlinear form when the load turns the structure through more than a few degrees.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Solve a truss and read every result.

```cpp
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
static const structure::Fix kSupports{{0, 0, 1, 1, 2, 2}, {0, 1, 0, 1, 0, 1}, {}};

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto truss = hung();
  const auto r = structure::solve(truss, grid({0, 0, 0, 0, 0, 0, 0, -10000}, 2), kSupports);
  out.writeln("displacement of the hung joint (mm): ", 1000.0 * flat(r.displacement)[6], " ", 1000.0 * flat(r.displacement)[7]);
  out.writeln("force (N):   ", flat(r.force));
  out.writeln("stress (MPa): ", flat(r.stress.mul(1e-6)));
  out.writeln("strain (1e-6): ", flat(r.strain.mul(1e6)));
  out.writeln("strain energy (J): ", r.energy.item<double>());
  out.writeln("reaction at the middle support (N): ", flat(r.reaction)[2], " ", flat(r.reaction)[3]);
  out.writeln("residual ", r.residual_norm.item<double>(), ", converged ", r.converged.item<bool>(), ", breakdown ", r.breakdown.item<bool>(), ", iterations ", r.iterations);

  // two load cases and four middle-bar areas in one call
  const auto areas = pluss::series::make(std::vector<double>{1e-3, 1e-3, 1e-3, 1e-3, 2e-3, 1e-3, 1e-3, 4e-3, 1e-3, 1e-3, 8e-3, 1e-3}).reshape({4, 3});
  const auto designs = structure::truss({grid({-3, 4, 0, 4, 3, 4, 0, 0}, 2), {0, 1, 2}, {3, 3, 3}}, {areas, vec({200e9, 200e9, 200e9})});
  const auto loads = pluss::series::make(std::vector<double>{0, 0, 0, 0, 0, 0, 0, -10000, 0, 0, 0, 0, 0, 0, 5000, 0}).reshape({2, 1, 4, 2});
  const auto batch = structure::solve(designs, loads, kSupports);
  out.writeln("batched: force shape ", batch.force.sizes(), ", displacement shape ", batch.displacement.sizes());
}
```

Output

```text
displacement of the hung joint (mm): 0.0000 -0.0988
force (N):   [3162.0553, 4940.7115, 3162.0553]
stress (MPa): [3.1621, 4.9407, 3.1621]
strain (1e-6): [15.8103, 24.7036, 15.8103]
strain energy (J): 0.4941
reaction at the middle support (N): 0.0000 4940.7115
residual 0.0000, converged true, breakdown false, iterations 8
batched: force shape [2, 4, 3], displacement shape [2, 4, 4, 2]
```
### Example 2

Solve a cantilever frame.

```cpp
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
  const int members = 4;
  const auto frame = beam(members, false);
  // 100 N down at the free end of a cantilever fixed at node 0: E I = 200e9 * 8.333e-8
  std::vector<double> load(3 * (members + 1), 0.0);
  load[3 * members + 1] = -100.0;
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const auto r = structure::solve(frame, grid(load, 3), wall);

  const auto u = flat(r.displacement);
  out.writeln("tip deflection (mm) ", 1000.0 * u[3 * members + 1], "   formula -P L^3 / (3 E I) = ", -1000.0 * 100.0 * 8.0 / (3 * 200e9 * 8.333e-8));
  out.writeln("tip rotation (rad)  ", u[3 * members + 2], "   formula -P L^2 / (2 E I) = ", -100.0 * 4.0 / (2 * 200e9 * 8.333e-8));
  out.writeln("wall reaction: force ", flat(r.reaction)[1], ", moment ", flat(r.reaction)[2]);
  const auto actions = flat(r.end_force);
  out.writeln("end actions of the member at the wall (Ni Vi Mi Nj Vj Mj): ", std::vector<double>(actions.begin(), actions.begin() + 6));
  out.writeln("strain energy (J) ", r.energy.item<double>(), ", converged ", r.converged.item<bool>(), ", iterations ", r.iterations);
}
```

Output

```text
tip deflection (mm) -16.00064   formula -P L^3 / (3 E I) = -16.00064
tip rotation (rad)  -0.01200   formula -P L^2 / (2 E I) = -0.01200
wall reaction: force 100.00000, moment 200.00000
end actions of the member at the wall (Ni Vi Mi Nj Vj Mj): [0.00000, 100.00000, 200.00000, -0.00000, -100.00000, -150.00000]
strain energy (J) 0.80003, converged true, iterations 8
```
### Example 3

Bend a cantilever through a large angle.

```cpp
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
```

Output

```text
tip down / L 0.3491 (elastica 0.3490), inward / L 0.0763 (elastica 0.0764)
converged true, load factor 1.0000, steps 20, cutbacks 0, Newton iterations 150
residual 0.0000, strain energy (J) 1628.4207
end actions of the member at the wall, in its own turned axes (Ni Vi Mi): -169.2417 4996.9348 9236.1541
tolerance 1e-12, 5 steps: tip down / L 0.3491, iterations 84, cutbacks 0
one iteration per step, failure off: load factor reached 0.0000, converged false
one iteration per step, failure on: eng::structure::solve: nonlinear equilibrium did not converge before the load-step cutback limit
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

statics/SolveOptions
statics/StaticResult
statics/FrameResult
statics/MemberLoad
statics/NonlinearOptions
statics/NonlinearResult
statics/solve
```
<!-- apiref:toctree:end -->
