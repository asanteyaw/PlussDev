<!-- generated from BuckResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::BuckResult`

Buckling factors and buckled shapes of a frame.

## Signature

```cpp
structure::BuckResult
```

## What It Does

The result of `buckling`: the load multipliers at which the frame becomes unstable and the shape it buckles into.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `factor` | Multipliers on the reference load, ascending, `[..., mode]`. |  |
| `shape` | Buckled shapes `(ux, uy, rz)`, `[..., mode, node, 3]`, scaled so that the largest component has magnitude one. |  |
| `residual` | `|| K phi - factor K_g phi ||`, `[..., mode]`. |  |
| `converged` | One where the pair reached its tolerance. |  |
| `iterations` | Iterations used. |  |

## Returns

Returned by `buckling`.

## Errors

Reading a field does not solve again.

## Interpretation and Use

A factor is a multiplier, not a load: the critical load is the factor times the reference load.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field of a buckling solve on a pinned column.

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
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  const int members = 8;
  const auto column = beam(members, false);
  const double euler = M_PI * M_PI * 200e9 * 8.333e-8 / 4.0;
  out.writeln("Euler load of the pinned column (N): ", euler);

  // a 1000 N compressive reference load along the column: the factors times 1000 are the critical loads
  std::vector<double> load(3 * (members + 1), 0.0);
  load[3 * members] = -1000.0;
  structure::EigOptions options;
  options.count = 3;
  const auto r = structure::buckling(column, grid(load, 3), {{0, 0, members}, {0, 1, 1}, {}}, options);
  out.writeln("factors: ", flat(r.factor), ", critical loads (N): ", flat(r.factor.mul(1000.0)));
  out.writeln("converged ", flat(r.converged), ", residuals ", flat(r.residual), ", shape array ", r.shape.sizes(), ", iterations ", r.iterations);

  // the same column fixed at the base and free at the top (a cantilever): a quarter of the load
  const auto cantilever = structure::buckling(column, grid(load, 3), {{0, 0, 0}, {0, 1, 2}, {}});
  out.writeln("cantilever critical load (N): ", flat(cantilever.factor)[0] * 1000.0, "   Euler / 4 = ", euler / 4.0);

  // a reference load that doesn't compress anything
  std::vector<double> pull(3 * (members + 1), 0.0);
  pull[3 * members] = 1000.0;
  try {
    (void)structure::buckling(column, grid(pull, 3), {{0, 0, members}, {0, 1, 1}, {}});
  } catch (const std::exception &e) {
    std::cout << "tensile reference load: " << e.what() << "\n";
  }
}
```

Output

```text
Euler load of the pinned column (N): 41121.71
factors: [41.12, 164.57, 371.02], critical loads (N): [41123.05, 164571.07, 371020.06]
converged [1.00, 1.00, 1.00], residuals [0.00, 0.00, 0.00], shape array [3, 9, 3], iterations 25
cantilever critical load (N): 10280.45   Euler / 4 = 10280.43
tensile reference load: eng::structure::buckling: the reference load puts no member in compression, so there is no buckling factor; apply a compressive axial reference load (a zero, tensile or purely transverse load never buckles a frame in a linearised analysis)
```
### Example 2

Four end conditions against Euler's loads.

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
  out.print_defaults(PrintOptions{}.fixed(1).with_sep(""));
  const double euler = M_PI * M_PI * 200e9 * 8.333e-8 / 4.0;
  out.writeln("pi^2 E I / L^2 = ", euler, " N");
  const int members = 16;
  const auto column = beam(members, false);
  std::vector<double> load(3 * (members + 1), 0.0);
  load[3 * members] = -1.0;   // a 1 N reference load: the factor is the critical load in newtons
  struct Case { const char *name; structure::Fix fix; double units; };
  const Case cases[] = {
      {"cantilever            ", {{0, 0, 0}, {0, 1, 2}, {}}, 0.25},
      {"pinned-pinned         ", {{0, 0, members}, {0, 1, 1}, {}}, 1.0},
      {"fixed-pinned          ", {{0, 0, 0, members}, {0, 1, 2, 1}, {}}, 2.0457},
      {"fixed-fixed (sliding) ", {{0, 0, 0, members, members}, {0, 1, 2, 1, 2}, {}}, 4.0},
  };
  for (const Case &c : cases) {
    const auto r = structure::buckling(column, grid(load, 3), c.fix);
    out.writeln(c.name, "critical load (N) ", flat(r.factor)[0], "   Euler ", c.units * euler);
  }
}
```

Output

```text
pi^2 E I / L^2 = 41121.7 N
cantilever            critical load (N) 10280.4   Euler 10280.4
pinned-pinned         critical load (N) 41121.8   Euler 41121.7
fixed-pinned          critical load (N) 84125.4   Euler 84122.7
fixed-fixed (sliding) critical load (N) 164492.2   Euler 164486.8
```
### Example 3

A reference load that compresses nothing.

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

#include <functional>

static void report(const char *what, const std::function<void()> &call) {
  try {
    call();
    std::cout << what << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << what << ": " << e.what() << "\n";
  }
}

int main() {
  const int members = 4;
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  std::vector<double> push(3 * (members + 1), 0.0);
  push[3 * members] = -1000.0;
  report("modes without density      ", [&] { (void)structure::modes(beam(members, false), wall); });
  report("mass without density       ", [&] { (void)structure::mass(beam(members, false), grid(std::vector<double>(3 * (members + 1), 0.0), 3)); });
  report("more modes than free dofs   ", [&] { structure::EigOptions o; o.count = 50; (void)structure::modes(beam(members), wall, o); });
  report("zero modes requested        ", [&] { structure::EigOptions o; o.count = 0; (void)structure::modes(beam(members), wall, o); });
  report("buckling, tensile reference ", [&] { for (auto &v : push) v = -v; (void)structure::buckling(beam(members, false), grid(push, 3), {{0, 0, members}, {0, 1, 1}, {}}); });
}
```

Output

```text
modes without density      : eng::structure::modes: frame sections do not define density
mass without density       : eng::structure::mass: frame sections do not define density
more modes than free dofs   : eng::structure::modes: count exceeds the free degree count
zero modes requested        : eng::structure::modes: count and max_iterations must both be positive
buckling, tensile reference : eng::structure::buckling: the reference load puts no member in compression, so there is no buckling factor; apply a compressive axial reference load (a zero, tensile or purely transverse load never buckles a frame in a linearised analysis)
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ModeResult.html">
    <span class="label">Previous</span>
    <span class="title">ModeResult</span>
  </a>
  <a class="next-link" href="mass.html">
    <span class="label">Next</span>
    <span class="title">mass</span>
  </a>
</div>
<!-- apiref:nav:end -->
