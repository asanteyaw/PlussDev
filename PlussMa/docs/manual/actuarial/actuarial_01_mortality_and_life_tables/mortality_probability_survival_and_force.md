<!-- generated from mortality_probability_survival_and_force.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Mortality Probability, Survival Probability and Force

For a life aged `x`, the one-year mortality probability

```{math}
q_x = \Pr(T_x \le 1)
```

is the probability of death during the next year, conditional on being alive at age `x`. Its complement

```{math}
p_x = 1-q_x
```

is the probability of surviving that year. The force of mortality is an instantaneous rate. On the discrete one-year convention used here,

```{math}
\mu_x = -\log(1-q_x), \qquad q_x = 1-e^{-\mu_x}.
```

These are transformations of the same one-year mortality assumption, not interchangeable numbers. For small mortality probabilities, `qx` and the force are close; as mortality rises, the distinction becomes material.

## Moving between the quantities

Four conversions connect the one-year quantities, and each has an inverse. `px_qx` turns mortality probabilities into survival probabilities and `qx_px` goes back. `force_qx` turns mortality probabilities into forces and `qx_force` goes back. They work elementwise on a one-dimensional `Series`, so a whole vector of ages or durations is converted in one call, and the result has the dtype and platform of the input.

Each function checks its domain and says what is wrong. Probabilities must be finite and lie in `[0, 1]`. The force must be non-negative. `force_qx` needs `qx < 1`: a mortality probability of one is a legitimate statement that death is certain, but it has infinite force, so it has no finite conversion and is rejected rather than turned into a large number.

## From annual probabilities to a survival path

`hazard_qx(qx)` returns the cumulative sums of the one-year forces. `survival_force(force)` returns one more value than the force vector:

```{math}
[1,\ e^{-\mu_0},\ e^{-(\mu_0+\mu_1)},\ldots].
```

The leading `1` is survival at duration zero. Consequently, a `qx` vector of length `n` describes `n` future years but produces `n + 1` survival points.

## Example 1: converting and checking the round trip

The program converts annual mortality probabilities into survival probabilities and forces, reconstructs `qx`, and then prints the survival path. The final survival probability is also the product of the annual `px` values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const pluss::Series qx = pluss::make_series({0.01, 0.02, 0.04});
  const pluss::Series px = life::px_qx(qx);
  const pluss::Series force = life::force_qx(qx);
  const pluss::Series reconstructed = life::qx_force(force);
  const pluss::Series survival = life::survival_force(force);

  out.writeln("qx          =", qx.items<double>());
  out.writeln("px          =", px.items<double>());
  out.writeln("force       =", force.items<double>());
  out.writeln("round trip  =", reconstructed.items<double>());
  out.writeln("survival    =", survival.items<double>());
}
```

Output

```text
qx          = [0.010000, 0.020000, 0.040000]
px          = [0.990000, 0.980000, 0.960000]
force       = [0.010050, 0.020203, 0.040822]
round trip  = [0.010000, 0.020000, 0.040000]
survival    = [1.000000, 0.990000, 0.970200, 0.931392]
```

The round trip recovers the original probabilities to floating-point precision. The survival path decreases from `1` to `(0.99)(0.98)(0.96) = 0.931392`.

## Mortality laws

The Gompertz law models an exponentially increasing force of mortality,

```{math}
\mu_x = b c^x,
```

while Makeham adds an age-independent component,

```{math}
\mu_x = a + b c^x.
```

`gompertz_force` and `makeham_force` evaluate these forces at a vector of attained ages, and `gompertz_qx` and `makeham_qx` return the one-year mortality probability `1 - exp(-mu)` at the same ages. The parameters have distinct roles. The scale `b` sets the age-dependent part of the force and cannot be negative. The base `c` sets how fast mortality grows with age; it must be positive, and values above one give the usual rising adult-mortality curve. The Makeham constant `a` is the age-independent force and cannot be negative. The attained ages are a non-negative one-dimensional `Series`, and the result has the same shape, dtype and platform.

## Example 2: comparing Gompertz and Makeham

This example evaluates both laws at four attained ages. It also obtains Makeham `qx` in two equivalent ways: directly and by transforming the force.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));

  const pluss::Series ages = pluss::make_series({40.0, 50.0, 60.0, 70.0});
  const pluss::Series gompertz =
      life::gompertz_force(ages, 0.0001, 1.08);
  const pluss::Series makeham =
      life::makeham_force(ages, 0.0002, 0.0001, 1.08);
  const pluss::Series direct_qx =
      life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const pluss::Series transformed_qx = life::qx_force(makeham);

  out.writeln("ages          =", ages.items<double>());
  out.writeln("Gompertz force=", gompertz.items<double>());
  out.writeln("Makeham force =", makeham.items<double>());
  out.writeln("direct qx     =", direct_qx.items<double>());
  out.writeln("from force qx =", transformed_qx.items<double>());
}
```

Output

```text
ages          = [40.00000000, 50.00000000, 60.00000000, 70.00000000]
Gompertz force= [0.00217245, 0.00469016, 0.01012571, 0.02186064]
Makeham force = [0.00237245, 0.00489016, 0.01032571, 0.02206064]
direct qx     = [0.00236964, 0.00487822, 0.01027258, 0.02181908]
from force qx = [0.00236964, 0.00487822, 0.01027258, 0.02181908]
```

The Makeham force exceeds the Gompertz force by exactly `a = 0.0002` at every age. The two `qx` rows agree because the direct surface applies the same transformation `1 - exp(-mu)` internally.

## Example 3: invalid mortality assumptions fail explicitly

An annual mortality probability of one is valid as a statement that death is certain during the year, but it has infinite force. The finite-force conversion therefore rejects it. This distinction prevents an infinite value from entering later cumulative-hazard calculations unnoticed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;

  try {
    const pluss::Series qx = pluss::make_series({0.02, 1.0});
    (void)life::force_qx(qx);
  } catch (const pluss::Error& error) {
    const std::string message(error.what());
    out.writeln("rejected:", message.substr(0, message.find('\n')));
  }
}
```

Output

```text
rejected: force_qx: qx must lie in [0,1)
```

## Reading the quantities correctly

- `qx[k]` is conditional on survival to the start of year `k`; it is not an unconditional death probability.
- `px[k]` describes survival through one year, while the survival path describes survival from duration zero through several consecutive years.
- The force is a rate, not a probability. It can exceed one even though `qx` cannot.
- A finite vector defines a finite projection horizon. The terminal survival probability is retained; the model does not assume that every life dies within the supplied horizon.
- Use direct `qx`/`px` conversions when the input is annual probability data. Use force-based functions when the model or estimation procedure naturally produces hazards.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
