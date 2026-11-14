<!-- generated from walls_and_fins.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Walls, fins and meshes

[Back to Thermal 1: Conduction](../thermal_01_conduction.md)

Before you can build a network you need its conductances, and before you trust a one-body model you need to know when it is allowed. This page gives the resistance of the standard shapes, the Biot number that says whether a body can be lumped, fins that extend a surface, and a layered wall that turns into a network of many nodes.

## The mental model

Conduction through a layer is a resistance, `R = L / (k A)` for a plane wall, and resistances in series add. Convection at a surface is another resistance, `1 / (h A)`. A body is **lumped** (one temperature) when its internal resistance is small against its surface resistance: the Biot number `h L / k` compares them. When it is not small the temperature varies inside the body, and the body has to be divided into cells. A fin is the opposite design question: how much heat does a protrusion lose, given that its own conduction limits how much of it is useful.

## Resistances of the standard shapes

A plane wall, with a sweep of thicknesses (the inputs broadcast):

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
  // A concrete wall, 0.2 m thick, k = 1.4 W/(m K), 10 m^2: R = L / (k A).
  const pluss::Series r = th::wall(vec({1.4}), vec({0.2}), vec({10.0}));
  out.writeln("resistance", flat(r), "K/W  closed form", 0.2 / (1.4 * 10.0));
  out.writeln("heat flow for 20 K across the wall", 20.0 / one(r), "W");
  const pluss::Series sweep = th::wall(vec({1.4}), vec({0.1, 0.2, 0.4}), vec({10.0}));
  out.writeln("three thicknesses (broadcast)", flat(sweep), "K/W");
}
```

Output

```text
resistance [0.01429] K/W  closed form 0.01429
heat flow for 20 K across the wall 1400.00000 W
three thicknesses (broadcast) [0.00714, 0.01429, 0.02857] K/W
```
A pipe wall or its insulation is a cylindrical shell, with `R = ln(r_out / r_in) / (2 pi k L)`:

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
A spherical shell, `(1 / r_in - 1 / r_out) / (4 pi k)`, tends to a finite limit as the outer radius grows:

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
A convection film, `1 / (h A)`, completes the set:

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
The overall coefficient of a wall, `U`, is the reciprocal of the area times the sum of the films and the wall:

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
The films matter: the wall alone would pass 500 W for 25 K, but with the two films 392 W. A tank is the spherical version of the same sum:

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
## The critical radius of insulation

Insulation on a thin pipe has a counter-intuitive effect: it adds a wall resistance and also enlarges the outside surface, which lowers the film resistance. Below the radius `k / h` the second effect wins and adding insulation increases heat loss.

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
The total resistance falls from 2.14 to 1.90 K/W as the outer radius grows to the critical radius of 0.02 m and then rises. The practical consequence: small pipes and wires may need a thick layer before insulation helps at all.

## When may a body be lumped?

The Biot number `h L / k`, with `L` the volume over the cooled area, compares internal and surface resistance.

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
Copper and steel in this air stream are below 0.1 and may be lumped (steel only just); brick, at 1.43, may not. The program below tests the rule. A plate with an insulated back face cools through its front face, once with a conductor (Biot 0.002) and once with a poor conductor (Biot 0.8), and each is compared with the lumped exponential.

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
At Biot 0.002 the two faces and the lumped model agree to within 0.04 K. At Biot 0.8 the back face is still at 50 degrees while the cooled face is at 37, and the lumped value 36.8 describes neither the plate's inside nor its average.

## Fins

A fin is a rod or plate that carries heat away from a surface by conduction along itself and convection from its sides. With `m = sqrt(h P / (k A))`, the heat at the base depends on the tip.

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
  // A pin fin, 4 mm square (P = 16 mm, A = 16 mm^2) of k = 200, 10 cm long, base at 100, air at 20 with h = 25. Adiabatic tip.
  const double h = 25.0, k = 200.0, p = 0.016, a = 1.6e-5, length = 0.1;
  const auto f = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({length}), vec({100.0}), vec({20.0}));
  const double m = std::sqrt(h * p / (k * a)), big_m = std::sqrt(h * p * k * a) * 80.0;
  out.writeln("m =", m, "1/m  m L =", m * length);
  out.writeln("heat from the base", flat(f.heat), "W  closed form M tanh(m L) =", big_m * std::tanh(m * length));
  out.writeln("tip temperature", flat(f.tip), " closed form", 20.0 + 80.0 / std::cosh(m * length));
  out.writeln("efficiency", flat(f.efficiency));
}
```

Output

```text
m = 11.18034 1/m  m L = 1.11803
heat from the base [2.30944] W  closed form M tanh(m L) = 2.30944
tip temperature [67.25680]  closed form 67.25680
efficiency [0.72170]
```
The heat is `M tanh(m L)`, the tip is much cooler than the base, and the efficiency (the heat divided by what an isothermal fin would lose) is 72 %. Three tip conditions at two lengths:

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
  // The three tip conditions for the same fin at two lengths. A convective tip adds the tip area; once the fin is long the three agree.
  const double h = 25.0, k = 200.0, p = 0.016, a = 1.6e-5;
  for (const double length : {0.05, 0.3}) {
    out.writeln("length", length, "m");
    for (const auto tip : {th::Tip::Adiabatic, th::Tip::Convective, th::Tip::Infinite}) {
      const auto f = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({length}), vec({100.0}), vec({20.0}), tip);
      out.writeln(tip == th::Tip::Adiabatic ? "  adiabatic " : tip == th::Tip::Convective ? "  convective" : "  infinite  ", "heat", flat(f.heat), "W  tip", flat(f.tip));
    }
  }
}
```

Output

```text
length 0.05000 m
  adiabatic  heat [1.45183] W  tip [88.94402]
  convective heat [1.47546] W  tip [88.55523]
  infinite   heat [2.86217] W  tip [20.00000]
length 0.30000 m
  adiabatic  heat [2.85519] W  tip [25.58370]
  convective heat [2.85534] W  tip [25.52211]
  infinite   heat [2.86217] W  tip [20.00000]
```
On a short fin the tip matters: a convective tip adds 1.6 % and an infinite fin almost doubles the heat. On a fin of 0.3 m the three agree to within 0.3 %, because the tip is already at nearly the ambient temperature.

Length is the design variable, and it saturates:

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
  // A batch over fin length: the heat saturates and the efficiency falls as the fin gets longer.
  const double h = 25.0, k = 200.0, p = 0.016, a = 1.6e-5;
  const auto f = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({0.01, 0.03, 0.1, 0.3}), vec({100.0}), vec({20.0}));
  out.writeln("heat (W)  ", flat(f.heat));
  out.writeln("efficiency", flat(f.efficiency));
  const auto inf = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({1.0}), vec({100.0}), vec({20.0}), th::Tip::Infinite);
  out.writeln("limit for an infinitely long fin", flat(inf.heat), "W");
}
```

Output

```text
heat (W)   [0.31867, 0.92555, 2.30944, 2.85519]
efficiency [0.99585, 0.96411, 0.72170, 0.29742]
limit for an infinitely long fin [2.86217] W
```
Going from 0.1 m to 0.3 m adds only 24 % more heat while the efficiency drops from 72 % to 30 %: the extra metal is mostly idle.

## A layered wall as a network

`mesh` divides each layer of a wall into cells and returns a ready network whose nodes are the inner surface, the cell centres and the outer surface.

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
  // Two layers: 0.2 m of brick in 3 cells and 0.05 m of insulation in 2 cells, on 1 m^2.
  th::Layers layers;
  layers.cells = {3, 2};
  layers.thickness = vec({0.2, 0.05});
  layers.conductivity = vec({0.8, 0.04});
  layers.density = vec({1800.0, 30.0});
  layers.specific_heat = vec({900.0, 1400.0});
  const th::Slab w = th::mesh(layers, vec({1.0}).reshape({}));
  out.writeln("nodes", w.net.nodes, " inner surface node", w.inner, " outer surface node", w.outer, " cells", w.cells);
  out.writeln("conductances (W/K)", flat(w.net.links.conductance));
  out.writeln("capacities (J/K)  ", flat(w.net.stores.capacity));
  out.writeln("total capacity", w.net.stores.capacity.sum().item<double>(), " closed form", 1800.0 * 900.0 * 0.2 + 30.0 * 1400.0 * 0.05);
}
```

Output

```text
nodes 7  inner surface node 0  outer surface node 6  cells 5
conductances (W/K) [24.00000, 12.00000, 12.00000, 2.82353, 1.60000, 3.20000]
capacities (J/K)   [108000.00000, 108000.00000, 108000.00000, 1050.00000, 1050.00000]
total capacity 326100.00000  closed form 326100.00000
```
Six conductances join seven nodes; the two half-cell links at the faces have double the cell conductance. The capacities sum to the exact `rho c L A` of the layers. The steady flux does not depend on how the layers are divided:

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
  // The steady flux through the meshed wall is the series resistance of its layers, however it is divided.
  const auto flux = [&](std::vector<std::int64_t> cells) {
    th::Layers layers;
    layers.cells = cells;
    layers.thickness = vec({0.2, 0.05});
    layers.conductivity = vec({0.8, 0.04});
    layers.density = vec({1800.0, 30.0});
    layers.specific_heat = vec({900.0, 1400.0});
    th::Slab w = th::mesh(layers, vec({1.0}).reshape({}));
    w.net.fixed = {{w.inner, w.outer}, vec({20.0, 0.0})};
    return flat(th::steady(w.net).link_flow)[0];
  };
  out.writeln("1 + 1 cells", flux({1, 1}), " 3 + 2 cells", flux({3, 2}), " 20 + 10 cells", flux({20, 10}));
  out.writeln("closed form 20 / (0.2 / 0.8 + 0.05 / 0.04) =", 20.0 / (0.2 / 0.8 + 0.05 / 0.04), "W/m^2");
}
```

Output

```text
1 + 1 cells 13.33333  3 + 2 cells 13.33333  20 + 10 cells 13.33333
closed form 20 / (0.2 / 0.8 + 0.05 / 0.04) = 13.33333 W/m^2
```
A transient is where cells matter. A slab at 0 degrees whose faces are suddenly held at 100 and 0 diffuses a temperature profile inward:

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
  // A slab at 0 degrees whose inner face is held at 100: the face-to-centre profile diffuses in. Slab of 0.1 m, alpha = k / (rho c) = 1e-6 m^2/s.
  th::Layers layers;
  layers.cells = {20};
  layers.thickness = vec({0.1});
  layers.conductivity = vec({1.0});
  layers.density = vec({1000.0});
  layers.specific_heat = vec({1000.0});
  th::Slab w = th::mesh(layers, vec({1.0}).reshape({}));
  w.net.fixed = {{w.inner, w.outer}, vec({100.0, 0.0})};
  const auto run = th::tran(w.net, vec(grid(100.0, 101)));
  out.writeln("temperature profile at 10000 s (node 0 is the face at 100, node 21 the face at 0)");
  const std::vector<double> profile = flat(run.temperature.select(0, 100));
  for (int node : {0, 1, 5, 10, 15, 20, 21}) out.writeln("  node", node, profile[node]);
  double series = 0.0;
  for (int n = 1; n < 200; n += 2) series += std::sin(n * M_PI * 0.5) * std::exp(-n * n * M_PI * M_PI * 1e-6 * 1e4 / 0.01) / n;
  out.writeln("series solution at the midplane (x = L/2):", 100.0 * (0.5 - 2.0 / M_PI * series), " mesh node 10 and 11 average", 0.5 * (profile[10] + profile[11]));
}
```

Output

```text
temperature profile at 10000 s (node 0 is the face at 100, node 21 the face at 0)
  node 0 100.00000
  node 1 97.49974
  node 5 77.49783
  node 10 52.49667
  node 15 27.49746
  node 20 2.49974
  node 21 -0.00000
series solution at the midplane (x = L/2): 49.99671  mesh node 10 and 11 average 49.99667
```
Twenty cells reproduce the series solution at the midplane to better than a thousandth of a kelvin.

## Decisions

- **Which resistance:** `wall` for planes, `tube` for pipes, `shell` for spheres, `film` at every surface that touches a fluid.
- **Lump or mesh:** Biot below 0.1 lumps; above it mesh the body. Check the mesh by doubling the cells.
- **How many cells:** more near the faces, where the temperature changes fastest; the steady flux does not care.
- **Fin length:** about `3 / m` is enough; beyond that you pay for metal that does nothing.

## Pitfalls

Inputs that cannot be physical are refused: non-positive conductivities or lengths, outer radii below inner radii, a fin whose base is at the ambient temperature (efficiency would divide by zero), layers without cells.

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
  report([&] { (void)th::wall(vec({0.0}), vec({0.1}), vec({1.0})); });     // zero conductivity
  report([&] { (void)th::wall(vec({1.4}), vec({-0.1}), vec({1.0})); });    // negative thickness
  report([&] { (void)th::wall(vec({1.4}), vec({0.1}), vec({std::nan("")})); }); // area not finite
  report([&] { (void)th::wall(pluss::series::make(std::vector<int>{1}), vec({0.1}), vec({1.0})); }); // not a real floating array
}
```

Output

```text
eng::thermal::wall: conductivity must be positive
eng::thermal::wall: thickness must be positive
eng::thermal::wall: area must be finite
eng::thermal::wall: conductivity must be a defined real floating array
```
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
  report([&] { (void)th::fin(vec({25.0}), vec({200.0}), vec({0.016}), vec({1.6e-5}), vec({0.1}), vec({20.0}), vec({20.0})); }); // base equals ambient
  report([&] { (void)th::fin(vec({25.0}), vec({200.0}), vec({0.016}), vec({1.6e-5}), vec({0.0}), vec({100.0}), vec({20.0})); });  // zero length
  report([&] { (void)th::fin(vec({0.0}), vec({200.0}), vec({0.016}), vec({1.6e-5}), vec({0.1}), vec({100.0}), vec({20.0})); });  // zero coefficient
}
```

Output

```text
eng::thermal::fin: base and ambient temperatures must differ
eng::thermal::fin: length must be positive
eng::thermal::fin: coefficient must be positive
```
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
  th::Layers layers;
  layers.cells = {3, 2};
  layers.thickness = vec({0.2, 0.05});
  layers.conductivity = vec({0.8, 0.04});
  layers.density = vec({1800.0, 30.0});
  layers.specific_heat = vec({900.0, 1400.0});
  report([&] { (void)th::mesh(layers, vec({-1.0}).reshape({})); });   // negative area
  th::Layers few = layers;
  few.cells = {3};
  report([&] { (void)th::mesh(few, vec({1.0}).reshape({})); });       // one cell count for two layers
  th::Layers none = layers;
  none.cells = {3, 0};
  report([&] { (void)th::mesh(none, vec({1.0}).reshape({})); });      // a layer with no cells
  th::Layers thin = layers;
  thin.thickness = vec({0.2, 0.0});
  report([&] { (void)th::mesh(thin, vec({1.0}).reshape({})); });      // zero thickness
}
```

Output

```text
eng::thermal::mesh: area must be positive
eng::thermal::mesh: thickness, conductivity, density, specific_heat and cells must have one entry per layer
eng::thermal::mesh: every layer needs at least one cell
eng::thermal::mesh: thickness must be positive
```
## Where next

Reference pages: [`wall`](../../../../api/eng/thermal/conduction/wall.md), [`tube`](../../../../api/eng/thermal/conduction/tube.md), [`shell`](../../../../api/eng/thermal/conduction/shell.md), [`film`](../../../../api/eng/thermal/conduction/film.md), [`biot`](../../../../api/eng/thermal/conduction/biot.md), [`fin`](../../../../api/eng/thermal/conduction/fin.md), [`Tip`](../../../../api/eng/thermal/conduction/Tip.md), [`mesh`](../../../../api/eng/thermal/conduction/mesh.md), [`Layers`](../../../../api/eng/thermal/conduction/Layers.md), [`Slab`](../../../../api/eng/thermal/conduction/Slab.md). Next chapter: [Thermal 2: Exchangers and Air](../thermal_02_exchangers_and_air.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transient_response.html">
    <span class="label">Previous</span>
    <span class="title">Transient response</span>
  </a>
</div>
<!-- apiref:nav:end -->
