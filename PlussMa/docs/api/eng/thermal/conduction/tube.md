<!-- generated from tube.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::tube`

The conduction resistance of a cylindrical shell.

## Signature

```cpp
Series thermal::tube(const Series& conductivity,
                     const Series& r_in,
                     const Series& r_out,
                     const Series& length);
```

## What It Does

Returns `ln(r_out / r_in) / (2 pi k L)` in kelvin per watt: radial conduction through a pipe wall or its insulation, from the inner radius to the outer one, along a length `L` with no axial flow.

## Parameters

`conductivity`
: The conductivity in W/(m K), positive.

`r_in`
: The inner radius in m, positive.

`r_out`
: The outer radius in m, larger than `r_in`.

`length`
: The length of the tube in m, positive.

## Returns

The resistance in K/W with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for an input that is not a real floating array, not finite or not positive, and when `r_out` does not exceed `r_in`.

## Interpretation and Use

Insulating a thin pipe can raise heat loss until the outer radius passes the critical radius `k / h`, because the film outside shrinks faster than the wall grows.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Insulation on a pipe.

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
  // Insulation on a pipe: 25 mm to 50 mm radius, k = 0.04 W/(m K), 10 m long.
  const pluss::Series r = th::tube(vec({0.04}), vec({0.025}), vec({0.05}), vec({10.0}));
  out.writeln("resistance", flat(r), "K/W  closed form", std::log(2.0) / (2.0 * M_PI * 0.04 * 10.0));
  out.writeln("heat loss for a 60 K difference", 60.0 / one(r), "W");
  const pluss::Series thicker = th::tube(vec({0.04}), vec({0.025}), vec({0.05, 0.075, 0.1}), vec({10.0}));
  out.writeln("outer radii 50, 75, 100 mm", flat(thicker), "K/W");
}
```

Output

```text
resistance [0.27579] K/W  closed form 0.27579
heat loss for a 60 K difference 217.55329 W
outer radii 50, 75, 100 mm [0.27579, 0.43712, 0.55159] K/W
```
### Example 2

The critical radius of insulation.

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
  // A thin pipe with a film outside: insulation first lowers the resistance until the outer radius passes k / h.
  const double k = 0.2, h = 10.0, length = 1.0;
  const pluss::Series r_out = vec({0.005, 0.01, 0.02, 0.04, 0.08});
  const pluss::Series wall = th::tube(vec({k}), vec({0.005}), r_out.slice(0, 1, 5), vec({length}));
  const pluss::Series film = th::film(vec({h}), r_out.mul(2.0 * M_PI * length));
  const std::vector<double> w = flat(wall), f = flat(film);
  out.writeln("critical radius k / h =", k / h, "m");
  const double radius[] = {0.01, 0.02, 0.04, 0.08};
  for (int i = 0; i < 4; ++i) out.writeln("outer radius", radius[i], "m  wall", w[i], " film", f[i + 1], " total", w[i] + f[i + 1], "K/W");
}
```

Output

```text
critical radius k / h = 0.02000 m
outer radius 0.01000 m  wall 0.55159  film 1.59155  total 2.14314 K/W
outer radius 0.02000 m  wall 1.10318  film 0.79577  total 1.89895 K/W
outer radius 0.04000 m  wall 1.65477  film 0.39789  total 2.05265 K/W
outer radius 0.08000 m  wall 2.20636  film 0.19894  total 2.40530 K/W
```
### Example 3

Rejected radii and lengths.

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
  report([&] { (void)th::tube(vec({0.04}), vec({0.05}), vec({0.025}), vec({10.0})); });  // the outer radius is smaller
  report([&] { (void)th::tube(vec({0.04}), vec({0.025}), vec({0.025}), vec({10.0})); }); // equal radii
  report([&] { (void)th::tube(vec({0.04}), vec({0.025}), vec({0.05}), vec({0.0})); });   // zero length
}
```

Output

```text
eng::thermal::tube: r_out must exceed r_in
eng::thermal::tube: r_out must exceed r_in
eng::thermal::tube: length must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="wall.html">
    <span class="label">Previous</span>
    <span class="title">wall</span>
  </a>
  <a class="next-link" href="shell.html">
    <span class="label">Next</span>
    <span class="title">shell</span>
  </a>
</div>
<!-- apiref:nav:end -->
