<!-- generated from controllers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Controller construction

This branch builds explicit feedback controllers. `place` computes state-feedback gain that assigns requested closed-loop poles. `pid` constructs an immutable parallel-form PID controller whose continuous or discrete frequency response can be evaluated through `freqresp`.

## Choosing A Surface

Pole placement specifies desired dynamics directly and requires a controllable state-space realization. PID construction requires only proportional, integral, derivative, and derivative-filter gains, making it useful when a low-order error-feedback law is preferable to full-state feedback.

## Practical Examples

The examples place second-order poles, construct and inspect a continuous PID controller, and exercise controllability and PID-parameter guards.

### Example 1

Assign two continuous closed-loop poles and inspect placement diagnostics.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace control = pluss::subsp::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const control::PlaceResult result = control::place(
      pluss::make_series({{0.0, 1.0}, {-2.0, -3.0}}),
      pluss::make_series({0.0, 1.0}, {2, 1}),
      pluss::make_series(
          {std::complex<double>{-4.0, 0.0}, std::complex<double>{-5.0, 0.0}}));

  out.writeln("gain =", result.gain.flatten().items<double>());
  out.writeln("closed loop =", result.closed_loop.flatten().items<double>());
  out.writeln("residual =", result.residual.item<double>());
  out.writeln("condition =", result.condition.item<double>());
}
```

Output

```text
gain = [18.000000, 6.000000]
closed loop = [0.000000, 1.000000, -20.000000, -9.000000]
residual = 0.000000
condition = 42.023796
```
### Example 2

Construct a filtered parallel PID controller and evaluate its frequency response.

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
### Example 3

Reject an uncontrollable plant and a negative derivative-filter constant.

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
```{toctree}
:maxdepth: 2
:hidden:

controllers/PlaceOptions
controllers/PlaceResult
controllers/PidRule
controllers/PidController
controllers/place
controllers/pid
controllers/freqresp
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

controllers/PlaceOptions
controllers/PlaceResult
controllers/PidRule
controllers/PidController
controllers/place
controllers/pid
controllers/freqresp
```
<!-- apiref:toctree:end -->
