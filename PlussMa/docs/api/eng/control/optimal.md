<!-- generated from optimal.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Optimal feedback

This branch solves continuous and discrete algebraic Riccati control problems. `care` and `dare` expose the equations directly; `lqr` reads the timebase from an `lti::StateSpace` and routes to the correct solver. Every result includes the value matrix, optimal feedback gain, and resulting `A - B K` dynamics.

## Cost Contract

`Q` must be Hermitian positive semidefinite and `R` Hermitian positive definite. An optional state-input cross weight represents the off-diagonal block of the joint quadratic cost; the complete joint cost must remain positive semidefinite. Leading dimensions are batch axes and compatible inputs broadcast.

## Practical Examples

The examples synthesize continuous feedback, compare direct and routed discrete solutions, and show cost validation failures.

### Example 1

Solve a scalar continuous LQR problem and inspect every returned artifact.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::StateSpace plant = lti::state(
      pluss::make_series({-2.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const control::RiccatiResult result = control::lqr(
      plant, pluss::make_series({4.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}));

  out.writeln("value =", result.value.flatten().items<double>());
  out.writeln("gain =", result.gain.flatten().items<double>());
  out.writeln("closed loop =", result.closed_loop.flatten().items<double>());
}
```

Output

```text
value = [0.828427]
gain = [0.828427]
closed loop = [-2.828427]
```
### Example 2

Show that direct `dare` and timebase-routed `lqr` produce the same discrete value solution.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const pluss::Series a = pluss::make_series({0.5}, {1, 1});
  const pluss::Series b = pluss::make_series({1.0}, {1, 1});
  const pluss::Series q = pluss::make_series({1.0}, {1, 1});
  const pluss::Series r = pluss::make_series({1.0}, {1, 1});
  const control::RiccatiResult direct = control::dare(a, b, q, r);
  const lti::StateSpace plant = lti::state(
      a, b, pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({0.0}, {1, 1}), 1.0);
  const control::RiccatiResult routed = control::lqr(plant, q, r);

  out.writeln("direct value =", direct.value.flatten().items<double>());
  out.writeln("routed value =", routed.value.flatten().items<double>());
  out.writeln("discrete gain =", routed.gain.flatten().items<double>());
}
```

Output

```text
direct value = [1.132782]
routed value = [1.132782]
discrete gain = [0.265564]
```
### Example 3

Reject a singular input weight and a non-Hermitian state weight.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace control = pluss::subsp::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const pluss::Series scalar = pluss::make_series({1.0}, {1, 1});
  try {
    static_cast<void>(control::care(
        pluss::make_series({-2.0}, {1, 1}), scalar,
        pluss::make_series({4.0}, {1, 1}),
        pluss::make_series({0.0}, {1, 1})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("singular R rejected =", message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(control::care(
        pluss::make_series({{-1.0, 0.0}, {0.0, -2.0}}),
        pluss::make_series({1.0, 0.0}, {2, 1}),
        pluss::make_series({{1.0, 2.0}, {0.0, 1.0}}), scalar));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("non-Hermitian Q rejected =",
                message.substr(0, message.find('\n')));
  }
}
```

Output

```text
singular R rejected = eng::control::care: R must be positive definite
non-Hermitian Q rejected = eng::control::care: Q must be Hermitian
```
```{toctree}
:maxdepth: 2
:hidden:

optimal/RiccatiResult
optimal/care
optimal/dare
optimal/lqr
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

optimal/RiccatiResult
optimal/care
optimal/dare
optimal/lqr
```
<!-- apiref:toctree:end -->
