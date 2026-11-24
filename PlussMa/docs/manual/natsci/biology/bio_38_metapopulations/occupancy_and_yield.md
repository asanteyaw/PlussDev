# Occupancy and yield

[Back to Biology 38: Metapopulations and Harvest](../bio_38_metapopulations.md)

Two questions about a population living in many places share one structure. A species lives in patches that are colonised and go extinct, and you want to know whether it persists in the landscape and how many patches it holds. A stock is harvested, and you want to know how much you can take for ever and what happens if you take too much. This page computes the equilibria and the thresholds of both, for patches that are alike and for patches that differ, and for a harvest set by effort and by quota.

## How it works

The code is in `pluss::ma::natsci::biology::population::ecology` (`ecology::` below). For identical patches the fraction occupied {math}`p` obeys Levins' equation

```{math}
\frac{\mathrm{d}p}{\mathrm{d}t} = c\,p(1-p) - e\,p
```

with colonisation rate {math}`c` and extinction rate {math}`e`. `ecology::levins_rate(p, c, e, rescue)` is the right-hand side (with `rescue = true` the extinction rate falls as more patches are occupied, {math}`e\,p(1-p)`) and `ecology::levins_equilibrium(c, e, like)` returns {math}`1 - e/c`, or zero when {math}`e \ge c`. For patches that differ, `ecology::patch_occupancy_rate(p, connectivity, e)` lets patch {math}`j` seed patch {math}`i` at rate {math}`\mathrm{connectivity}_{ij}`, and `ecology::metapopulation_capacity(connectivity)` returns the largest eigenvalue of the connectivity matrix: the species persists when it exceeds the extinction rate {math}`e`. For a logistic stock with growth rate {math}`r` and carrying capacity {math}`K`, harvest at fixed effort {math}`E` with catchability {math}`q` is {math}`\dot N = rN(1 - N/K) - qEN`, whose equilibrium and yield are `ecology::effort_equilibrium(r, K, q, E, like)` and `ecology::effort_yield(r, K, q, E, like)`; the largest sustainable yield is `ecology::maximum_sustainable_yield(r, K, like)`, {math}`rK/4`. A fixed quota {math}`h` is `ecology::constant_yield_rate(n, r, K, h)` with the equilibria `ecology::constant_yield_equilibria(r, K, h, like)`, a result holding `stable`, `unstable` and `exists`. The last argument `like` only gives the shape and type of the result.

## Patches that are alike

The program fixes the colonisation rate at `0.4`, simulates the fraction of occupied patches from `5%` for three extinction rates, and compares with the formula. It then repeats two cases with the rescue effect.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace dynamics = natsci::common::dynamics;
namespace ecology = biology::population::ecology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // Levins: the fraction p of occupied patches grows by colonisation c p (1 - p) and shrinks by extinction e p
  const double c = 0.4;
  for (double e : {0.1, 0.3, 0.5}) {
    const auto rate = [&](const pluss::Series &p) { return ecology::levins_rate(p, c, e); };
    const auto settled = dynamics::simulate(rate, vec({0.05}), 0.0, 200.0, 4000);
    out.writeln("extinction ", e, ": formula 1 - e/c = ", flat(ecology::levins_equilibrium(c, e, vec({0.0})))[0], ", simulated ", flat(settled)[0]);
  }
  // with the rescue effect extinction falls as more patches are occupied (rate e p (1 - p)): the net rate is (c - e) p (1 - p), so the metapopulation fills up or dies out
  for (double e : {0.3, 0.5}) {
    const auto rescue = [&](const pluss::Series &p) { return ecology::levins_rate(p, c, e, true); };
    out.writeln("rescue effect, extinction ", e, ": p after 200 time units ", flat(dynamics::simulate(rescue, vec({0.05}), 0.0, 200.0, 4000))[0]);
  }
}
```

Output

```text
extinction 0.1000: formula 1 - e/c = 0.7500, simulated 0.7500
extinction 0.3000: formula 1 - e/c = 0.2500, simulated 0.2500
extinction 0.5000: formula 1 - e/c = 0.0000, simulated 0.0000
rescue effect, extinction 0.3000: p after 200 time units 1.0000
rescue effect, extinction 0.5000: p after 200 time units 0.0000
```

The simulation settles exactly at the formula {math}`1 - e/c`: `0.7500` for {math}`e = 0.1`, `0.2500` for {math}`e = 0.3` and `0.0000` for {math}`e = 0.5`, where extinction exceeds colonisation and the species cannot persist. With the rescue effect the net rate is {math}`(c - e)p(1-p)` and there is no intermediate level: for {math}`e = 0.3` the species spreads to every patch (`1.0000`) and for {math}`e = 0.5` it dies out (`0.0000`).

## Patches that differ

Four patches on a line, three of them close together and the fourth far from the others, are connected by the matrix in the program. It prints the metapopulation capacity and follows the occupancy of all four patches for three extinction rates.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace dynamics = natsci::common::dynamics;
namespace ecology = biology::population::ecology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // four patches on a line; connectivity[i][j] says how strongly occupied patch j seeds patch i. Patch 3 is isolated from the rest
  const auto near = mat({{0.0, 0.8, 0.2, 0.0}, {0.8, 0.0, 0.8, 0.0}, {0.2, 0.8, 0.0, 0.05}, {0.0, 0.0, 0.05, 0.0}});
  const auto capacity = ecology::metapopulation_capacity(near);
  out.writeln("metapopulation capacity (largest eigenvalue) = ", flat(capacity)[0]);
  for (double e : {0.6, 1.2, 2.0}) {
    const auto rate = [&](const pluss::Series &p) { return ecology::patch_occupancy_rate(p, near, e); };
    out.writeln("extinction ", e, ": occupancy after 300 time units ", flat(dynamics::simulate(rate, vec({0.5, 0.5, 0.5, 0.5}), 0.0, 300.0, 6000)));
  }
}
```

Output

```text
metapopulation capacity (largest eigenvalue) = 1.2363
extinction 0.6000: occupancy after 300 time units [0.4747, 0.5589, 0.4755, 0.0381]
extinction 1.2000: occupancy after 300 time units [0.0260, 0.0335, 0.0260, 0.0011]
extinction 2.0000: occupancy after 300 time units [0.0000, 0.0000, 0.0000, 0.0000]
```

The capacity is `1.2363`. For {math}`e = 0.6` and {math}`e = 1.2`, both below it, the species persists, but the isolated patch is occupied much less (`0.0381` and `0.0011`) than the patches in the cluster (`0.4747`, `0.5589`, `0.4755` and `0.0260`, `0.0335`, `0.0260`); for {math}`e = 2.0` above the capacity everything is lost. A patch that is poorly connected is a poor place to live even when the metapopulation as a whole persists.

## Fishing with a fixed effort

A logistic stock with {math}`r = 0.5`, {math}`K = 100` and catchability {math}`q = 0.01` is fished with five efforts. The program prints the equilibrium stock and the yield for each.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace dynamics = natsci::common::dynamics;
namespace ecology = biology::population::ecology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a logistic stock (r = 0.5, K = 100) fished with effort E at catchability q = 0.01: dN/dt = r N (1 - N/K) - q E N
  const double r = 0.5, K = 100.0, q = 0.01;
  out.writeln("maximum sustainable yield r K / 4 = ", flat(ecology::maximum_sustainable_yield(r, K, vec({0.0})))[0]);
  for (double effort : {5.0, 12.5, 25.0, 40.0, 60.0}) {
    const auto n = ecology::effort_equilibrium(r, K, q, effort, vec({0.0}));
    const auto y = ecology::effort_yield(r, K, q, effort, vec({0.0}));
    out.writeln("effort ", effort, ": stock ", flat(n)[0], ", yield ", flat(y)[0]);
  }
}
```

Output

```text
maximum sustainable yield r K / 4 = 12.5000
effort 5.0000: stock 90.0000, yield 4.5000
effort 12.5000: stock 75.0000, yield 9.3750
effort 25.0000: stock 50.0000, yield 12.5000
effort 40.0000: stock 20.0000, yield 8.0000
effort 60.0000: stock -20.0000, yield -12.0000
```

The yield rises with effort until it reaches the maximum sustainable yield {math}`rK/4 = 12.5` at the effort {math}`r/(2q) = 25`, where the stock has been halved to `50`. More effort gives less, because the stock falls faster than the effort rises: `8.0000` at effort `40` with a stock of `20`. At effort `60`, beyond {math}`r/q = 50`, the formulas return a negative stock and yield: that is the signal that no positive equilibrium exists and the stock is driven to zero.

## Decisions

- Use `levins_equilibrium` for a landscape of similar patches and `metapopulation_capacity` as soon as patches differ in size or isolation; the capacity is the single number to compare with the extinction rate.
- To manage a stock, regulate effort rather than a fixed quota where you can: effort has a stable equilibrium at every level below {math}`r/q`, while a quota has the cliff shown below.
- Treat the harvest equilibria with the `exists` flag of `constant_yield_equilibria` and with the sign of `effort_equilibrium` before reading any other number.

## Pitfalls

A fixed quota looks safe because it is below the maximum, but it can still empty the stock. The program computes the equilibria for four quotas and follows two stocks under a quota of `10`, one just above and one just below the unstable equilibrium.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace dynamics = natsci::common::dynamics;
namespace ecology = biology::population::ecology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a fixed quota h instead of a fixed effort: dN/dt = r N (1 - N/K) - h. Two equilibria exist only while h <= r K / 4 = 12.5
  const double r = 0.5, K = 100.0;
  for (double h : {5.0, 10.0, 12.5, 15.0}) {
    const auto eq = ecology::constant_yield_equilibria(r, K, h, vec({0.0}));
    out.writeln("quota ", h, ": exists ", flat(eq.exists)[0], ", stable stock ", flat(eq.stable)[0], ", unstable stock ", flat(eq.unstable)[0]);
  }
  // a population started below the unstable equilibrium (27.6393) collapses even though the quota is below the maximum
  const auto rate = [&](const pluss::Series &n) { return ecology::constant_yield_rate(n, r, K, 10.0); };
  for (double n0 : {30.0, 27.0}) {
    out.writeln("quota 10, start ", n0, ": stock at times 5, 10, 14 = ", flat(dynamics::simulate(rate, vec({n0}), 0.0, 5.0, 500))[0], ", ",
                flat(dynamics::simulate(rate, vec({n0}), 0.0, 10.0, 1000))[0], ", ", flat(dynamics::simulate(rate, vec({n0}), 0.0, 14.0, 1400))[0]);
  }
}
```

Output

```text
quota 5.0000: exists 1.0000, stable stock 88.7298, unstable stock 11.2702
quota 10.0000: exists 1.0000, stable stock 72.3607, unstable stock 27.6393
quota 12.5000: exists 1.0000, stable stock 50.0000, unstable stock 50.0000
quota 15.0000: exists 0.0000, stable stock 50.0000, unstable stock 50.0000
quota 10, start 30.0000: stock at times 5, 10, 14 = 34.1524, 42.9662, 52.7060
quota 10, start 27.0000: stock at times 5, 10, 14 = 25.6244, 20.8460, 6.3465
```

For quotas of `5` and `10` there are two equilibria, a stable one (`88.7298` and `72.3607`) and an unstable one (`11.2702` and `27.6393`). At the maximum `12.5` they merge at `50`, and for a quota of `15` there is none: `exists` is `0.0000` and the two numbers printed are placeholders that you must not use. A stock of `30` under the quota of `10` recovers (`34.1524`, `42.9662`, `52.7060` at times `5`, `10` and `14`), while one of `27`, just below the unstable equilibrium `27.6393`, collapses (`25.6244`, `20.8460`, `6.3465`). Fishing at the same rate as before can turn a recoverable stock into a lost one.

The effort formula does not stop at zero. The program asks for the equilibrium at an effort beyond {math}`r/q`, and for a quota above the maximum.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace dynamics = natsci::common::dynamics;
namespace ecology = biology::population::ecology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // fishing harder than r/q leaves a negative stock; asking for an unsustainable quota has no equilibrium to report
  const double r = 0.5, K = 100.0, q = 0.01;
  out.writeln("effort 40 (r/q = 50): stock ", flat(ecology::effort_equilibrium(r, K, q, 40.0, vec({0.0})))[0]);
  out.writeln("effort 80 (r/q = 50): stock ", flat(ecology::effort_equilibrium(r, K, q, 80.0, vec({0.0})))[0], ", yield ", flat(ecology::effort_yield(r, K, q, 80.0, vec({0.0})))[0]);
  const auto eq = ecology::constant_yield_equilibria(r, K, 20.0, vec({0.0}));
  out.writeln("quota 20 > r K / 4: exists ", flat(eq.exists)[0], ", stable stock ", flat(eq.stable)[0], ", unstable stock ", flat(eq.unstable)[0]);
}
```

Output

```text
effort 40 (r/q = 50): stock 20.0000
effort 80 (r/q = 50): stock -60.0000, yield -48.0000
quota 20 > r K / 4: exists 0.0000, stable stock 50.0000, unstable stock 50.0000
```

At effort `40` the stock is `20.0000`; at `80`, beyond {math}`r/q = 50`, the formula returns `-60.0000` and a yield of `-48.0000`, which are not populations but a signal that the stock is extinct. A quota of `20` exceeds {math}`rK/4 = 12.5`, and again `exists` is `0.0000`.

## Where next

[Biology 1: Growth](../bio_01_growth.md) gives the logistic growth under the harvest, [Biology 37: Host-Parasitoid Maps](../bio_37_parasitoids.md) puts populations in patches linked by dispersal, and [Biology 3: Demography](../bio_03_demography.md) shows how to ask about extinction risk for a single population.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
