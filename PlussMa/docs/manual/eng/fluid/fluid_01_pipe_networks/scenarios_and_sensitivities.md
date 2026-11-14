<!-- generated from scenarios_and_sensitivities.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Scenarios and sensitivities

[Back to Fluid 1: Pipe Networks](../fluid_01_pipe_networks.md)

A planner never solves a network once. They solve it for the peak hour and the night, for a pipe replaced by a larger one, for a hundred random demand patterns, and then ask which pipe to enlarge. This page shows how to solve a whole set of those networks in one call, by giving the arrays a leading scenario axis, and how to ask the solver for the derivative of any head with respect to any demand or any pipe diameter, so that the question "which pipe?" has a numerical answer.

## The mental model

Every physical array of the description ends in its node axis or its pipe axis, and may have any number of leading axes in front. Those leading axes are **scenarios**. All the arrays that have them must have the same shape in front, and an array without them is shared by every scenario, so a network whose pipes are fixed and whose demands vary needs a leading axis on the demands alone. The result arrays have the same leading axes: a `head` of shape `[scenario, node]`, a `flow` of shape `[scenario, link]`, a `converged` of shape `[scenario]`. Every scenario is solved in the same Newton iteration, and each is solved as if it were alone.

The same arrays can be **tracked**. Mark an input with `tracks_slope_(true)` before the solve, call `reverse()` on a scalar function of the result, and the sensitivity of that scalar to every tracked input is stored on the input as `sensitivity()`. The solver differentiates through the Newton iteration to the converged solution, so the derivative is the exact derivative of the solved network, not a finite difference of repeated solves.

## Four demand scenarios in one call

The looped network of the previous page is solved with its demands scaled by `0.5`, `1.0`, `1.5` and `2.0`. Each array of the description carries a leading axis of length four. The program prints the shape of the results, the head and pressure at the far junction in each scenario, and, as a check, solves each scenario on its own and compares.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the looped network with a demand multiplier per scenario: every array carries a leading scenario axis
static fluid::Net looped(const std::vector<double> &scale, double d0 = 0.30) {
  const std::int64_t S = static_cast<std::int64_t>(scale.size());
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction,
                    fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  const double base[5] = {0.0, 0.010, 0.015, 0.012, 0.008};
  const double elevation[5] = {0.0, 10.0, 12.0, 8.0, 15.0};
  std::vector<double> demand, head, elev;
  for (double s : scale)
    for (int n = 0; n < 5; ++n) {
      demand.push_back(base[n] * s);
      head.push_back(n == 0 ? 60.0 : 55.0);
      elev.push_back(elevation[n]);
    }
  net.nodes.elevation = pluss::series::make(elev, {S, 5});
  net.nodes.demand = pluss::series::make(demand, {S, 5});
  net.nodes.head = pluss::series::make(head, {S, 5});
  net.pipes.from = {0, 1, 1, 2, 3, 2};
  net.pipes.to = {1, 2, 3, 4, 4, 3};
  const double length[6] = {800, 500, 600, 450, 550, 300};
  const double diameter[6] = {d0, 0.20, 0.20, 0.15, 0.15, 0.10};
  std::vector<double> L, D, R, K;
  for (std::int64_t s = 0; s < S; ++s)
    for (int p = 0; p < 6; ++p) {
      L.push_back(length[p]);
      D.push_back(diameter[p]);
      R.push_back(1e-4);
      K.push_back(0.0);
    }
  net.pipes.length = pluss::series::make(L, {S, 6});
  net.pipes.diameter = pluss::series::make(D, {S, 6});
  net.pipes.roughness = pluss::series::make(R, {S, 6});
  net.pipes.minor = pluss::series::make(K, {S, 6});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const std::vector<double> scale = {0.5, 1.0, 1.5, 2.0};
  const auto r = fluid::steady(fluid::network(looped(scale)));
  out.writeln("head shape ", r.head.sizes(), ", flow shape ", r.flow.sizes(), ", converged shape ", r.converged.sizes());
  const auto head = flat(r.head), pressure = flat(r.pressure);
  for (std::size_t s = 0; s < scale.size(); ++s)
    out.writeln("demand x", scale[s], ": head at junction 4 ", head[s * 5 + 4], " m, pressure ", pressure[s * 5 + 4] / 1000.0, " kPa");
  for (std::size_t s = 0; s < scale.size(); ++s) {
    const auto alone = fluid::steady(fluid::network(looped({scale[s]})));
    out.writeln("scenario ", s, " solved alone: ", flat(alone.head)[4], ", difference ", std::abs(flat(alone.head)[4] - head[s * 5 + 4]));
  }
}
```

Output

```text
head shape [4, 5], flow shape [4, 6], converged shape [4]
demand x0.500000: head at junction 4 59.425413 m, pressure 434.880281 kPa
demand x1.000000: head at junction 4 57.917692 m, pressure 420.121206 kPa
demand x1.500000: head at junction 4 55.522258 m, pressure 396.672301 kPa
demand x2.000000: head at junction 4 52.250448 m, pressure 364.644561 kPa
scenario 0 solved alone: 59.425413, difference 0.000000
scenario 1 solved alone: 57.917692, difference 0.000000
scenario 2 solved alone: 55.522258, difference 0.000000
scenario 3 solved alone: 52.250448, difference 0.000000
```

The results have a leading axis of four: `head` is `[4, 5]`, `flow` is `[4, 6]` and `converged` is `[4]`. Doubling the demand takes the far junction from `59.43` m to `52.25` m and its pressure from `434.9` kPa to `364.6` kPa, and each scenario agrees with its own stand-alone solve exactly. A scenario axis is a convenience and a speed-up, never a change of answer.

## Which pipe, which demand: sensitivities

`steady` is differentiable. The program marks the demands and the pipe diameters as tracked, solves the base network, and asks for the derivative of the head at junction `4` with respect to all of them at once. It then checks each derivative with a central finite difference, which solves the network twice more per entry.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static fluid::Net looped(double d0 = 0.30) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction,
                    fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = pluss::series::make(std::vector<double>{0.0, 10.0, 12.0, 8.0, 15.0});
  net.nodes.demand = pluss::series::make(std::vector<double>{0.0, 0.010, 0.015, 0.012, 0.008});
  net.nodes.head = pluss::series::make(std::vector<double>{60.0, 55.0, 55.0, 55.0, 55.0});
  net.pipes.from = {0, 1, 1, 2, 3, 2};
  net.pipes.to = {1, 2, 3, 4, 4, 3};
  net.pipes.length = pluss::series::make(std::vector<double>{800.0, 500.0, 600.0, 450.0, 550.0, 300.0});
  net.pipes.diameter = pluss::series::make(std::vector<double>{d0, 0.20, 0.20, 0.15, 0.15, 0.10});
  net.pipes.roughness = pluss::series::make(std::vector<double>(6, 1e-4));
  net.pipes.minor = pluss::series::make(std::vector<double>(6, 0.0));
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));

  auto net = looped();
  net.nodes.demand.tracks_slope_(true);
  net.pipes.diameter.tracks_slope_(true);
  const auto solved = fluid::steady(fluid::network(net));
  solved.head.select(-1, 4).reverse();
  out.writeln("d head4 / d demand   ", flat(net.nodes.demand.sensitivity()), "  (m per m3/s)");
  out.writeln("d head4 / d diameter ", flat(net.pipes.diameter.sensitivity()), "  (m per m)");

  // central finite differences, one demand at a time
  const double step = 1e-7;
  std::vector<double> numeric;
  for (int node = 1; node < 5; ++node) {
    auto up = looped(), down = looped();
    auto plus = flat(up.nodes.demand), minus = flat(down.nodes.demand);
    plus[node] += step;
    minus[node] -= step;
    up.nodes.demand = pluss::series::make(plus);
    down.nodes.demand = pluss::series::make(minus);
    numeric.push_back((flat(fluid::steady(fluid::network(up)).head)[4] - flat(fluid::steady(fluid::network(down)).head)[4]) / (2 * step));
  }
  out.writeln("finite differences for the four demands ", numeric);
  const auto wider = fluid::steady(fluid::network(looped(0.30 + 1e-6)));
  const auto narrower = fluid::steady(fluid::network(looped(0.30 - 1e-6)));
  out.writeln("finite difference for the first diameter ", (flat(wider.head)[4] - flat(narrower.head)[4]) / 2e-6);
}
```

Output

```text
d head4 / d demand   [0.000000, -41.841689, -89.080924, -88.045557, -137.204157]  (m per m3/s)
d head4 / d diameter [16.611408, 11.603627, 10.130873, 3.512418, 3.459978, 0.016247]  (m per m)
finite differences for the four demands [-41.841689, -89.080924, -88.045557, -137.204157]
finite difference for the first diameter 16.611408
```

The derivative with respect to a demand is negative everywhere: more draw at any junction lowers the head at junction `4`. The entry for junction `1` is `-41.84` and the entry for junction `4` itself is `-137.20` m per m³/s, so a litre per second more at junction `4` costs `0.137` m of head there. The derivative with respect to a diameter is positive, and its size ranks the pipes: widening the supply pipe `0` raises the head at junction `4` by `16.61` m per metre of diameter, ahead of pipe `1` with `11.60` and pipe `2` with `10.13`, the two small pipes `3` and `4` contribute about `3.5` each, and the cross-link `5` contributes `0.016`, almost nothing. Each number that was checked agrees with its finite difference to the printed digits. The derivatives came from one solve and one reverse sweep; a finite difference needs two more solves for every entry.

## Decisions

- **Scenario axis or a loop.** Use a leading axis when the scenarios share the topology: it solves them in one Newton iteration and keeps them on the device. Use a loop only when the topologies differ, since `Network` has one topology.
- **Which arrays vary.** An array without leading axes is shared. Give a leading axis only to those that change; every array that has one must have the same leading shape.
- **When to differentiate.** Ask for sensitivities when the question is "which input matters" or when an optimiser needs a gradient. For a single question about one input, one scenario axis of perturbed values is as cheap.
- **Precision.** Use `Float64` for gradients that you compare to finite differences; `Float32` gradients are good to single precision.

## Pitfalls

The mistake is to give two arrays leading axes of different lengths. The scenarios cannot be paired, and the description is refused rather than broadcast.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

int main() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = pluss::series::make(std::vector<double>{0.0, 5.0});
  // three demand scenarios, but two head scenarios
  net.nodes.demand = pluss::series::make(std::vector<double>{0.0, 0.01, 0.0, 0.02, 0.0, 0.03}, {3, 2});
  net.nodes.head = pluss::series::make(std::vector<double>{50.0, 49.0, 50.0, 49.0}, {2, 2});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = pluss::series::make(std::vector<double>{1000.0});
  net.pipes.diameter = pluss::series::make(std::vector<double>{0.30});
  net.pipes.roughness = pluss::series::make(std::vector<double>{1e-4});
  net.pipes.minor = pluss::series::make(std::vector<double>{0.0});
  try {
    (void)fluid::network(net);
  } catch (const std::exception &e) {
    std::cout << "three demand scenarios, two head scenarios: " << e.what() << "\n";
  }
}
```

Output

```text
three demand scenarios, two head scenarios: eng::fluid::network: non-scalar batches must have equal leading shapes
```

The compiler names the problem; give the shared arrays no leading axis, or the same one.

Reference: [`network`](../../../../api/eng/fluid/model/network_fn.md), [`steady`](../../../../api/eng/fluid/steady/steady.md), [`Net`](../../../../api/eng/fluid/model/Net.md), [`SteadyResult`](../../../../api/eng/fluid/steady/SteadyResult.md).

Where next: [Fluid 2: Pumps and Valves](../fluid_02_pumps_and_valves.md) adds the devices that put energy into the water and take it out.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pipe_loss_models.html">
    <span class="label">Previous</span>
    <span class="title">Pipe loss models</span>
  </a>
</div>
<!-- apiref:nav:end -->
