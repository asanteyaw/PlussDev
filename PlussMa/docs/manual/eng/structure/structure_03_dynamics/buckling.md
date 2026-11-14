<!-- generated from buckling.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Buckling

[Back to Structures 3: Dynamics](../structure_03_dynamics.md)

A slender column loaded in compression carries the load straight down until the load reaches a critical value, and then it bows sideways and cannot carry more. That failure is **buckling**, and it happens at a stress far below the strength of the material, so a column is designed against it separately. `structure::buckling` finds the critical loads of a frame. This page finds the buckling loads of a column with four different end conditions against Euler's formulas, checks the shape of the buckled column, and shows how the result scales with the reference load.

## The mental model

Compression stiffens nothing: it softens the member sideways. The sideways stiffness of a member under an axial compressive force `N` is its ordinary stiffness `K` minus a term proportional to `N`, the **geometric stiffness** `K_g`. The member loses its sideways stiffness when `K - lambda K_g` can be singular, and the load multiplier at which that first happens is the buckling factor `lambda`: the eigenvalue of

```{math}
K \phi = \lambda K_g \phi,
```

where `K_g` is built from the axial forces that the **reference load** produces. `buckling(frame, reference_load, fix, options)` solves the structure under the reference load to find those axial forces, builds `K_g`, and returns the lowest positive factors. It returns a `BuckResult`:

| Member | Shape | Meaning |
|---|---|---|
| `factor` | `[..., mode]` | the multipliers `lambda`, ascending: the structure buckles when the load is `lambda` times the reference load |
| `shape` | `[..., mode, node, 3]` | the buckled shape, scaled so that its largest component is one in magnitude |
| `residual` | `[..., mode]` | `\|\| K phi - lambda K_g phi \|\|`, how well the pair solves the equation |
| `converged` | `[..., mode]` | one when the mode reached its tolerance |
| `iterations` | (a number) | iterations used |

The reference load is any load; the answer is a **factor** on it. Apply a reference load of `1 N` and the factor is the critical load in newtons; apply `2000 N` and the critical load is `2000` times the factor. The analysis is linearised: it finds where the structure stops being stable, not how it behaves after, and it ignores imperfections, which in a real column lower the critical load.

## Euler's four columns

A column of length `L`, bending stiffness `E I`, has Euler buckling load

```{math}
P_{cr} = \frac{\pi^2 E I}{(K L)^2},
```

where `K` is an **effective length factor** that depends on how the ends are held. The classical cases, with the loaded end free to move in the load direction and the rest as stated:

| Ends | Effective length factor `K` | `P_cr` in units of `pi^2 E I / L^2` |
|---|---|---|
| one end fixed, the other free (cantilever) | 2 | 1/4 |
| both ends pinned | 1 | 1 |
| one end fixed, the other pinned | about 0.7 | about 2.046 |
| both ends fixed, one free to slide along the axis | 0.5 | 4 |

The program builds a steel column `2 m` long with `E I = 200e9 * 8.333e-8 = 16.67 kN m^2`, so that `pi^2 E I / L^2 = 41.12 kN`, and puts a `1 N` compressive reference load at its top, along the column, so that the factors are the critical loads in newtons. It finds the lowest factor for each of the four end conditions with 4, 8 and 16 members and prints it beside Euler's value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static st::Frame column(int members, double length) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(length * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  const std::vector<double> area(members, 1e-3), inertia(members, 8.333e-8), modulus(members, 200e9);
  return st::frame({grid(position, 2), from, to}, {vec(area), vec(inertia), vec(modulus)});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(1).with_sep(""));
  const double length = 2.0, euler = M_PI * M_PI * 200e9 * 8.333e-8 / (length * length);
  out.writeln("pi^2 E I / L^2 = ", euler, " N");

  struct Case { const char *name; double units; };
  const Case cases[] = {{"cantilever          ", 0.25}, {"pinned-pinned       ", 1.0}, {"fixed-pinned        ", 2.0457}, {"fixed-fixed (sliding)", 4.0}};
  for (int members : {4, 8, 16}) {
    const auto frame = column(members, length);
    std::vector<double> reference(3 * (members + 1), 0.0);
    reference[3 * members] = -1.0;   // 1 N pushing the top node toward the base
    const auto load = grid(reference, 3);
    const st::Fix fixes[] = {
        {{0, 0, 0}, {0, 1, 2}, {}},                          // cantilever: base fixed
        {{0, 0, members}, {0, 1, 1}, {}},                    // pinned-pinned: pin at the base, roller at the top
        {{0, 0, 0, members}, {0, 1, 2, 1}, {}},              // fixed base, roller at the top
        {{0, 0, 0, members, members}, {0, 1, 2, 1, 2}, {}},  // both ends fixed in rotation; the top slides along the axis
    };
    out.writeln(members, " members:");
    for (int c = 0; c < 4; ++c) {
      st::EigOptions options;
      options.count = 1;
      const auto result = st::buckling(frame, load, fixes[c], options);
      out.writeln("  ", cases[c].name, " critical load (N): ", flat(result.factor)[0], "   Euler ", cases[c].units * euler);
    }
  }
}
```

Output

```text
pi^2 E I / L^2 = 41121.7 N
4 members:
  cantilever           critical load (N): 10280.8   Euler 10280.4
  pinned-pinned        critical load (N): 41142.8   Euler 41121.7
  fixed-pinned         critical load (N): 84297.5   Euler 84122.7
  fixed-fixed (sliding) critical load (N): 165724.2   Euler 164486.8
8 members:
  cantilever           critical load (N): 10280.4   Euler 10280.4
  pinned-pinned        critical load (N): 41123.1   Euler 41121.7
  fixed-pinned         critical load (N): 84136.1   Euler 84122.7
  fixed-fixed (sliding) critical load (N): 164571.1   Euler 164486.8
16 members:
  cantilever           critical load (N): 10280.4   Euler 10280.4
  pinned-pinned        critical load (N): 41121.8   Euler 41121.7
  fixed-pinned         critical load (N): 84125.4   Euler 84122.7
  fixed-fixed (sliding) critical load (N): 164492.2   Euler 164486.8
```

With sixteen members every case is within about `0.01 percent` of its classical value, and every case approaches it from above as the mesh is refined, because a finite-element column is slightly stiffer than the continuous one. The fixed-pinned value is the root of a transcendental equation, `2.0457 pi^2 E I / L^2` (the effective length factor of about `0.7`); the cantilever, pinned and fixed-fixed ones are exact multiples.

## The buckled shape

At the critical load the column bows into a shape that belongs to the factor, with its amplitude undetermined. The pinned-pinned column's first shape is a half sine wave, `sin(pi x / L)`. The program finds the first three factors of the sixteen-member pinned-pinned column with a reference load of `2000 N` and prints the transverse shape of the first mode against the half sine, the critical load `2000 * factor`, and the next two factors, which belong to the shapes with one and two interior nodes and are `4` and `9` times the first (Euler's modes `n^2 pi^2 E I / L^2`).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 16;
  const double length = 2.0, reference = 2000.0;
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(length * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  const auto column = st::frame({grid(position, 2), from, to}, {vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))});

  std::vector<double> load(3 * (members + 1), 0.0);
  load[3 * members] = -reference;
  st::EigOptions options;
  options.count = 3;
  const auto result = st::buckling(column, grid(load, 3), {{0, 0, members}, {0, 1, 1}, {}}, options);

  const auto factor = flat(result.factor);
  out.writeln("factors: ", factor, "   ratios to the first: ", factor[1] / factor[0], " ", factor[2] / factor[0]);
  out.writeln("critical load of the first mode (N): ", reference * factor[0]);

  // the first shape, normalised to a largest component of magnitude one, against the half sine
  const auto shape = flat(result.shape.select(0, 0).select(-1, 1));
  double largest = 0.0;
  for (double v : shape) largest = std::max(largest, std::abs(v));
  const double sign = shape[members / 2] < 0 ? -1.0 : 1.0;
  out.writeln("node     shape       sin(pi x / L)");
  for (int i = 0; i <= members; i += 2) out.writeln(i, "      ", sign * shape[i] / largest + 0.0, "     ", std::sin(M_PI * i / members));
}
```

Output

```text
factors: [20.5609, 82.2461, 185.0781]   ratios to the first: 4.0001 9.0015
critical load of the first mode (N): 41121.7915
node     shape       sin(pi x / L)
0      0.0000     0.0000
2      0.3827     0.3827
4      0.7071     0.7071
6      0.9239     0.9239
8      1.0000     1.0000
10      0.9239     0.9239
12      0.7071     0.7071
14      0.3827     0.3827
16      0.0000     0.0000
```

The factor times the reference load is `41121.8 N`, the pinned column's Euler load `41121.7 N`; the next factors are `4.0001` and `9.0015` times the first; and the shape follows the half sine to the printed digits. A shape of the first mode is normalised so that its largest component is one in magnitude; its sign is arbitrary.

## The reference load and when buckling does not happen

The reference load fixes only the **pattern** of the compression. Doubling the reference load halves the factor, and the product, the critical load, is unchanged. The program applies reference loads of `1`, `1000` and `50000 N` to the pinned column and prints factor and critical load for each.

If the reference load puts no member in compression, there is no buckling factor: a tensile load stiffens the structure, a purely transverse load produces bending and no axial force, and a load applied directly onto a support never reaches a member. The program shows the error for a tensile and for a transverse reference load.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  const int members = 8;
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
  const auto column = st::frame({grid(position, 2), from, to}, {vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))});
  const st::Fix pinned{{0, 0, members}, {0, 1, 1}, {}};
  const auto top = [&](double axial, double transverse) {
    std::vector<double> load(3 * (members + 1), 0.0);
    load[3 * members] = axial;
    load[3 * (members / 2) + 1] = transverse;
    return grid(load, 3);
  };

  for (double reference : {1.0, 1000.0, 50000.0}) {
    const auto result = st::buckling(column, top(-reference, 0.0), pinned);
    const double factor = flat(result.factor)[0];
    out.writeln("reference load ", reference, " N: factor ", factor, ", critical load (N) ", reference * factor);
  }

  const auto attempt = [&](const char *what, const pluss::Series &load) {
    try {
      (void)st::buckling(column, load, pinned);
      std::cout << what << ": solved\n";
    } catch (const std::exception &e) {
      std::cout << what << ": " << e.what() << "\n";
    }
  };
  attempt("tensile reference load   ", top(+1000.0, 0.0));
  attempt("transverse reference load", top(0.0, -1000.0));
}
```

Output

```text
reference load 1.00 N: factor 41123.05, critical load (N) 41123.05
reference load 1000.00 N: factor 41.12, critical load (N) 41123.05
reference load 50000.00 N: factor 0.82, critical load (N) 41123.05
tensile reference load   : eng::structure::buckling: the reference load puts no member in compression, so there is no buckling factor; apply a compressive axial reference load (a zero, tensile or purely transverse load never buckles a frame in a linearised analysis)
transverse reference load: eng::structure::buckling: the reference load puts no member in compression, so there is no buckling factor; apply a compressive axial reference load (a zero, tensile or purely transverse load never buckles a frame in a linearised analysis)
```

The critical load is `41123.05 N` however the reference is scaled; a factor below one means the reference load itself is already past buckling. Without compression in any member the call fails with an error that says what is missing, rather than returning a meaningless number.

## Decisions

- **Which reference load:** the real service load, if you want the factor to be a safety margin against buckling (a factor of `3` means the load can be tripled); a unit load, if you want the critical load directly.
- **How many modes:** usually one; ask for more to see how close the next mode is, since a structure with several nearly equal factors is sensitive to imperfection.
- **A frame, not just a column:** the same call works on any frame; the reference load is applied to the whole frame and every member that carries compression contributes to the instability.

## Pitfalls

The program shows two mistakes: a mesh too coarse to represent the buckled shape, and reading a factor as a load. For the pinned column it finds the factor with `1`, `2`, `4` and `8` members and a reference load of `1000 N`, and prints the critical load beside Euler's value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace st = pluss::subsp::eng::structure;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a straight steel beam along x, 2 m long, with density
static st::Frame beam(int members, bool with_density = true) {
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
  st::Sections sections{vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))};
  if (with_density) sections.density = vec(std::vector<double>(members, 7850.0));
  return st::frame({grid(position, 2), from, to}, sections);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  const double euler = M_PI * M_PI * 200e9 * 8.333e-8 / 4.0, reference = 1000.0;
  out.writeln("Euler load for the pinned column (N): ", euler);
  for (int members : {1, 2, 4, 8}) {
    std::vector<double> load(3 * (members + 1), 0.0);
    load[3 * members] = -reference;
    const auto r = st::buckling(beam(members, false), grid(load, 3), {{0, 0, members}, {0, 1, 1}, {}});
    const double factor = flat(r.factor)[0];
    out.writeln(members, " member(s): factor ", factor, ", critical load = factor x reference (N) ", factor * reference);
  }
}
```

Output

```text
Euler load for the pinned column (N): 41121.71
1 member(s): factor 50.00, critical load = factor x reference (N) 49998.00
2 member(s): factor 41.43, critical load = factor x reference (N) 41431.04
4 member(s): factor 41.14, critical load = factor x reference (N) 41142.77
8 member(s): factor 41.12, critical load = factor x reference (N) 41123.05
```

One member cannot bend into a half wave: it overestimates the load by `22 percent`; the estimate falls toward Euler's value as the mesh is refined, always from above, so a coarse mesh is unsafe. And the factor `41.12` is not a load: the critical load is the factor times the reference load, `41 kN`.

Reference: [`buckling`](../../../../api/eng/structure/spectral/buckling.md), [`BuckResult`](../../../../api/eng/structure/spectral/BuckResult.md), [`EigOptions`](../../../../api/eng/structure/spectral/EigOptions.md), [`Fix`](../../../../api/eng/structure/model/Fix.md).

Where next: [Transient response](transient_response.md) follows a structure through time when the load changes, with damping and with initial motion.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mass_and_modes.html">
    <span class="label">Previous</span>
    <span class="title">Mass and natural modes</span>
  </a>
  <a class="next-link" href="transient_response.html">
    <span class="label">Next</span>
    <span class="title">Transient response</span>
  </a>
</div>
<!-- apiref:nav:end -->
