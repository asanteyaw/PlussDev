<!-- generated from PidRule.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::PidRule`

Discretization rule used by a sampled PID controller.

## Signature

```cpp
control::PidRule
```

## What It Does

This value object groups artifacts that belong to one control calculation so callers can preserve both the primary answer and the diagnostics needed to interpret it. Its `Series` members retain the computation's dtype, platform, and leading batch dimensions.

The rule affects discrete controllers only. Continuous controllers retain their exact parallel-form expression.

## Fields

### `Forward`

Forward-Euler substitution. It is explicit and inexpensive but least damping-preserving.

### `Backward`

Backward-Euler substitution. It is implicit and generally more damping-preserving.

### `Trapezoid`

Bilinear/trapezoidal substitution, balancing frequency fidelity and numerical robustness.

## Returns

This type is returned by, or supplied to, the functions described in this branch. Reading its fields does not trigger another solve.

## Errors

The value object performs no deferred validation. Invalid dimensions, domains, or numerical conditions are rejected by the function that creates or consumes it.

## Interpretation and Use

Rule choice matters only for sampled controllers. `Forward` is explicit, `Backward` is more damping-preserving, and `Trapezoid` usually offers better frequency fidelity. The integral and derivative rules may differ when implementation constraints demand it.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Select trapezoidal integration and backward differentiation explicitly.

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
### Example 2

Show that continuous PID ignores discrete substitution rules.

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
### Example 3

Review validation performed before rule evaluation.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PlaceResult.html">
    <span class="label">Previous</span>
    <span class="title">PlaceResult</span>
  </a>
  <a class="next-link" href="PidController.html">
    <span class="label">Next</span>
    <span class="title">Pid</span>
  </a>
</div>
<!-- apiref:nav:end -->
