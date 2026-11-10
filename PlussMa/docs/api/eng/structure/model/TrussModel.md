<!-- generated from TrussModel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::Truss`

An immutable compiled truss of pin-jointed bars.

## Signature

```cpp
std::int64_t structure::Truss::nodes() const noexcept;
std::int64_t structure::Truss::bars() const noexcept;
std::int64_t structure::Truss::dim() const noexcept;
```

## What It Does

A `Truss` is what `structure::truss` returns: the geometry, bar lengths, directions, areas, moduli and stiffnesses of a validated truss, held immutably so that one model can be solved repeatedly under many loads and supports. Its only public members are three counts; the rest is for the solvers.

## Methods

`nodes()`
: The number of nodes.

`bars()`
: The number of bars.

`dim()`
: The number of spatial axes, `2` for a plane truss and `3` for a space truss.

## Returns

Accessors return counts.

## Errors

Accessors do not fail. Invalid descriptions are rejected by `structure::truss` before a `Truss` exists.

## Changelog

- **0.1** — Initial public contract.

## Interpretation and Use

A `Truss` cannot be default-constructed; build it with `truss`. It is cheap to copy. Solve it with `solve(truss, load, fix, options)`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Counts and dimension of a plane and a space truss.

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

Solving a compiled truss.

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

The same truss under a different support arrangement.

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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the triangle with a tie bar from the left foot to the right foot
  const auto truss = structure::truss({grid({0, 0, 6, 0, 3, 4}, 2), {0, 1, 0}, {2, 2, 1}}, {vec({1e-3, 1e-3, 1e-3}), vec({200e9, 200e9, 200e9})});
  const auto load = grid({0, 0, 0, 0, 0, -10000}, 2);

  // a pin at node 0 (x and y) and a roller at node 1 (y only)
  const structure::Fix pin_and_roller{{0, 0, 1}, {0, 1, 1}, {}};
  const auto a = structure::solve(truss, load, pin_and_roller);
  out.writeln("pin and roller: right foot slides (mm) ", 1000.0 * flat(a.displacement)[2], ", tie force (N) ", flat(a.force)[2]);

  // two pins: the right foot cannot slide, and the tie carries nothing
  const structure::Fix two_pins{{0, 0, 1, 1}, {0, 1, 0, 1}, {}};
  const auto b = structure::solve(truss, load, two_pins);
  out.writeln("two pins:       right foot slides (mm) ", 1000.0 * flat(b.displacement)[2] + 0.0, ", tie force (N) ", flat(b.force)[2]);

  // a prescribed value: the right foot is also pushed 1 mm to the right (the third pair is node 1, x, value 0.001)
  const structure::Fix pushed{{0, 0, 1, 1}, {0, 1, 0, 1}, vec({0, 0, 0.001, 0})};
  const auto c = structure::solve(truss, load, pushed);
  out.writeln("prescribed 1 mm: right foot moves (mm) ", 1000.0 * flat(c.displacement)[2], ", tie force (N) ", flat(c.force)[2]);

  // the same degree of freedom named twice is rejected
  try {
    (void)structure::solve(truss, load, structure::Fix{{0, 0, 0}, {0, 1, 1}, {}});
  } catch (const std::exception &e) {
    std::cout << "a degree of freedom named twice: " << e.what() << "\n";
  }
}
```

Output

```text
pin and roller: right foot slides (mm) 0.1125, tie force (N) 3750.0000
two pins:       right foot slides (mm) 0.0000, tie force (N) 0.0000
prescribed 1 mm: right foot moves (mm) 1.0000, tie force (N) 33333.3333
a degree of freedom named twice: eng::structure::solve: constrained degrees of freedom must be unique
```

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Fix.html">
    <span class="label">Previous</span>
    <span class="title">Fix</span>
  </a>
  <a class="next-link" href="FrameModel.html">
    <span class="label">Next</span>
    <span class="title">Frame</span>
  </a>
</div>
<!-- apiref:nav:end -->
