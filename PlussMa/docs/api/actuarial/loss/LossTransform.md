<!-- generated from LossTransform.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::LossTransform`

A rule that turns gross losses into the payments of a contract, one amount for one amount.

## Signature

```cpp
using actuarial::LossTransform = actuarial::Operator<pluss::Series(const pluss::Series &)>;
```

## What It Represents

An [`Operator`](../foundation/Operator.md) from a series of losses to a series of payments of the same shape and platform. [`deductible`](deductible.md), [`limit`](limit.md), [`coinsurance`](coinsurance.md) and [`layer`](layer.md) all return one. Because input and output have the same type, transforms **compose** by calling one on the output of another, in the order the contract applies them. The result is a payment: pass it to [`LossMeasure`](LossMeasure.md) for its expectation. A custom rule is built with [`make_operator`](../foundation/make_operator.md).

## Methods

An operator is called like a function and has the four members of [`Operator`](../foundation/Operator.md): [`defined`](../foundation/Operator/defined.md), [`key`](../foundation/Operator/key.md), [`operator bool`](../foundation/Operator/operator_bool.md) and [`operator()`](../foundation/Operator/operator_call.md), which does the work described here.

## Practical Examples

### Example 1: composing a policy

A policy with a 100 deductible, then a 1,000 limit on what is left, then 80% coinsurance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  const actuarial::LossTransform policy = actuarial::make_operator<pluss::Series(const pluss::Series &)>("policy", [](const pluss::Series &gross) {
    return actuarial::operators::coinsurance(0.8)(actuarial::operators::limit(1000.0)(actuarial::operators::deductible(100.0)(gross)));
  });
  out.writeln("gross   =", x.items<double>());
  out.writeln("payment =", policy(x).items<double>());
}
```

Output

```text
gross   = [80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
payment = [0.000000, 40.000000, 160.000000, 480.000000, 800.000000, 800.000000]
```

### Example 2: the same shape out as in

The output has the shape of the input.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  const pluss::Series pay = actuarial::operators::limit(500.0)(x);
  out.writeln("in", x.numel(), " out", pay.numel());
}
```

Output

```text
in 6  out 6
```

### Example 3: the key names the contract

Each factory names its transform in the key.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  out.writeln(actuarial::operators::deductible(1.0).key(), actuarial::operators::limit(1.0).key(), actuarial::operators::coinsurance(0.5).key(), actuarial::operators::layer(1.0, 1.0).key());
}
```

Output

```text
deductible limit coinsurance layer
```

### Example 4: an empty transform

A default-constructed transform is not defined and throws when called.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
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
  out.writeln("defined:", actuarial::LossTransform{}.defined());
  report([&] { (void)actuarial::LossTransform{}(pluss::make_series({1.0})); });
}
```

Output

```text
defined: false
actuarial Operator is undefined
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`LossMeasure`](LossMeasure.md) reduces the payments, and [`Operator`](../foundation/Operator.md) is the callable contract.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="LossMeasure.html">
    <span class="label">Next</span>
    <span class="title">LossMeasure</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
