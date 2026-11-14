<!-- generated from pipe_loss_models.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Pipe loss models

[Back to Fluid 1: Pipe Networks](../fluid_01_pipe_networks.md)

Everything the solver knows about a pipe is its head-loss law: how much head the water gives up to friction at a given flow. Choose the wrong law, or the wrong roughness for it, and every head and flow in the network is wrong while the solver still reports convergence. This page explains the two laws, `DarcyWeisbach` and `HazenWilliams`, shows the friction factor passing through the laminar, transition and turbulent ranges and checks each against its closed form, checks the Hazen-Williams law against its formula, shows what roughness and minor losses add, and shows the two unit mistakes that give plausible wrong answers.

## The mental model

For a pipe of length `L` and diameter `D` carrying a flow `q`, the mean velocity is `v = 4 q / (π D²)`. The two laws are

```{math}
h = \left( f \frac{L}{D} + K \right) \frac{v |v|}{2 g}, \qquad
h = \frac{10.67 \, L \, |q|^{0.852} q}{C^{1.852} \, D^{4.8704}} + K \frac{v |v|}{2 g},
```

the first from Darcy and Weisbach, the second from Hazen and Williams. `K` is the pipe's minor-loss coefficient, the loss of fittings in units of velocity head. Both are odd functions of the flow, so a reversed flow loses the same head with the opposite sign. You choose the law with `Options::loss`, for the whole network.

The Darcy law needs a friction factor `f`, which is not a constant. It depends on the Reynolds number `Re = v D / ν` and on the relative roughness `ε / D`:

- **Laminar**, `Re ≤ 2000`: `f = 64 / Re`, independent of roughness.
- **Turbulent**, `Re ≥ 4000`: the Swamee-Jain relation, `f = 0.25 / log10(ε/(3.7 D) + 5.74/Re^0.9)^2`.
- **Transition**, `2000 < Re < 4000`: a cubic interpolation that joins the two values and their slopes, so the friction factor is continuous and the solver's Newton step is well behaved.

The Hazen-Williams law has no friction factor. Its roughness is the dimensionless coefficient `C`, larger for smoother pipes (about `150` for new plastic, `100` for old iron), and it is meant for turbulent water flow at ordinary temperatures.

`roughness` therefore has a different meaning under each law: an absolute roughness in metres for `DarcyWeisbach`, the coefficient `C` for `HazenWilliams`. The Reynolds number is computed from the solved flow, and the friction factor comes back in `SteadyResult::friction_factor`, `0` for every pipe when the law is Hazen-Williams.

## The three flow ranges

A pipe `100` m long and `0.1` m in diameter with a roughness of `1e-4` m is given flows chosen to land at Reynolds numbers from `500` to `100000`. The flows are a batch, one scenario each (see [Scenarios and sensitivities](scenarios_and_sensitivities.md)). The program prints the friction factor the solver used and, where the law has a closed form, the closed form beside it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static const double kPi = 3.14159265358979323846, nu = 1.0038e-6;

// one reservoir and one junction, joined by a pipe; each scenario draws a different flow
static fluid::Net pipe_batch(const std::vector<double> &demands, double D, double L, double rough) {
  const std::int64_t S = static_cast<std::int64_t>(demands.size());
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  std::vector<double> demand, head;
  for (double q : demands) {
    demand.push_back(0.0);
    demand.push_back(q);
    head.push_back(100.0);
    head.push_back(90.0);
  }
  net.nodes.elevation = pluss::series::make(std::vector<double>(2 * S, 0.0), {S, 2});
  net.nodes.demand = pluss::series::make(demand, {S, 2});
  net.nodes.head = pluss::series::make(head, {S, 2});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = pluss::series::make(std::vector<double>(S, L), {S, 1});
  net.pipes.diameter = pluss::series::make(std::vector<double>(S, D), {S, 1});
  net.pipes.roughness = pluss::series::make(std::vector<double>(S, rough), {S, 1});
  net.pipes.minor = pluss::series::make(std::vector<double>(S, 0.0), {S, 1});
  return net;
}

static double swamee_jain(double Re, double D, double eps) {
  const double lg = std::log10(eps / (3.7 * D) + 5.74 / std::pow(Re, 0.9));
  return 0.25 / (lg * lg);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const double D = 0.1, L = 100.0, eps = 1e-4;
  const std::vector<double> reynolds = {500, 1500, 2000, 2001, 3000, 3999, 4000, 8000, 100000};
  std::vector<double> flows;
  for (double Re : reynolds) flows.push_back(Re * kPi * D * nu / 4.0);

  const auto r = fluid::steady(fluid::network(pipe_batch(flows, D, L, eps)));
  const auto f = flat(r.friction_factor), loss = flat(r.head_loss);
  for (std::size_t i = 0; i < reynolds.size(); ++i) {
    const double Re = reynolds[i];
    if (Re <= 2000)
      out.writeln("Re ", Re, " laminar:    friction ", f[i], ", 64/Re = ", 64.0 / Re, ", head loss ", loss[i], " m");
    else if (Re >= 4000)
      out.writeln("Re ", Re, " turbulent:  friction ", f[i], ", Swamee-Jain = ", swamee_jain(Re, D, eps), ", head loss ", loss[i], " m");
    else
      out.writeln("Re ", Re, " transition: friction ", f[i], ", head loss ", loss[i], " m");
  }
}
```

Output

```text
Re 500.000000 laminar:    friction 0.128000, 64/Re = 0.128000, head loss 0.000164 m
Re 1500.000000 laminar:    friction 0.042667, 64/Re = 0.042667, head loss 0.000493 m
Re 2000.000000 laminar:    friction 0.032000, 64/Re = 0.032000, head loss 0.000658 m
Re 2001.000000 transition: friction 0.031984, head loss 0.000658 m
Re 3000.000000 transition: friction 0.033616, head loss 0.001554 m
Re 3999.000000 transition: friction 0.041698, head loss 0.003426 m
Re 4000.000000 turbulent:  friction 0.041695, Swamee-Jain = 0.041695, head loss 0.003427 m
Re 8000.000000 turbulent:  friction 0.034500, Swamee-Jain = 0.034500, head loss 0.011343 m
Re 100000.000000 turbulent:  friction 0.022342, Swamee-Jain = 0.022342, head loss 1.147820 m
```

Below `Re = 2000` the solver's friction factor is `64 / Re` to every digit, and above `4000` it is the Swamee-Jain value. At `Re = 2000` the two sides agree, `0.032000`, and the transition values lie between the laminar and the turbulent branch: `0.033616` at `3000` and `0.041698` at `3999`, against `0.041695` at `4000`. The friction factor and the head loss have no jump anywhere. In the laminar and the turbulent range the factor falls as the flow rises; across the transition it rises, from `0.032` at `Re = 2000` to `0.0417` at `4000`. In the laminar range the head loss is proportional to the flow, `0.000164` m at `500` and `0.000493` m at `1500`, and in the turbulent range it grows more slowly than the square: a flow `12.5` times larger, `Re = 8000` against `100000`, loses `101` times the head, not `156`.

## Hazen-Williams, roughness and minor losses

A reservoir at `100` m, a junction drawing `0.05` m³/s and a kilometre of `0.30` m pipe give a head loss that depends only on the pipe's law and roughness. The program computes it under `HazenWilliams` for four coefficients and checks each against the formula, under `DarcyWeisbach` for four roughnesses, and for three minor-loss coefficients, checked against the base loss plus `K v² / (2 g)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static fluid::Net pipe(double q, double rough, double K) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 0.0});
  net.nodes.demand = vec({0.0, q});
  net.nodes.head = vec({100.0, 90.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({rough});
  net.pipes.minor = vec({K});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const double q = 0.05, D = 0.30, L = 1000.0, g = 9.80665, kPi = 3.14159265358979323846;
  const double v = 4.0 * q / (kPi * D * D);

  fluid::Options hazen;
  hazen.loss = fluid::LossModel::HazenWilliams;
  for (double C : {80.0, 100.0, 130.0, 150.0}) {
    const auto r = fluid::steady(fluid::network(pipe(q, C, 0.0)), hazen);
    const double formula = 10.67 * L * std::pow(q, 1.852) / (std::pow(C, 1.852) * std::pow(D, 4.8704));
    out.writeln("Hazen-Williams C = ", C, ": head loss ", flat(r.head_loss)[0], ", formula ", formula, ", friction factor field ", flat(r.friction_factor)[0]);
  }

  for (double eps : {1e-5, 1e-4, 1e-3, 1e-2}) {
    const auto r = fluid::steady(fluid::network(pipe(q, eps, 0.0)));
    out.writeln("Darcy-Weisbach roughness ", eps, " m: friction ", flat(r.friction_factor)[0], ", head loss ", flat(r.head_loss)[0]);
  }

  const auto base = fluid::steady(fluid::network(pipe(q, 1e-4, 0.0)));
  for (double K : {0.0, 5.0, 20.0}) {
    const auto r = fluid::steady(fluid::network(pipe(q, 1e-4, K)));
    out.writeln("minor loss K = ", K, ": head loss ", flat(r.head_loss)[0], ", base + K v^2/2g = ", flat(base.head_loss)[0] + K * v * v / (2 * g));
  }
}
```

Output

```text
Hazen-Williams C = 80.000000: head loss 4.372833, formula 4.372833, friction factor field 0.000000
Hazen-Williams C = 100.000000: head loss 2.892581, formula 2.892581, friction factor field 0.000000
Hazen-Williams C = 130.000000: head loss 1.779354, formula 1.779354, friction factor field 0.000000
Hazen-Williams C = 150.000000: head loss 1.365100, formula 1.365100, friction factor field 0.000000
Darcy-Weisbach roughness 0.000010 m: friction 0.015677, head loss 1.333124
Darcy-Weisbach roughness 0.000100 m: friction 0.017888, head loss 1.521119
Darcy-Weisbach roughness 0.001000 m: friction 0.027725, head loss 2.357617
Darcy-Weisbach roughness 0.010000 m: friction 0.060021, head loss 5.103949
minor loss K = 0.000000: head loss 1.521119, base + K v^2/2g = 1.521119
minor loss K = 5.000000: head loss 1.648673, base + K v^2/2g = 1.648673
minor loss K = 20.000000: head loss 2.031336, base + K v^2/2g = 2.031336
```

The Hazen-Williams head loss equals the formula to the last printed digit, and its friction factor field is `0`: the model has none to report. A rougher pipe loses more: under the Darcy law the head loss grows from `1.333` m at `1e-5` m to `5.104` m at `1e-2` m, while the same flow in a Hazen-Williams pipe loses `4.373` m at `C = 80` and `1.365` m at `C = 150`. The minor loss adds exactly the velocity head times `K`: `5` adds `0.127554` m and `20` adds `0.510217` m. Minor losses add to the pipe's loss under either law.

## Decisions

- **Darcy-Weisbach or Hazen-Williams.** Use Darcy-Weisbach by default. It is valid at every Reynolds number and for any liquid, and its friction factor responds to temperature and viscosity. Use Hazen-Williams when the data you hold are `C` coefficients, as they are for most distribution-system records, and the flow is turbulent water.
- **Mixed laws.** The law is one setting for the whole network. A network that mixes both kinds of data has to be converted to one: express the roughness of every pipe as an absolute roughness, or as a `C`.
- **Minor losses.** Put the `K` of bends, tees and entrances on the pipe they sit on. A fitting that is not on a pipe, such as a throttling valve, is a link of its own (see [Control valves](../fluid_02_pumps_and_valves/control_valves.md)).

## Pitfalls

Both mistakes below are unit errors, and both produce an answer. The first takes a roughness written for the Darcy law, `1e-4` m, and solves with the Hazen-Williams law, where the same number is read as the coefficient `C`. The second does the opposite and passes `C = 130` to the Darcy law, where it is read as a roughness of `130` m.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static fluid::Net pipe(double q, double rough) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 0.0});
  net.nodes.demand = vec({0.0, q});
  net.nodes.head = vec({100.0, 90.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({rough});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  fluid::Options hazen;
  hazen.loss = fluid::LossModel::HazenWilliams;

  const auto right = fluid::steady(fluid::network(pipe(0.05, 130.0)), hazen);
  const auto wrong = fluid::steady(fluid::network(pipe(0.05, 1e-4)), hazen);
  out.writeln("Hazen-Williams with C = 130:  head loss ", flat(right.head_loss)[0], " m, junction pressure ", flat(right.pressure)[1] / 1000.0, " kPa");
  out.writeln("Hazen-Williams with C = 1e-4: head loss ", flat(wrong.head_loss)[0], " m, junction pressure ", flat(wrong.pressure)[1] / 1000.0, " kPa, converged ", flat(wrong.converged)[0]);

  const auto smooth = fluid::steady(fluid::network(pipe(0.05, 1e-4)));
  const auto huge = fluid::steady(fluid::network(pipe(0.05, 130.0)));
  out.writeln("Darcy-Weisbach with 1e-4 m:   head loss ", flat(smooth.head_loss)[0], " m, friction ", flat(smooth.friction_factor)[0]);
  out.writeln("Darcy-Weisbach with 130 m:    head loss ", flat(huge.head_loss)[0], " m, friction ", flat(huge.friction_factor)[0]);
}
```

Output

```text
Hazen-Williams with C = 130:  head loss 1.779 m, junction pressure 961.482 kPa
Hazen-Williams with C = 1e-4: head loss 374356612254.718 m, junction pressure -3664576138900.008 kPa, converged 1.000
Darcy-Weisbach with 1e-4 m:   head loss 1.521 m, friction 0.018
Darcy-Weisbach with 130 m:    head loss 4.968 m, friction 0.058
```

A coefficient of `1e-4` is accepted, because any positive `C` is, and the solver converges to a head loss of `3.7e11` m: a pipe with essentially infinite resistance, which no pressure could drive a flow through. The error announces itself. The opposite mistake does not: a roughness of `130` m, `430` times the pipe's own diameter, gives a friction factor of `0.058` and a head loss of `4.968` m, a believable number that is `3.3` times the correct `1.521` m. After choosing the law, look at the magnitudes of the roughness column: roughness lengths are a millimetre or smaller, coefficients are tens or hundreds.

Reference: [`Options`](../../../../api/eng/fluid/steady/Options.md), [`LossModel`](../../../../api/eng/fluid/steady/LossModel.md), [`Pipes`](../../../../api/eng/fluid/model/Pipes.md), [`SteadyResult`](../../../../api/eng/fluid/steady/SteadyResult.md), [`Liquid`](../../../../api/eng/fluid/model/Liquid.md).

Where next: [Scenarios and sensitivities](scenarios_and_sensitivities.md) solves many networks at once and differentiates the answer.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="looped_networks.html">
    <span class="label">Previous</span>
    <span class="title">Looped networks</span>
  </a>
  <a class="next-link" href="scenarios_and_sensitivities.html">
    <span class="label">Next</span>
    <span class="title">Scenarios and sensitivities</span>
  </a>
</div>
<!-- apiref:nav:end -->
