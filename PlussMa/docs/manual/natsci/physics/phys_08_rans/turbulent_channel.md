# The turbulent channel

[Back to Physics 8: Reynolds-averaged turbulence](../phys_08_rans.md)

A large-eddy simulation resolves the big eddies; for most engineering flows even that is too expensive. A Reynolds-averaged model solves only for the mean flow and models all of the turbulence as one eddy viscosity found from an extra transport equation. This page solves the mean flow in a turbulent channel with the two models the library provides, and checks the answer against the law of the wall and against the exact momentum balance.

## How it works

In a channel driven by a pressure gradient the mean flow depends on the distance from the wall only, and the mean momentum equation integrates to an exact statement about the total stress. With lengths scaled by the half height and velocities by the friction velocity {math}`u_\tau`, the viscosity is {math}`1/\mathrm{Re}_\tau` and the driving pressure gradient is one, so

```{math}
(\nu + \nu_t)\,\frac{\mathrm{d}U}{\mathrm{d}y} = 1 - y.
```

The eddy viscosity {math}`\nu_t` comes from a model. The library provides **Spalart-Allmaras** (`rans::Model::SpalartAllmaras`), which transports one variable {math}`\tilde\nu`, and **k-omega SST** (`rans::Model::KOmegaSST`), which transports the turbulent kinetic energy {math}`k` and the specific dissipation {math}`\omega`. Near the wall the velocity follows the law of the wall: {math}`U^+ = y^+` in the viscous sublayer ({math}`y^+ \lesssim 5`) and

```{math}
\begin{aligned}
&U^+ = \frac{1}{\kappa} \ln y^+ + B, \\
&\kappa = 0.41,\ B \approx 5.2
\end{aligned}
```

in the logarithmic layer ({math}`y^+ \gtrsim 30`), with {math}`y^+ = y\,\mathrm{Re}_\tau`.

`rans::solve_channel(options)` marches the model equations in pseudo-time, on a mesh clustered at the wall, until the velocity stops changing. `rans::ChannelOptions` has `model`, `friction_reynolds`, `cells`, `stretching` (the clustering at the wall), `max_iterations` and `tolerance`. The result carries the cell-centre height `y`, the mean `velocity`, the `eddy_viscosity`, the `total_stress`, the transported variables of the model (`nu_tilde`, or `k` and `omega`; those of the other model are left undefined), and `converged`, `iterations` and `residual`.

## Spalart-Allmaras at Re_tau = 1000

The program solves the channel with the default options and prints the velocity and the eddy viscosity, in units of the molecular viscosity, at six distances from the wall, next to the law of the wall.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace rans = natsci::physics::fluids::rans;

// the value of a profile at wall distance y+, interpolated between the two cells that surround it
static double at_yplus(const pluss::Series &y, const pluss::Series &f, double re_tau, double yplus) {
  const auto yv = y.host<double>(), fv = f.host<double>();
  for (std::size_t i = 1; i < yv.size(); ++i)
    if (yv[i] * re_tau >= yplus) {
      const double t = (yplus - yv[i - 1] * re_tau) / ((yv[i] - yv[i - 1]) * re_tau);
      return fv[i - 1] + t * (fv[i] - fv[i - 1]);
    }
  return fv.back();
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  rans::ChannelOptions options;  // Spalart-Allmaras at Re_tau = 1000 on 160 cells, with the cells clustered at the wall
  const auto result = rans::solve_channel(options);
  out.writeln("converged: ", result.converged ? "yes" : "no", " after ", result.iterations, " iterations");
  out.writeln("residual of the velocity change per iteration ", result.residual, PrintOptions{}.sci(2).with_sep(""));
  out.writeln("  y+      U+     nu_t/nu    log law");
  for (double yp : {2.0, 5.0, 30.0, 100.0, 300.0, 800.0}) {
    const double u = at_yplus(result.y, result.velocity, 1000.0, yp);
    const double nut = at_yplus(result.y, result.eddy_viscosity, 1000.0, yp) * 1000.0;
    out.writeln(yp, "   ", u, "   ", nut, "   ", yp < 11.0 ? yp : std::log(yp) / 0.41 + 5.2);
  }
  out.writeln("velocity at the centreline (U+) ", result.velocity.reshape({result.velocity.numel()}).select(0, options.cells - 1).item<double>());
}
```

Output

```text
converged: yes after 2201 iterations
residual of the velocity change per iteration 9.99e-11
  y+      U+     nu_t/nu    log law
2.000   1.997   0.001   2.000
5.000   4.940   0.048   5.000
30.000   13.406   9.749   13.496
100.000   16.436   34.618   16.432
300.000   19.358   74.094   19.112
800.000   21.896   93.758   21.504
velocity at the centreline (U+) 22.110
```

The run converged in 2201 iterations to a velocity change of `9.99e-11` per iteration. In the viscous sublayer the velocity is the wall law, `1.997` at `y+ = 2` and `4.940` at `y+ = 5`, with an eddy viscosity of only `0.001` and `0.048` times the molecular one. In the logarithmic layer the computed `16.436` at `y+ = 100` agrees with the log law `16.432`, and `19.358` at `y+ = 300` with `19.112`. The eddy viscosity rises to `34.6` times the molecular one at `y+ = 100` and `93.8` at `y+ = 800`, where a mixing-length estimate {math}`\kappa y^+ (1 - y)` gives `36.9` at `y+ = 100`. The velocity at the centreline is `22.110`, above the log law because the wake adds to it.

## Spalart-Allmaras and k-omega SST

The two models are calibrated for the same flows and should agree on a flat-plate-like profile. The program runs both at `Re_tau = 1000` and prints the velocity and eddy viscosity at five heights, and what each model transports.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace rans = natsci::physics::fluids::rans;

// the value of a profile at wall distance y+, interpolated between the two cells that surround it
static double at_yplus(const pluss::Series &y, const pluss::Series &f, double re_tau, double yplus) {
  const auto yv = y.host<double>(), fv = f.host<double>();
  for (std::size_t i = 1; i < yv.size(); ++i)
    if (yv[i] * re_tau >= yplus) {
      const double t = (yplus - yv[i - 1] * re_tau) / ((yv[i] - yv[i - 1]) * re_tau);
      return fv[i - 1] + t * (fv[i] - fv[i - 1]);
    }
  return fv.back();
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const double re_tau = 1000.0;
  out.writeln("   y+     SA: U+   nu_t/nu    SST: U+   nu_t/nu");
  rans::ChannelOptions sa, sst;
  sst.model = rans::Model::KOmegaSST;
  const auto a = rans::solve_channel(sa), b = rans::solve_channel(sst);
  for (double yp : {5.0, 30.0, 100.0, 300.0, 800.0})
    out.writeln(yp, "   ", at_yplus(a.y, a.velocity, re_tau, yp), "   ", at_yplus(a.y, a.eddy_viscosity, re_tau, yp) * re_tau, "      ", at_yplus(b.y, b.velocity, re_tau, yp), "   ", at_yplus(b.y, b.eddy_viscosity, re_tau, yp) * re_tau);
  out.writeln("centreline U+: SA ", a.velocity.select(0, sa.cells - 1).item<double>(), ", SST ", b.velocity.select(0, sst.cells - 1).item<double>());
  out.writeln("iterations: SA ", a.iterations, ", SST ", b.iterations, "; the SST run also carries k and omega, SA carries nu~:");
  out.writeln("  SA has nu~: ", a.nu_tilde.defined() ? "yes" : "no", ", k: ", a.k.defined() ? "yes" : "no", "; SST has nu~: ", b.nu_tilde.defined() ? "yes" : "no", ", k: ", b.k.defined() ? "yes" : "no", ", omega: ", b.omega.defined() ? "yes" : "no");
}
```

Output

```text
   y+     SA: U+   nu_t/nu    SST: U+   nu_t/nu
5.000   4.940   0.048      4.895   0.111
30.000   13.406   9.749      12.719   7.505
100.000   16.436   34.618      16.438   30.353
300.000   19.358   74.094      19.475   78.649
800.000   21.896   93.758      21.599   136.355
centreline U+: SA 22.110, SST 21.742
iterations: SA 2201, SST 3115; the SST run also carries k and omega, SA carries nu~:
  SA has nu~: yes, k: no; SST has nu~: no, k: yes, omega: yes
```

In the logarithmic layer they agree: `16.436` and `16.438` at `y+ = 100`, `19.358` and `19.475` at `y+ = 300`. They differ near the wall, where the eddy viscosity at `y+ = 5` is `0.048` for Spalart-Allmaras and `0.111` for SST, and the velocity at `y+ = 30` is `13.406` against `12.719`, a spread of about 5%, which is typical of how two models disagree in the buffer layer. The centreline values are `22.110` and `21.742`. SST needs more iterations (`3115` against `2201`) because it solves for two variables. The last two lines show that each result carries only the variables of its own model.

## The exact stress balance

Whatever the model, the total stress must equal {math}`1 - y`. The program prints the computed total stress {math}`(\nu + \nu_t) \mathrm{d}U/\mathrm{d}y` at five heights next to it, the largest departure over the lower 150 cells, and the share of the stress that the model carries at four heights above the wall.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace rans = natsci::physics::fluids::rans;

// the value of a profile at wall distance y+, interpolated between the two cells that surround it
static double at_yplus(const pluss::Series &y, const pluss::Series &f, double re_tau, double yplus) {
  const auto yv = y.host<double>(), fv = f.host<double>();
  for (std::size_t i = 1; i < yv.size(); ++i)
    if (yv[i] * re_tau >= yplus) {
      const double t = (yplus - yv[i - 1] * re_tau) / ((yv[i] - yv[i - 1]) * re_tau);
      return fv[i - 1] + t * (fv[i] - fv[i - 1]);
    }
  return fv.back();
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // in a channel driven by a unit pressure gradient the total stress is exactly 1 - y, whatever the model: (nu + nu_t) dU/dy = 1 - y
  rans::ChannelOptions options;
  const auto r = rans::solve_channel(options);
  const auto exact = r.y.mul(-1.0).add(1.0);
  out.writeln("   y        total stress   1 - y");
  for (std::int64_t i : {10, 40, 80, 120, 150})
    out.writeln(r.y.select(0, i).item<double>(), "   ", r.total_stress.select(0, i).item<double>(), "        ", exact.select(0, i).item<double>());
  out.writeln("largest departure from 1 - y over the lower 150 cells: ", r.total_stress.sub(exact).abs().slice(0, 0, 150).max().item<double>());
  // the viscosity makes up the whole stress at the wall and nearly none of it by mid-channel: the share of the stress carried by the eddy viscosity
  out.writeln("share of the stress carried by the model at y+ = 5, 30, 100, 500:");
  for (double yp : {5.0, 30.0, 100.0, 500.0}) {
    const double nut_ratio = at_yplus(r.y, r.eddy_viscosity, 1000.0, yp) * 1000.0;
    out.writeln("  y+ = ", yp, ": ", nut_ratio / (1.0 + nut_ratio));
  }
}
```

Output

```text
   y        total stress   1 - y
0.0024   0.9975        0.9976
0.0175   0.9831        0.9825
0.0921   0.9080        0.9079
0.3674   0.6327        0.6326
0.8229   0.1771        0.1771
largest departure from 1 - y over the lower 150 cells: 0.0017
share of the stress carried by the model at y+ = 5, 30, 100, 500:
  y+ = 5.0000: 0.0454
  y+ = 30.0000: 0.9070
  y+ = 100.0000: 0.9719
  y+ = 500.0000: 0.9891
```

The computed stress `0.9975`, `0.9831`, `0.9080`, `0.6327` and `0.1771` matches {math}`1 - y` (`0.9976`, `0.9825`, `0.9079`, `0.6326`, `0.1771`) to a few parts in a thousand, and the largest departure over the lower 150 cells is `0.0017`. This is a check you can make on any channel run: it holds for any model, so a departure is a discretisation or convergence error, not a modelling one. The last four lines show where the model matters: at `y+ = 5` the eddy viscosity carries `4.5%` of the stress, at `y+ = 30` `90.7%`, and from `y+ = 100` outwards nearly all of it, `97%` and `99%`. The viscous sublayer is the only part of the channel in which the molecular viscosity counts.

## Decisions

- Use **Spalart-Allmaras** as the economical choice for attached wall-bounded flows: one transported variable, fewer iterations, and a good record on boundary layers.
- Use **SST** when separation or an adverse pressure gradient matters, or when a freestream value of {math}`k` and {math}`\omega` is part of the problem. It blends a near-wall model with a free-stream one and is the standard for separated flows.
- Raise `cells` and `stretching` together: the first cell must sit inside the viscous sublayer, {math}`y^+ \lesssim 1`, for a wall-resolved solution. The cell-centre heights are in `result.y`, so you can read off the `y+` of the first cell as `result.y[0] * friction_reynolds` and check it.
- Always check the total-stress balance and the convergence flag; neither costs anything and both catch a wrong run.

## Pitfalls

A run that is cut short does not fail; it reports that it has not converged. The program runs the same channel for 100 iterations, then tries a mesh that is too coarse and a Reynolds number of 0.5.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace rans = natsci::physics::fluids::rans;

// the value of a profile at wall distance y+, interpolated between the two cells that surround it
static double at_yplus(const pluss::Series &y, const pluss::Series &f, double re_tau, double yplus) {
  const auto yv = y.host<double>(), fv = f.host<double>();
  for (std::size_t i = 1; i < yv.size(); ++i)
    if (yv[i] * re_tau >= yplus) {
      const double t = (yplus - yv[i - 1] * re_tau) / ((yv[i] - yv[i - 1]) * re_tau);
      return fv[i - 1] + t * (fv[i] - fv[i - 1]);
    }
  return fv.back();
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // 1. a run that is stopped too early reports that it has not converged, with the profile it had reached: check `converged` before trusting the numbers
  rans::ChannelOptions short_run;
  short_run.max_iterations = 100;
  const auto early = rans::solve_channel(short_run);
  out.writeln("100 iterations: converged = ", early.converged ? "yes" : "no", ", centreline U+ ", early.velocity.select(0, short_run.cells - 1).item<double>());
  out.writeln("   the velocity was still changing per iteration by ", early.residual, PrintOptions{}.sci(2).with_sep(""));
  // 2. the problem is checked before it starts
  rans::ChannelOptions coarse;
  coarse.cells = 8;
  try {
    (void)rans::solve_channel(coarse);
  } catch (const pluss::Error &e) {
    out.writeln("8 cells: ", e.message());
  }
  rans::ChannelOptions laminar;
  laminar.friction_reynolds = 0.5;
  try {
    (void)rans::solve_channel(laminar);
  } catch (const pluss::Error &e) {
    out.writeln("Re_tau = 0.5: ", e.message());
  }
}
```

Output

```text
100 iterations: converged = no, centreline U+ 22.119
   the velocity was still changing per iteration by 3.43e-06
8 cells: natsci::fluids::rans::solve_channel: needs cells >= 16, Re_tau > 1 and positive step limits
Re_tau = 0.5: natsci::fluids::rans::solve_channel: needs cells >= 16, Re_tau > 1 and positive step limits
```

After 100 iterations `converged` is `no`, and the velocity is still changing by `3.43e-06` per iteration. The centreline velocity `22.119` is close to the converged `22.110`, but you cannot tell that without the converged run: the flag exists so that you don't have to guess. A mesh with fewer than 16 cells, or a friction Reynolds number of 1 or less, is refused with one message that names the limits: at least 16 cells, `Re_tau > 1` and positive step limits.

## Where next

The next page, [Coupling a model to the flow solver](coupled_model.md), carries the model variables next to a general two- or three-dimensional flow. To resolve the large eddies instead of modelling them, read [Physics 7](../phys_07_les.md). For compressible flow with viscosity, see [Physics 6](../phys_06_gas_dynamics.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="coupled_model.html">
    <span class="label">Next</span>
    <span class="title">Coupling a model to the flow solver</span>
  </a>
</div>
<!-- apiref:nav:end -->
