<!-- generated from film.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::film`

The resistance of a convection film.

## Signature

```cpp
Series thermal::film(const Series& coefficient,
                     const Series& area);
```

## What It Does

Returns `1 / (h A)` in kelvin per watt: the resistance between a surface and the fluid that passes over it, for a film coefficient `h` in W/(m^2 K) over an area `A`.

## Parameters

`coefficient`
: The film coefficient `h` in W/(m^2 K), positive.

`area`
: The surface area in m^2, positive.

## Returns

The resistance in K/W with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for an input that is not a real floating array, not finite or not positive.

## Interpretation and Use

The overall coefficient of a wall is `U = 1 / (A * (R_inside + R_wall + R_outside))`; build the three resistances with `film` and `wall`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A film and a sweep of the coefficient.

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
  // Convection resistance 1 / (h A): 25 W/(m^2 K) over 2 m^2.
  out.writeln("resistance", flat(th::film(vec({25.0}), vec({2.0}))), "K/W  closed form", 1.0 / 50.0);
  out.writeln("a sweep of h = 5, 25, 100 W/(m^2 K)", flat(th::film(vec({5.0, 25.0, 100.0}), vec({2.0}))), "K/W");
}
```

Output

```text
resistance [0.02000] K/W  closed form 0.02000
a sweep of h = 5, 25, 100 W/(m^2 K) [0.10000, 0.02000, 0.00500] K/W
```
### Example 2

The overall coefficient of a wall.

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
  // The overall coefficient of a wall: films on both faces plus the wall itself, U = 1 / (A R_total).
  const double area = 12.0;
  const pluss::Series inside = th::film(vec({8.0}), vec({area}));
  const pluss::Series outside = th::film(vec({25.0}), vec({area}));
  const pluss::Series wall = th::wall(vec({0.5}), vec({0.3}), vec({area}));
  const double total = one(inside) + one(wall) + one(outside);
  out.writeln("resistances: inside", flat(inside), " wall", flat(wall), " outside", flat(outside), "K/W");
  out.writeln("U =", 1.0 / (area * total), "W/(m^2 K); heat flow for 25 K", 25.0 / total, "W");
  out.writeln("without the films the wall alone would pass", 25.0 / one(wall), "W");
}
```

Output

```text
resistances: inside [0.01042]  wall [0.05000]  outside [0.00333] K/W
U = 1.30719 W/(m^2 K); heat flow for 25 K 392.15686 W
without the films the wall alone would pass 500.00000 W
```
### Example 3

Rejected inputs.

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
  report([&] { (void)th::film(vec({0.0}), vec({2.0})); });     // zero coefficient
  report([&] { (void)th::film(vec({25.0}), vec({-2.0})); });   // negative area
}
```

Output

```text
eng::thermal::film: coefficient must be positive
eng::thermal::film: area must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shell.html">
    <span class="label">Previous</span>
    <span class="title">shell</span>
  </a>
  <a class="next-link" href="biot.html">
    <span class="label">Next</span>
    <span class="title">biot</span>
  </a>
</div>
<!-- apiref:nav:end -->
