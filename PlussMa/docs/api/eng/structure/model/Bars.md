<!-- generated from Bars.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::Bars`

Cross-section area and modulus of each truss bar.

## Signature

```cpp
structure::Bars
```

## What It Does

A `Bars` holds the section properties of the bars of a truss: the cross-sectional area and Young's modulus of each. A bar of length `L` acts as an axial spring of stiffness `E A / L`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `area` | Cross-sectional area of each bar, shape `[..., bar]`, positive. | required |
| `modulus` | Young's modulus of each bar, shape `[..., bar]`, positive. | required |

## Returns

A value; validated by `truss`.

## Errors

A non-positive or non-finite entry, a length that differs from the bar count, or a dtype or platform different from the mesh is rejected by `truss`.

## Interpretation and Use

Use one consistent system of units for coordinates, areas, moduli and loads; nothing can detect a mixture. Leading axes are batches of designs (a sweep of areas) that broadcast against the batch axes of the mesh and the load.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A mesh and bars compiled into a truss, with a batch of apex heights.

```cpp
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
  out.print_defaults(PrintOptions{}.with_sep(""));
  // a plane truss: two bars from the feet (0, 0) and (6, 0) to the apex (3, 4)
  const structure::Mesh plane{grid({0, 0, 6, 0, 3, 4}, 2), {0, 1}, {2, 2}};
  const auto triangle = structure::truss(plane, {vec({1e-3, 1e-3}), vec({200e9, 200e9})});
  out.writeln("plane truss: nodes ", triangle.nodes(), ", bars ", triangle.bars(), ", axes ", triangle.dim());

  // a space truss: three legs from a ring on the ground to an apex 2 m above its centre
  const structure::Mesh space{grid({1.5, 0, 0, -0.75, 1.299038105676658, 0, -0.75, -1.299038105676658, 0, 0, 0, 2}, 3), {0, 1, 2}, {3, 3, 3}};
  const auto tripod = structure::truss(space, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
  out.writeln("space truss: nodes ", tripod.nodes(), ", bars ", tripod.bars(), ", axes ", tripod.dim());

  // a batch of three apex heights: a leading axis on the positions describes three trusses at once
  const auto positions = pluss::series::make(std::vector<double>{0, 0, 6, 0, 3, 2, 0, 0, 6, 0, 3, 4, 0, 0, 6, 0, 3, 8}).reshape({3, 3, 2});
  const auto batch = structure::truss({positions, {0, 1}, {2, 2}}, {vec({1e-3, 1e-3}), vec({200e9, 200e9})});
  const auto result = structure::solve(batch, grid({0, 0, 0, 0, 0, -10000}, 2), {{0, 0, 1, 1}, {0, 1, 0, 1}, {}});
  out.writeln("batch of three heights: bar forces have shape ", result.force.sizes());
}
```

Output

```text
plane truss: nodes 3, bars 2, axes 2
space truss: nodes 4, bars 3, axes 3
batch of three heights: bar forces have shape [3, 2]
```
### Example 2

A batch of four middle-bar areas solved against two load cases.

```cpp
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
### Example 3

Rejected bar properties.

```cpp
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

#include <functional>

static void report(const char *what, const std::function<void()> &build) {
  try {
    build();
    std::cout << what << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << what << ": " << e.what() << "\n";
  }
}

int main() {
  const auto line = grid({0, 0, 2, 0}, 2);
  const structure::Bars good{vec({1e-3}), vec({200e9})};
  report("negative area         ", [&] { (void)structure::truss({line, {0}, {1}}, {vec({-1e-3}), vec({200e9})}); });
  report("bar from a node to itself", [&] { (void)structure::truss({line, {0}, {0}}, good); });
  report("coincident ends       ", [&] { (void)structure::truss({grid({0, 0, 0, 0}, 2), {0}, {1}}, good); });
  report("node index out of range", [&] { (void)structure::truss({line, {0}, {5}}, good); });
  report("wrong area count      ", [&] { (void)structure::truss({line, {0}, {1}}, {vec({1e-3, 1e-3}), vec({200e9})}); });
  report("coordinates not a grid", [&] { (void)structure::truss({vec({0, 2}), {0}, {1}}, good); });
}
```

Output

```text
negative area         : eng::structure::truss: area and modulus must be positive
bar from a node to itself: eng::structure::truss: an element cannot connect a node to itself
coincident ends       : eng::structure::truss: every bar must have positive length
node index out of range: eng::structure::truss: connectivity contains an invalid node
wrong area count      : eng::structure::truss: area and modulus must end in the bar count
coordinates not a grid: eng::structure::truss: position must be real floating [..., node, axis]
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Mesh.html">
    <span class="label">Previous</span>
    <span class="title">Mesh</span>
  </a>
  <a class="next-link" href="Sections.html">
    <span class="label">Next</span>
    <span class="title">Sections</span>
  </a>
</div>
<!-- apiref:nav:end -->
