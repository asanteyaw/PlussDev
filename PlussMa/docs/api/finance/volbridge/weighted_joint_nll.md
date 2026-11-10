<!-- generated from weighted_joint_nll.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `bridge::weighted_joint_nll`

The negative of the weighted joint log-likelihood: the quantity a minimiser reduces.

## Signature

```cpp
double bridge::weighted_joint_nll(double returns_loglik, double options_loglik, const bridge::JointLikelihoodWeights &weights = {});
```

## What It Does

`−(w_r · returns_loglik + w_o · options_loglik)`: [`weighted_joint_loglik`](weighted_joint_loglik.md) negated, for use as the objective of a minimiser. It is the function to hand to an optimiser that estimates NGARCH parameters, their premium, and the option-price fit together.

## Parameters

`double returns_loglik`
: Log-likelihood of the returns.

`double options_loglik`
: Log-likelihood of the option prices.

`const bridge::JointLikelihoodWeights &weights`
: The weights.

## Returns

The negative weighted sum.

## Errors

- As for `weighted_joint_loglik`.

## Practical Examples

### Example 1

The default weights.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  out.writeln(fin::bridge::weighted_joint_nll(-1500.0, -120.0));
}
```

Output

```text
1620.00
```

### Example 2

It is the negative of the log-likelihood form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  out.writeln(fin::bridge::weighted_joint_nll(-1500.0, -120.0, {.returns_weight = 0.5, .options_weight = 2.0}), fin::bridge::weighted_joint_loglik(-1500.0, -120.0, {.returns_weight = 0.5, .options_weight = 2.0}));
}
```

Output

```text
990.00 -990.00
```

### Example 3

A one-parameter search: the premium that minimises a joint objective built from a returns piece that does not depend on it and a quadratic option piece.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  double best = 1e30, best_lambda = 0.0;
  for (double lambda = -0.1; lambda <= 0.1001; lambda += 0.01) {
    const double options_loglik = -1000.0 * (lambda - 0.03) * (lambda - 0.03) - 20.0;
    const double nll = fin::bridge::weighted_joint_nll(-1500.0, options_loglik);
    if (nll < best) { best = nll; best_lambda = lambda; }
  }
  out.writeln("best lambda", best_lambda);
}
```

Output

```text
best lambda 0.03
```

## See Also

- [`weighted_joint_loglik`](weighted_joint_loglik.md): the log-likelihood form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weighted_joint_loglik.html">
    <span class="label">Previous</span>
    <span class="title">weighted_joint_loglik</span>
  </a>
</div>
<!-- apiref:nav:end -->
