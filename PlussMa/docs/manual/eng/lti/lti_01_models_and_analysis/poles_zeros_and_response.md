<!-- generated from poles_zeros_and_response.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Poles, zeros and frequency response

[Back to Systems 1: Models and Analysis](../lti_01_models_and_analysis.md)

A state-space model describes the inside of a system. For many questions you want to know what it does from the outside: how it treats a sinusoid of each frequency, and whether it is stable. `zpk` describes a single-input, single-output system by its **zeros, poles and gain**; `freqresp` evaluates the frequency response of either description; `PoleZero::is_stable` tests stability; and `to_sos` turns a discrete pole-zero model into the section matrix that [`sos`](../../signal/signal_02_recursive_filters/second_order_sections.md) filters with.

## The mental model

The transfer function of a single-input, single-output system factors into a gain and the roots of two polynomials:

```{math}
H(s) = k\,\frac{(s-z_1)(s-z_2)\cdots}{(s-p_1)(s-p_2)\cdots},
```

with `s` the Laplace variable for a continuous system and `z` the shift variable for a discrete one. The **poles** `p` decide stability and how fast the system responds: a continuous system is stable when every pole has a negative real part, a discrete one when every pole lies inside the unit circle. The **zeros** `z` shape the response between the poles. The **frequency response** is the transfer function evaluated along the imaginary axis, `s = j w`, for a continuous system, and around the unit circle, `z = e^{j w T}`, for a discrete one with sample time `T`. Its magnitude is the gain at the frequency `w` (radians per time unit) and its angle the phase.

A `PoleZero` stores the three parts as `zeros()`, `poles()` and `gain()`, always as complex numbers, with a sample time like `StateSpace`. A state-space model and a pole-zero model of the same system have the same frequency response; `freqresp` accepts both.

## Poles, zeros and gain

The spring of the previous pages has transfer function `1 / (s^2 + 3 s + 2) = 1 / ((s + 1)(s + 2))`: no zeros, poles at `-1` and `-2`, and gain `1`. The program builds it, reads back the parts and tests stability.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
using Complex = std::complex<double>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto spring = lti::zpk(pluss::series::zeros({0}, f64), pluss::series::make(std::vector<double>{-1.0, -2.0}),
                               pluss::series::make(std::vector<double>{1.0}));
  out.writeln("zeros", spring.zeros().sizes(), " poles", spring.poles().items<Complex>(), " gain", spring.gain().item<Complex>());
  out.writeln("continuous", spring.is_cont(), " stable", spring.is_stable(), " element type", spring.poles().dtype().name);
}
```

Output

```text
zeros [0]  poles [(-1,0), (-2,0)]  gain (1,0)
continuous true  stable true  element type complex128
```

The zeros are an empty vector, and the roots and gain are stored as complex numbers even though these are real. A system with no poles at all is trivially stable.

## Stability

The rule depends on the sample time. The poles `0.5` and `1.5` are unstable in both settings: `1.5` has a positive real part, and lies outside the unit circle. The poles `0.5` and `-0.9` differ: `0.5` has a positive real part, so the continuous system is unstable, but both lie inside the unit circle, so the discrete system is stable. The program prints all four cases, and then the two boundary cases.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

int main() {
  OutputEngine out;
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series none = pluss::series::zeros({0}, f64);
  const pluss::Series one = pluss::series::make(std::vector<double>{1.0});
  const pluss::Series first = pluss::series::make(std::vector<double>{0.5, 1.5});
  const pluss::Series second = pluss::series::make(std::vector<double>{0.5, -0.9});

  out.writeln("poles 0.5 and 1.5:  continuous", lti::zpk(none, first, one).is_stable(), " discrete", lti::zpk(none, first, one, 1.0).is_stable());
  out.writeln("poles 0.5 and -0.9: continuous", lti::zpk(none, second, one).is_stable(), " discrete", lti::zpk(none, second, one, 1.0).is_stable());

  const pluss::Series on_the_boundary = pluss::series::make(std::vector<double>{0.0, -1.0});
  const pluss::Series on_the_circle = pluss::series::make(std::vector<double>{1.0, 0.5});
  out.writeln("a pole at 0 (continuous):", lti::zpk(none, on_the_boundary, one).is_stable(), "  a pole at 1 (discrete):", lti::zpk(none, on_the_circle, one, 1.0).is_stable());
}
```

Output

```text
poles 0.5 and 1.5:  continuous false  discrete false
poles 0.5 and -0.9: continuous false  discrete true
a pole at 0 (continuous): false   a pole at 1 (discrete): false
```

A pole exactly on the stability boundary (`0` real part, or magnitude `1`) is reported as not stable: the response neither decays nor is it guaranteed to stay bounded.

## Frequency response

`freqresp(system, omega)` evaluates the response at the frequencies in `omega`, a real vector of angular frequencies. For a `PoleZero` the result is a complex vector with one entry per frequency. At `w = 1` the spring has `H = 1 / ((j + 1)(j + 2))`, of magnitude `1/sqrt(10) = 0.316228`. The program evaluates three frequencies and prints the gain and the phase.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto spring = lti::zpk(pluss::series::zeros({0}, f64), pluss::series::make(std::vector<double>{-1.0, -2.0}),
                               pluss::series::make(std::vector<double>{1.0}));
  const pluss::Series omega = pluss::series::make(std::vector<double>{0.1, 1.0, 10.0});
  const pluss::Series response = lti::freqresp(spring, omega);

  out.writeln("shape", response.sizes());
  out.writeln("gain ", response.abs().items<double>());
  out.writeln("phase", response.angle().items<double>());
  out.writeln("analytic gain at w = 1:", std::abs(1.0 / ((std::complex<double>(0, 1) + 1.0) * (std::complex<double>(0, 1) + 2.0))));
}
```

Output

```text
shape [3]
gain  [0.496898, 0.316228, 0.009757]
phase [-0.149627, -1.249046, -2.844528]
analytic gain at w = 1: 0.316228
```

The gain is `0.4969` at `w = 0.1` (close to the DC gain `1/2`), `0.316228` at `w = 1` and falls to `0.0098` at `w = 10`; the phase swings from nearly zero to `-2.84` radians, towards the `-pi` of a two-pole system.

## The same system as a state-space model

For a state-space model the response is `C (j w I - A)^-1 B + D`, a matrix for every frequency: the result has shape `[outputs, inputs, frequencies]`. The spring written as state matrices has the same single-input, single-output response, so the shape is `[1, 1, 3]` and the gains agree with the pole-zero model to the printed digits.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto model = lti::state(matrix({0, 1, -2, -3}, 2, 2), matrix({0, 1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1));
  const auto factored = lti::zpk(pluss::series::zeros({0}, f64), pluss::series::make(std::vector<double>{-1.0, -2.0}),
                                 pluss::series::make(std::vector<double>{1.0}));
  const pluss::Series omega = pluss::series::make(std::vector<double>{0.1, 1.0, 10.0});

  const pluss::Series from_state = lti::freqresp(model, omega);
  out.writeln("state-space response shape", from_state.sizes());
  out.writeln("gain from the state model", from_state.abs().reshape({3}).items<double>());
  out.writeln("gain from the pole-zero model", lti::freqresp(factored, omega).abs().items<double>());
  out.writeln("largest difference between the two responses:", (from_state.reshape({3}) - lti::freqresp(factored, omega)).abs().max().item<double>());
}
```

Output

```text
state-space response shape [1, 1, 3]
gain from the state model [0.496898, 0.316228, 0.009757]
gain from the pole-zero model [0.496898, 0.316228, 0.009757]
largest difference between the two responses: 0.000000
```

With several inputs and outputs the middle axes hold the matrix response, and a batch of systems adds leading axes in front.

## Discrete systems

For a discrete model the frequency `w` is in radians per time unit, and the response is taken at `z = e^{j w T}`, so a sample time of `T` scales the axis. The first-order discrete system `x[k+1] = 0.5 x[k] + u[k]`, `y = x`, has `H(z) = 1 / (z - 0.5)`: gain `2` at `w = 0`, and at `w = 1` with `T = 1` the magnitude `1 / |e^{j} - 0.5| = 1.187034`. The program evaluates three frequencies and the highest representable one, `w = pi / T`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto discrete = lti::state(scalar(0.5), scalar(1.0), scalar(1.0), scalar(0.0), 1.0);
  const pluss::Series omega = pluss::series::make(std::vector<double>{0.0, 1.0, M_PI});
  out.writeln("gain at w = 0, 1, pi:", lti::freqresp(discrete, omega).abs().reshape({3}).items<double>());
  out.writeln("analytic             :", std::vector<double>{2.0, 1.0 / std::abs(std::exp(std::complex<double>(0, 1)) - 0.5), 1.0 / 1.5});
}
```

Output

```text
gain at w = 0, 1, pi: [2.000000, 1.187034, 0.666667]
analytic             : [2.000000, 1.187034, 0.666667]
```

At the Nyquist frequency `w = pi / T` the gain is `1 / (-1 - 0.5)` in magnitude, `0.666667`: the system attenuates the fastest oscillation it can represent by a factor of three relative to DC.

## From poles and zeros to filter sections

A discrete pole-zero model with one zero for each pole is a filter. `to_sos` factors it into second-order sections, in the `[b0 b1 b2 a0 a1 a2]` rows that `sig::sos` runs. The program converts `H(z) = 0.25 (z + 1) / (z - 0.5)`, which has a zero at `-1`, a pole at `0.5` and gain `0.25`: the single section has numerator `0.25 (1 + z^-1)` and denominator `1 - 0.5 z^-1`, padded with a zero to second order.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto smoother = lti::zpk(pluss::series::make(std::vector<double>{-1.0}), pluss::series::make(std::vector<double>{0.5}),
                                 pluss::series::make(std::vector<double>{0.25}), 1.0);
  const pluss::Series sections = lti::to_sos(smoother);
  out.writeln("sections shape", sections.sizes());
  out.writeln("[b0 b1 b2 a0 a1 a2]", sections.reshape({6}).items<double>());
}
```

Output

```text
sections shape [1, 6]
[b0 b1 b2 a0 a1 a2] [0.2500, 0.2500, 0.0000, 1.0000, -0.5000, 0.0000]
```

Only discrete models can be turned into sections, because the sections are in terms of delays, and the model must list one zero for each pole (add zeros at the origin to make up the count).

## Errors

The program shows more zeros than poles, a gain that is not one number, a continuous model passed to `to_sos`, and a sample time that is negative.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

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
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series none = pluss::series::zeros({0}, f64);
  const pluss::Series one = pluss::series::make(std::vector<double>{1.0});
  const pluss::Series pole = pluss::series::make(std::vector<double>{-1.0});
  report([&] { (void)lti::zpk(pluss::series::make(std::vector<double>{1.0, 2.0, 3.0}), pole, one); });
  report([&] { (void)lti::zpk(none, pole, pluss::series::make(std::vector<double>{1.0, 2.0})); });
  report([&] { (void)lti::zpk(none, pole, one, -1.0); });
  report([&] { (void)lti::to_sos(lti::zpk(none, pole, one)); });
  report([&] { (void)lti::freqresp(lti::zpk(none, pole, one), pluss::series::zeros({0}, f64)); });
}
```

Output

```text
eng::lti::zpk: a proper system cannot have more zeros than poles
eng::lti::zpk: gain must contain one value
eng::lti::zpk: sample_time must be finite and non-negative
eng::lti::to_sos: only discrete systems can be realized by the delay-form SOS filter
eng::lti::freqresp: omega must be a non-empty vector
```

## Decisions

- **You have the physics or a model with several inputs and outputs:** a state-space model. It is the form that simulation and the matrix tools use.
- **You have a specification of poles and zeros, or want a filter:** `zpk`. Keep it factored until the last step.
- **Checking the response of either:** `freqresp`, the same call for both.
- **Stability of a pole-zero model:** `is_stable`. For a state-space model, check that the free response decays in a simulation, or that the gramian exists.

## Pitfalls

The program makes four mistakes: passing hertz where radians per time unit are expected, evaluating an integrator at zero frequency, reading a discrete response outside one period, and building a complex pole without its conjugate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
using Complex = std::complex<double>;

static std::vector<double> gain(const pluss::Series &s) { return s.abs().reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto none = pluss::series::zeros({0}, f64);
  const auto one = pluss::series::make(std::vector<double>{1.0});
  const double pi = std::acos(-1.0);

  // 1. omega is in radians per time unit: a lag with a corner at 1 rad/s is at 0.159 Hz
  const auto lag = lti::zpk(none, pluss::series::make(std::vector<double>{-1.0}), one);
  out.writeln("gain at omega = 1 (the corner):   ", gain(lti::freqresp(lag, pluss::series::make(std::vector<double>{1.0}))), "   1/sqrt(2) = ", 1.0 / std::sqrt(2.0));
  out.writeln("gain at 1 Hz, omega = 2 pi:       ", gain(lti::freqresp(lag, pluss::series::make(std::vector<double>{2.0 * pi}))), "   passing 1 as if it were hertz gives the first line");

  // 2. a pole at the origin has infinite gain at omega = 0
  const auto integrator = lti::zpk(none, pluss::series::make(std::vector<double>{0.0}), one);
  out.writeln("integrator gain at omega = 0, 0.1, 1:", gain(lti::freqresp(integrator, pluss::series::make(std::vector<double>{0.0, 0.1, 1.0}))));

  // 3. a discrete response repeats every 2 pi / T
  const auto smoother = lti::zpk(none, pluss::series::make(std::vector<double>{0.5}), one, 1.0);
  out.writeln("discrete gain at 0.5 pi and 0.5 pi + 2 pi:", gain(lti::freqresp(smoother, pluss::series::make(std::vector<double>{0.5 * pi, 2.5 * pi}))));
  out.writeln("on [0, pi]: ", gain(lti::freqresp(smoother, pluss::series::make(std::vector<double>{0.0, 0.25 * pi, 0.5 * pi, 0.75 * pi, pi}))));

  // 4. half of a conjugate pair is not a real system
  const Complex j(0.0, 1.0);
  const auto pair = lti::zpk(none, pluss::series::make(std::vector<Complex>{Complex(-1.0, 2.0), Complex(-1.0, -2.0)}), one);
  const auto half = lti::zpk(none, pluss::series::make(std::vector<Complex>{Complex(-1.0, 2.0)}), one);
  const auto w = pluss::series::make(std::vector<double>{1.0});
  const auto wn = pluss::series::make(std::vector<double>{-1.0});
  const auto mismatch = [&](const lti::PoleZero &system) {
    return std::abs(lti::freqresp(system, wn).items<Complex>()[0] - std::conj(lti::freqresp(system, w).items<Complex>()[0]));
  };
  out.writeln("|H(-jw) - conj(H(jw))| with the full pair:", mismatch(pair), "   with one member only:", mismatch(half));
}
```

Output

```text
gain at omega = 1 (the corner):   [0.7071]   1/sqrt(2) = 0.7071
gain at 1 Hz, omega = 2 pi:       [0.1572]   passing 1 as if it were hertz gives the first line
integrator gain at omega = 0, 0.1, 1:[inf, 10.0000, 1.0000]
discrete gain at 0.5 pi and 0.5 pi + 2 pi:[0.8944, 0.8944]
on [0, pi]: [2.0000, 1.3572, 0.8944, 0.7148, 0.6667]
|H(-jw) - conj(H(jw))| with the full pair:0.0000   with one member only:0.8944
```

The lag `1 / (s + 1)` has its corner at `omega = 1`, a gain of `0.7071`; at `1` Hz, `omega = 2 pi`, the gain is `0.1572`. Passing `1` meaning one hertz gives the first number instead of the second, so convert with `omega = 2 pi f`. An integrator has an infinite gain at zero frequency, and `freqresp` reports it as `inf` rather than hiding it; evaluate it from a small positive frequency. A discrete response repeats every `2 pi / T`: the gains at `0.5 pi` and `0.5 pi + 2 pi` are identical, `0.8944`, so the response is evaluated on `[0, pi / T]` and anything beyond it is the same curve again. And a single complex pole is not a real system: with both members of the pair `H(-jw)` is the conjugate of `H(jw)` to rounding, and with only one the two differ by `0.8944`, the response of something that no real device can have.

Reference: [`zpk`](../../../../api/eng/lti/response/zpk.md), [`freqresp`](../../../../api/eng/lti/response/freqresp.md), [`state`](../../../../api/eng/lti/state/state.md), [`to_sos`](../../../../api/eng/lti/response/to_sos.md).

Where next: [Simulating a system](../lti_02_simulation/simulating_a_system.md) computes the time response of a state-space model to any input.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="controllability_and_gramians.html">
    <span class="label">Previous</span>
    <span class="title">Controllability and gramians</span>
  </a>
</div>
<!-- apiref:nav:end -->
