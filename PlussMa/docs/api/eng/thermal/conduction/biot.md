<!-- generated from biot.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::biot`

The Biot number of a body.

## Signature

```cpp
Series thermal::biot(const Series& coefficient,
                      const Series& conductivity,
                      const Series& length);
```

## What It Does

Returns `h L / k`: the ratio of the resistance inside a body (`L / k`) to the resistance of its surface film (`1 / h`). A small Biot number means the body is nearly uniform in temperature.

## Parameters

`coefficient`
: The film coefficient in W/(m^2 K), positive.

`conductivity`
: The conductivity of the body in W/(m K), positive.

`length`
: The characteristic length in m, positive: volume over surface area, or the half-thickness of a plate cooled on both faces.

## Returns

The dimensionless Biot number with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for an input that is not a real floating array, not finite or not positive.

## Interpretation and Use

Below about `0.1` a body may be treated as one lumped capacity (a single store); above it the temperature varies through the body and it should be meshed, for example with `mesh`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Biot numbers of three materials.

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
  // Bi = h L / k compares the surface resistance with the internal one. Copper, steel and a brick in the same air stream.
  const pluss::Series k = vec({400.0, 15.0, 0.7});
  const pluss::Series bi = th::biot(vec({50.0}), k, vec({0.02}));
  out.writeln("Biot numbers", flat(bi), " for copper, steel, brick (h = 50, L = 0.02 m)");
  out.writeln("lumped treatment is sound below about 0.1:", flat(bi.lt(0.1)));
}
```

Output

```text
Biot numbers [0.00250, 0.06667, 1.42857]  for copper, steel, brick (h = 50, L = 0.02 m)
lumped treatment is sound below about 0.1: [1.00000, 1.00000, 0.00000]
```
### Example 2

A lumped body against a mesh, at small and large Biot number.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A plate heated on one face cools through the other. A lumped body (C = rho c L A, G = h A) against a ten-cell mesh.
  const double thickness = 0.02, area = 1.0, h = 20.0, rho = 2700.0, c = 900.0;
  const auto run_for = [&](double k) {
    th::Layers layers;
    layers.cells = {10};
    layers.thickness = vec({thickness});
    layers.conductivity = vec({k});
    layers.density = vec({rho});
    layers.specific_heat = vec({c});
    th::Slab w = th::mesh(layers, vec({area}).reshape({}));
    const std::int64_t ambient = w.net.nodes;
    w.net.nodes += 1;
    w.net.links.a.push_back(w.outer);
    w.net.links.b.push_back(ambient);
    w.net.links.conductance = w.net.links.conductance.unite({vec({h * area})}, -1);
    w.net.fixed = {{ambient}, vec({0.0})};
    th::Start start;
    start.temperature = vec(std::vector<double>(10, 100.0));
    const double tau = rho * c * thickness / h;
    const auto run = th::tran(w.net, vec(grid(tau / 50.0, 51)), {}, start);
    return std::make_pair(flat(run.temperature.select(1, 1)), flat(run.temperature.select(1, 10)));
  };
  const double tau = rho * c * thickness / h;
  const double bi = h * thickness / 200.0;
  const auto [near_face, far_face] = run_for(200.0);
  out.writeln("k = 200: Bi =", bi, " time constant", tau, "s");
  out.writeln("  at one time constant: face next to the heat source", near_face[50], " cooled face", far_face[50], " lumped", 100.0 * std::exp(-1.0));
  const double slow = h * thickness / 0.5;
  const auto [near_slow, far_slow] = run_for(0.5);
  out.writeln("k = 0.5: Bi =", slow);
  out.writeln("  at one time constant: face next to the heat source", near_slow[50], " cooled face", far_slow[50], " lumped", 100.0 * std::exp(-1.0));
}
```

Output

```text
k = 200: Bi = 0.00200  time constant 2430.00000 s
  at one time constant: face next to the heat source 36.82198  cooled face 36.80759  lumped 36.78794
k = 0.5: Bi = 0.80000
  at one time constant: face next to the heat source 50.37563  cooled face 36.84239  lumped 36.78794
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
  report([&] { (void)th::biot(vec({0.0}), vec({15.0}), vec({0.02})); });   // zero coefficient
  report([&] { (void)th::biot(vec({50.0}), vec({0.0}), vec({0.02})); });   // zero conductivity
  report([&] { (void)th::biot(vec({50.0}), vec({15.0}), vec({-0.02})); }); // negative length
}
```

Output

```text
eng::thermal::biot: coefficient must be positive
eng::thermal::biot: conductivity must be positive
eng::thermal::biot: length must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="film.html">
    <span class="label">Previous</span>
    <span class="title">film</span>
  </a>
  <a class="next-link" href="Tip.html">
    <span class="label">Next</span>
    <span class="title">Tip</span>
  </a>
</div>
<!-- apiref:nav:end -->
