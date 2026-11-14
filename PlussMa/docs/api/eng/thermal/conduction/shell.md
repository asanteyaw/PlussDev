<!-- generated from shell.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::shell`

The conduction resistance of a spherical shell.

## Signature

```cpp
Series thermal::shell(const Series& conductivity,
                      const Series& r_in,
                      const Series& r_out);
```

## What It Does

Returns `(1 / r_in - 1 / r_out) / (4 pi k)` in kelvin per watt: radial conduction through a spherical wall. As the outer radius grows the resistance tends to `1 / (4 pi k r_in)`, the resistance of the medium around a sphere.

## Parameters

`conductivity`
: The conductivity in W/(m K), positive.

`r_in`
: The inner radius in m, positive.

`r_out`
: The outer radius in m, larger than `r_in`.

## Returns

The resistance in K/W with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for an input that is not a real floating array, not finite or not positive, and when `r_out` does not exceed `r_in`.

## Interpretation and Use

Use it for spherical tanks and insulated spheres; combine it with `film` for the outside convection.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A spherical shell and its limit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A spherical shell of k = 1 from radius 1 m to 2 m: R = (1 / r1 - 1 / r2) / (4 pi k).
  const pluss::Series r = th::shell(vec({1.0}), vec({1.0}), vec({2.0}));
  out.writeln("resistance", flat(r), "K/W  closed form", 0.5 / (4.0 * M_PI));
  out.writeln("an infinite outer radius leaves 1 / (4 pi k r1) =", 1.0 / (4.0 * M_PI), "K/W");
  out.writeln("outer radii 1.5, 2, 4, 100 m", flat(th::shell(vec({1.0}), vec({1.0}), vec({1.5, 2.0, 4.0, 100.0}))), "K/W");
}
```

Output

```text
resistance [0.03979] K/W  closed form 0.03979
an infinite outer radius leaves 1 / (4 pi k r1) = 0.07958 K/W
outer radii 1.5, 2, 4, 100 m [0.02653, 0.03979, 0.05968, 0.07878] K/W
```
### Example 2

An insulated tank.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static double one(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>()[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A tank of 1 m radius held at 80 degrees, insulated to 1.1 m (k = 0.04), losing heat to 20 degree air through h = 8 W/(m^2 K).
  const double r1 = 1.0, r2 = 1.1;
  const pluss::Series insulation = th::shell(vec({0.04}), vec({r1}), vec({r2}));
  const pluss::Series film = th::film(vec({8.0}), vec({4.0 * M_PI * r2 * r2}));
  const double total = one(insulation) + one(film);
  out.writeln("insulation", flat(insulation), "K/W  film", flat(film), "K/W");
  out.writeln("heat loss", 60.0 / total, "W");
}
```

Output

```text
insulation [0.18086] K/W  film [0.00822] K/W
heat loss 317.32818 W
```
### Example 3

Rejected radii.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  report([&] { (void)th::shell(vec({1.0}), vec({2.0}), vec({1.0})); });  // the outer radius is smaller
  report([&] { (void)th::shell(vec({1.0}), vec({0.0}), vec({1.0})); });  // zero inner radius
  report([&] { (void)th::shell(vec({-1.0}), vec({1.0}), vec({2.0})); }); // negative conductivity
}
```

Output

```text
eng::thermal::shell: r_out must exceed r_in
eng::thermal::shell: r_in must be positive
eng::thermal::shell: conductivity must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tube.html">
    <span class="label">Previous</span>
    <span class="title">tube</span>
  </a>
  <a class="next-link" href="film.html">
    <span class="label">Next</span>
    <span class="title">film</span>
  </a>
</div>
<!-- apiref:nav:end -->
