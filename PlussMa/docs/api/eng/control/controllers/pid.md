<!-- generated from pid.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::pid`

Constructs a continuous or discrete PID controller.

## Signature

```cpp
control::Pid control::pid(
    const pluss::Series& kp,
    const pluss::Series& ki,
    const pluss::Series& kd,
    const pluss::Series& filter,
    double sample_time = 0.0,
    control::PidRule int_rule = control::PidRule::Forward,
    control::PidRule diff_rule = control::PidRule::Forward);
```

## What It Does

The represented continuous transfer function is `Kp + Ki/s + Kd*s/(filter*s + 1)`. For discrete controllers, the selected rules replace the integral and filtered-derivative factors.

## Parameters

`kp`
: Proportional gain, scalar or batched.

`ki`
: Integral gain, broadcast-compatible with the other gains.

`kd`
: Derivative gain.

`filter`
: Non-negative derivative-filter time constant in `Kd*s/(filter*s+1)`.

`sample_time`
: Zero for continuous time or a positive discrete sampling interval.

`int_rule`
: Discrete integrator substitution.

`diff_rule`
: Discrete filtered-derivative substitution.

## Returns

A validated immutable `control::Pid`.

## Errors

Throws when gains cannot broadcast, the filter is negative, or the sample time is negative/non-finite.

## Interpretation and Use

A zero sample time creates the continuous parallel form. A positive sample time activates the stored substitution rules. Keep `filter` positive when derivative action is nonzero to limit high-frequency amplification; zero remains valid when an ideal derivative is intentional.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Construct a continuous filtered PID.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>
#include <iostream>

namespace control = pluss::subsp::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const control::Pid controller = control::pid(
      pluss::make_series(2.0), pluss::make_series(3.0),
      pluss::make_series(4.0), pluss::make_series(0.5));
  const pluss::Series response =
      control::freqresp(controller, pluss::make_series({0.5, 1.0}));

  std::cout << std::boolalpha << "continuous = " << controller.is_cont()
            << ", discrete = " << controller.is_disc() << "\n";
  out.writeln("response =", response.items<std::complex<double>>());
}
```

Output

```text
continuous = true, discrete = false
response = [(2.47059,-4.11765), (3.6,0.2)]
```
### Example 2

Construct a sampled controller with explicit rules.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>
#include <iostream>

namespace control = pluss::subsp::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const control::Pid controller = control::pid(
      pluss::make_series(1.5), pluss::make_series(0.4),
      pluss::make_series(0.2), pluss::make_series(0.05), 0.1,
      control::PidRule::Trapezoid, control::PidRule::Backward);
  const pluss::Series response =
      control::freqresp(controller, pluss::make_series({0.5, 2.0}));

  std::cout << std::boolalpha
            << "continuous = " << controller.is_cont()
            << ", discrete = " << controller.is_disc() << "\n";
  std::cout << "sample time = " << controller.sample_time()
            << ", integral rule = " << static_cast<int>(controller.int_rule())
            << ", derivative rule = " << static_cast<int>(controller.diff_rule())
            << "\n";
  out.writeln("discrete response =",
              response.items<std::complex<double>>());
}
```

Output

```text
continuous = false, discrete = true
sample time = 0.1, integral rule = 2, derivative rule = 1
discrete response = [(1.50499,-0.700062), (1.57742,0.18647)]
```
### Example 3

Reject a negative derivative-filter constant.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>
#include <exception>
#include <string>

namespace control = pluss::subsp::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(control::place(
        pluss::make_series({{0.0, 1.0}, {-2.0, -3.0}}),
        pluss::series::zeros({2, 1}, pluss::ScalarType::Float64),
        pluss::make_series({std::complex<double>{-4.0, 0.0},
                            std::complex<double>{-5.0, 0.0}})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("uncontrollable plant rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(control::pid(
        pluss::make_series(1.0), pluss::make_series(0.0),
        pluss::make_series(0.0), pluss::make_series(-0.1)));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("negative filter rejected =",
                message.substr(0, message.find('\n')));
  }
}
```

Output

```text
uncontrollable plant rejected = eng::control::place: B must have non-zero numerical rank
negative filter rejected = eng::control::pid: filter must be non-negative
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="place.html">
    <span class="label">Previous</span>
    <span class="title">place</span>
  </a>
  <a class="next-link" href="freqresp.html">
    <span class="label">Next</span>
    <span class="title">freqresp</span>
  </a>
</div>
<!-- apiref:nav:end -->
