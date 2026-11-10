<!-- generated from PidController.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::Pid`

An immutable validated parallel-form PID controller.

## Signature

```cpp
const pluss::Series& control::Pid::kp() const noexcept;
const pluss::Series& control::Pid::ki() const noexcept;
const pluss::Series& control::Pid::kd() const noexcept;
const pluss::Series& control::Pid::filter() const noexcept;
double control::Pid::sample_time() const noexcept;
bool control::Pid::is_cont() const noexcept;
bool control::Pid::is_disc() const noexcept;
control::PidRule control::Pid::int_rule() const noexcept;
control::PidRule control::Pid::diff_rule() const noexcept;
```

## What It Does

`Pid` stores proportional, integral, derivative, derivative-filter, timebase, and discretization data after `control::pid` has validated them. The object is immutable so it can be shared safely between repeated frequency-response analyses.

## Methods

`kp()`, `ki()`, `kd()`
: Return the controller gains without copying.

`filter()`
: Returns the non-negative derivative-filter time constant.

`sample_time()`
: Returns zero for continuous time or the positive sampling interval for discrete time.

`is_cont()`, `is_disc()`
: Report the mutually exclusive timebase state.

`int_rule()`, `diff_rule()`
: Return the selected discrete substitutions.

## Returns

Accessors return const `Series` references, scalar metadata, or `PidRule` values.

## Errors

Accessors do not fail. Invalid gains, filter values, or sample times are rejected by `control::pid` before a `Pid` exists.

## Changelog

- **0.1** — Initial immutable continuous/discrete parallel PID contract.

## Interpretation and Use

`Pid` is immutable: construction validates the controller, and accessors expose its stored contract without recomputation. Use `is_cont()`/`is_disc()` before interpreting rules, and preserve the object for repeated frequency sweeps.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Inspect and evaluate a continuous PID controller.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>
#include <iostream>

namespace control = pluss::ma::eng::control;
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

Inspect timebase and rule accessors on a sampled controller.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>
#include <iostream>

namespace control = pluss::ma::eng::control;
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

See invalid construction rejected before an object exists.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>
#include <exception>
#include <string>

namespace control = pluss::ma::eng::control;
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

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PidRule.html">
    <span class="label">Previous</span>
    <span class="title">PidRule</span>
  </a>
  <a class="next-link" href="place.html">
    <span class="label">Next</span>
    <span class="title">place</span>
  </a>
</div>
<!-- apiref:nav:end -->
