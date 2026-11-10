<!-- generated from freqresp.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::freqresp`

Evaluates a PID frequency response.

## Signature

```cpp
pluss::Series control::freqresp(
    const control::Pid& controller,
    const pluss::Series& omega);
```

## What It Does

Continuous controllers are evaluated on `s = j omega`; discrete controllers use their stored sampling interval and discretization rules. Computation remains vectorized over frequency and controller batches.

## Parameters

`controller`
: A validated controller created by `control::pid`.

`omega`
: Real angular frequencies in radians per unit time.

## Returns

A complex `Series` whose trailing axis follows the frequency grid and whose leading axes follow broadcast controller batches.

## Errors

Throws for invalid frequency data or singular evaluations such as an active integral term at zero frequency.

## Interpretation and Use

`freqresp` returns complex response values without changing the controller. Avoid zero frequency when integral gain is active because the ideal integral contribution is singular. For discrete controllers, keep the requested band below the Nyquist frequency for interpretable results.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Evaluate a continuous controller at two frequencies.

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

Evaluate a sampled controller using its stored substitutions.

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

Review controller validation that precedes response evaluation.

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
  <a class="prev-link" href="pid.html">
    <span class="label">Previous</span>
    <span class="title">pid</span>
  </a>
</div>
<!-- apiref:nav:end -->
