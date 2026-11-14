<!-- generated from pid_controllers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# PID controllers

[Back to Control 3: Classical Design](../control_03_classical_design.md)

When you can measure only the output, and want a controller you can tune by hand, the proportional-integral-derivative law is the standard choice. `control::pid` builds one as an object that holds its three gains and a derivative filter, and `control::freqresp` evaluates its frequency response, so that you can combine it with a plant and examine the loop. This page covers the continuous and the discrete forms, the three rules for discretising the integral and the derivative, and the checks that refuse an unstable discrete derivative.

## The mental model

The controller acts on the error `e = r - y` between a reference and the measurement. In the **parallel form** used here its transfer function is

```{math}
C(s) = K_p + \frac{K_i}{s} + \frac{K_d\,s}{T_f\,s + 1},
```

with the proportional gain `Kp`, the integral gain `Ki`, the derivative gain `Kd`, and the derivative filter time constant `Tf` (the `filter` argument). The integral term removes steady-state error; the derivative term anticipates; the filter `Tf` rolls the derivative off above `1 / Tf` so that it does not amplify measurement noise without limit. A zero gain switches its term off, and `Tf = 0` is an unfiltered derivative.

A discrete controller with sample time `T` replaces `1/s` in the integral and in the filtered derivative by a discrete operator, chosen by the **rule**:

| Rule | Replaces `1/s` by | Behaviour |
|---|---|---|
| `Forward` | `T / (z - 1)` | simple; the derivative needs `T < 2 Tf` to be stable |
| `Backward` | `z T / (z - 1)` | always stable; the integral leads the exact one in phase |
| `Trapezoid` | `(T/2) (z + 1) / (z - 1)` | matches the exact integral's phase; an unfiltered trapezoidal derivative is unstable |

`pid(kp, ki, kd, filter, sample_time, int_rule, diff_rule)` returns a `Pid` that stores the gains, the sample time (`0` for continuous) and the two rules. The gains may carry batch axes, and the response then has the same leading axes.

## The response of a continuous PID

With `Kp = 2`, `Ki = 1`, `Kd = 0.5` and `Tf = 0.1`, the response at `w` is `2 + 1/(j w) + 0.5 j w / (0.1 j w + 1)`. The program evaluates it at three frequencies and prints the gain next to the analytic value. At low frequency the integral term dominates (large gain), and at high frequency the filtered derivative adds a constant `Kd / Tf = 5` to the proportional `2`, so the gain tends to `7`; it is `5.1` at `w = 10`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto controller = ctl::pid(vec({2.0}), vec({1.0}), vec({0.5}), vec({0.1}));
  const std::vector<double> frequencies = {0.1, 1.0, 10.0};

  out.writeln("Kp", controller.kp().reshape({1}).items<double>(), " Ki", controller.ki().reshape({1}).items<double>(),
              " Kd", controller.kd().reshape({1}).items<double>(), " filter", controller.filter().reshape({1}).items<double>());
  out.writeln("continuous", controller.is_cont(), " discrete", controller.is_disc(), " sample time", controller.sample_time());
  out.writeln("gain        ", ctl::freqresp(controller, vec(frequencies)).abs().reshape({3}).items<double>());

  std::vector<double> analytic;
  for (const double w : frequencies) {
    const std::complex<double> s(0.0, w);
    analytic.push_back(std::abs(2.0 + 1.0 / s + 0.5 * s / (0.1 * s + 1.0)));
  }
  out.writeln("analytic    ", analytic);
}
```

Output

```text
Kp [2.00000]  Ki [1.00000]  Kd [0.50000]  filter [0.10000]
continuous true  discrete false  sample time 0.00000
gain         [10.14912, 2.11079, 5.10000]
analytic     [10.14912, 2.11079, 5.10000]
```

## The discretisation rules

A pure integrator `Ki = 1` is exactly `1 / (j w)`: gain `1/w` and phase `-90` degrees. The program discretises it at `T = 0.1` with each rule and evaluates at `w = 1, 5, 20`. `Trapezoid` has exactly `-90` degrees at every frequency (it is the bilinear transform); `Forward` and `Backward` lag and lead it by a few degrees at low frequency and far more near the sampling limit. The gains of all three are close to `1/w` (`1.0`, `0.2`, `0.05`), drifting at the highest frequency.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <utility>
#include <vector>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const pluss::Series omega = vec({1.0, 5.0, 20.0});
  const std::vector<std::pair<const char *, ctl::PidRule>> rules = {{"Forward  ", ctl::PidRule::Forward}, {"Backward ", ctl::PidRule::Backward}, {"Trapezoid", ctl::PidRule::Trapezoid}};
  out.writeln("exact 1/w: gain 1, 0.2, 0.05; phase -90 degrees");
  for (const auto &[name, rule] : rules) {
    const auto controller = ctl::pid(vec({0.0}), vec({1.0}), vec({0.0}), vec({0.0}), 0.1, rule, ctl::PidRule::Backward);
    const pluss::Series response = ctl::freqresp(controller, omega).reshape({3});
    out.writeln(name, "gain", response.abs().items<double>(), " phase (degrees)", (response.angle() * (180.0 / M_PI)).items<double>());
  }
}
```

Output

```text
exact 1/w: gain 1, 0.2, 0.05; phase -90 degrees
Forward   gain [1.00042, 0.20210, 0.05942]  phase (degrees) [-92.86479, -104.32394, -147.29578]
Backward  gain [1.00042, 0.20210, 0.05942]  phase (degrees) [-87.13521, -75.67606, -32.70422]
Trapezoid gain [0.99917, 0.19582, 0.03210]  phase (degrees) [-90.00000, -90.00000, -90.00000]
```

At `w = 1` the forward rule is off by `2.9` degrees (`-92.86`) and the backward rule by the same in the other direction (`-87.14`). At `w = 20`, about two thirds of the way to the Nyquist frequency of `31.4`, the forward phase has drifted to `-147` degrees, and the backward one to `-33`. The trapezoidal rule's phase is exact; its gain is off by `0.08` percent at `w = 1` and drops more at `w = 20` (`0.0321` against `0.05`).

## The derivative filter

The filter decides how much high-frequency gain the derivative gives. The program evaluates the controller `Kp = 2`, `Kd = 0.5` over four decades of frequency for `Tf = 0.1`, `0.01` and `0.001`. A smaller `Tf` lets the derivative act further up the frequency axis, which is what amplifies noise.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series omega = vec({1.0, 10.0, 100.0, 1000.0});
  out.writeln("w                  ", omega.items<double>());
  for (const double tf : {0.1, 0.01, 0.001}) {
    const auto controller = ctl::pid(vec({2.0}), vec({0.0}), vec({0.5}), vec({tf}));
    out.writeln("Kp 2, Kd 0.5, Tf", tf, ctl::freqresp(controller, omega).abs().reshape({4}).items<double>());
  }
}
```

Output

```text
w                   [1.0000, 10.0000, 100.0000, 1000.0000]
Kp 2, Kd 0.5, Tf 0.1000 [2.1084, 5.1478, 6.9681, 6.9997]
Kp 2, Kd 0.5, Tf 0.0100 [2.0664, 5.5437, 36.7967, 51.7423]
Kp 2, Kd 0.5, Tf 0.0010 [2.0620, 5.4035, 49.9905, 354.9704]
```

With `Tf = 0.1` the gain levels off at `2 + 0.5 / 0.1 = 7` (`6.9997` at `w = 1000`); with `Tf = 0.01` it is heading for `52` (`51.74` at `w = 1000`); with `Tf = 0.001` it is still climbing towards `502` (`354.97` at `w = 1000`). The high-frequency gain of the controller is the factor by which sensor noise is amplified, so a derivative filter is chosen from the noise, not from the plant.

## A batch of controllers

The gains are Series, so a vector of values is a batch of controllers, and the frequency response has the batch as its leading axis. The program builds three controllers (P, PI, PID) in one object and evaluates them at two frequencies.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto family = ctl::pid(vec({1.0, 2.0, 3.0}), vec({0.0, 1.0, 1.0}), vec({0.0, 0.0, 0.1}), vec({0.0, 0.0, 0.01}));
  const pluss::Series response = ctl::freqresp(family, vec({1.0, 10.0}));
  out.writeln("response shape", response.sizes());
  out.writeln("gain at w = 1 and w = 10: P  ", response.abs().select(0, 0).items<double>());
  out.writeln("                          PI ", response.abs().select(0, 1).items<double>());
  out.writeln("                          PID", response.abs().select(0, 2).items<double>());
}
```

Output

```text
response shape [3, 2]
gain at w = 1 and w = 10: P   [1.00000, 1.00000]
                          PI  [2.23607, 2.00250]
                          PID [3.13305, 3.22430]
```

## Errors

A discrete controller with a derivative is checked for the stability of the derivative operator at construction. The program shows an unfiltered trapezoidal derivative, a forward derivative whose filter is shorter than half the sample time, and a negative filter.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  report([&] { (void)ctl::pid(vec({2.0}), vec({1.0}), vec({0.5}), vec({0.0}), 0.05, ctl::PidRule::Forward, ctl::PidRule::Trapezoid); });
  report([&] { (void)ctl::pid(vec({2.0}), vec({1.0}), vec({0.5}), vec({0.01}), 0.05, ctl::PidRule::Forward, ctl::PidRule::Forward); });
  report([&] { (void)ctl::pid(vec({2.0}), vec({1.0}), vec({0.5}), vec({-0.1})); });
  report([&] { (void)ctl::pid(vec({2.0}), vec({1.0}), vec({0.5}), vec({0.1}), -1.0); });
}
```

Output

```text
eng::control::pid: an unfiltered trapezoidal derivative is unstable
eng::control::pid: forward derivative requires sample_time < 2*filter
eng::control::pid: filter must be non-negative
eng::control::pid: sample_time must be finite and non-negative
```

## Decisions

- **Rule for a discrete integral:** `Backward` or `Trapezoid`. `Forward` is the simplest but lags; prefer it only to match an existing implementation.
- **Rule for a discrete derivative:** `Backward` is always stable and is the usual choice with a filter. `Forward` needs `T < 2 Tf`.
- **Starting values:** begin with `P` alone, add `I` to remove steady error, add `D` only if the response needs damping and the sensor is clean enough; evaluate the loop with the [margins](loop_analysis_and_margins.md) after each change.
- **A derivative filter:** `Tf` about one tenth of the derivative time `Kd / Kp`, then adjust from the noise.

## Pitfalls

The program makes three mistakes with a PID object: giving the integral time where a gain is expected, misreading the unit of the derivative filter, and judging a loop from the controller alone.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static double gain(const ctl::Pid &c, double w) { return ctl::freqresp(c, vec({w})).abs().reshape({1}).items<double>()[0]; }
static double one(const pluss::Series &s) { return s.reshape({1}).items<double>()[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto none = pluss::series::zeros({0}, f64);

  // 1. Ki is a gain: the textbook form Kp (1 + 1 / (Ti s)) with Kp = 2 and Ti = 4 has Ki = Kp / Ti = 0.5
  const auto textbook = ctl::pid(vec({2.0}), vec({0.5}), vec({0.0}), vec({0.0}));
  const auto mistaken = ctl::pid(vec({2.0}), vec({4.0}), vec({0.0}), vec({0.0}));
  out.writeln("gain at w = 0.1: Ki = Kp / Ti = 0.5 gives ", gain(textbook, 0.1), "   Ki = Ti = 4 gives ", gain(mistaken, 0.1), "   (the textbook value is |2 (1 - 2.5j)| = ", std::abs(std::complex<double>(2.0, -5.0)), ")");

  // 2. the derivative filter is a time constant in the units of the plant: 0.1 s is not 0.1 ms
  for (const double tau : {0.1, 1e-4}) {
    const auto c = ctl::pid(vec({2.0}), vec({0.0}), vec({0.5}), vec({tau}));
    out.writeln("filter ", tau, ": gain at w = 10, 1000, 100000: ", std::vector<double>{gain(c, 10.0), gain(c, 1000.0), gain(c, 100000.0)}, "   high-frequency limit Kp + Kd / filter = ", 2.0 + 0.5 / tau);
  }

  // 3. a Pid is not a loop: its gain says nothing about the margins until it is multiplied by the plant
  const auto plant = lti::zpk(none, vec({-1.0, -1.0, -1.0}), vec({1.0}));
  const pluss::Series omega = pluss::series::linspace(0.01, 10.0, 2000, f64);
  for (const double kp : {1.0, 2.0, 6.0}) {
    const auto c = ctl::pid(vec({kp}), vec({0.0}), vec({0.0}), vec({0.0}));
    const pluss::Series loop = ctl::freqresp(c, omega).reshape({2000}) * lti::freqresp(plant, omega).reshape({2000});
    const auto m = ctl::margin(loop, omega);
    out.writeln("Kp = ", kp, ": the controller's gain is ", gain(c, 1.0), " at every frequency;  loop gain margin ", one(m.gain), ", phase margin ", one(m.phase), " degrees");
  }
}
```

Output

```text
gain at w = 0.1: Ki = Kp / Ti = 0.5 gives 5.3852   Ki = Ti = 4 gives 40.0500   (the textbook value is |2 (1 - 2.5j)| = 5.3852)
filter 0.1000: gain at w = 10, 1000, 100000: [5.1478, 6.9997, 7.0000]   high-frequency limit Kp + Kd / filter = 7.0000
filter 0.0001: gain at w = 10, 1000, 100000: [5.3870, 497.7216, 4977.1760]   high-frequency limit Kp + Kd / filter = 5002.0000
Kp = 1.0000: the controller's gain is 1.0000 at every frequency;  loop gain margin 8.0001, phase margin inf degrees
Kp = 2.0000: the controller's gain is 2.0000 at every frequency;  loop gain margin 4.0000, phase margin 67.5989 degrees
Kp = 6.0000: the controller's gain is 6.0000 at every frequency;  loop gain margin 1.3333, phase margin 10.1675 degrees
```

A textbook controller `Kp (1 + 1 / (Ti s))` with `Kp = 2` and `Ti = 4` has `Ki = Kp / Ti = 0.5`, and its gain at `w = 0.1` is `5.3852`, the magnitude of `2 (1 - 2.5j)`. Giving `Ki = Ti = 4` is a controller eight times as strong in its integral action, with a gain of `40.05`. The filter is a time constant in the units of the plant: `0.1` bounds the high-frequency gain at `Kp + Kd / filter = 7`, and a value meant as `0.1` milliseconds in a plant measured in seconds, `1e-4`, gives `5002`, a controller that amplifies noise a thousand times. And a `Pid` is only parameters and a frequency response: its gain is `1`, `2` or `6` at every frequency for a proportional controller, and says nothing about the loop. Multiplied by the plant `1 / (s + 1)^3`, the three loops have gain margins of `8.0001`, `4.0000` and `1.3333` and phase margins of `inf`, `67.5989` and `10.1675` degrees; combine the response with the plant's and read the margins, as on the [loop analysis](loop_analysis_and_margins.md) page.

Reference: [`pid`](../../../../api/eng/control/controllers/pid.md), [`freqresp`](../../../../api/eng/control/controllers/freqresp.md), [`PidRule`](../../../../api/eng/control/controllers/PidRule.md).

Where next: [Loop analysis and margins](loop_analysis_and_margins.md) combines a controller with a plant and measures how far the loop is from instability.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pole_placement.html">
    <span class="label">Previous</span>
    <span class="title">Pole placement</span>
  </a>
  <a class="next-link" href="loop_analysis_and_margins.html">
    <span class="label">Next</span>
    <span class="title">Loop analysis and margins</span>
  </a>
</div>
<!-- apiref:nav:end -->
