<!-- generated from Reserving.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::Reserving`

A reserving estimator with its method assumptions already bound.

## Signature

```cpp
using actuarial::Reserving = actuarial::Operator<actuarial::ReserveResult(const actuarial::Triangle &)>;
```

## What It Represents

An [`Operator`](../foundation/Operator.md) from a [`Triangle`](Triangle.md) to a [`ReserveResult`](ReserveResult.md). [`chain_ladder`](chain_ladder.md) and [`bornhuetter_ferguson`](bornhuetter_ferguson.md) return one, with the method assumptions fixed and the triangle varying per call. Because both estimators have the same type, a function written against `Reserving` works with either, and comparing methods on one triangle is a loop over operators. The key is the method name, `"chain_ladder"` or `"bornhuetter_ferguson"`.

## Methods

An operator is called like a function and has the four members of [`Operator`](../foundation/Operator.md): [`defined`](../foundation/Operator/defined.md), [`key`](../foundation/Operator/key.md), [`operator bool`](../foundation/Operator/operator_bool.md) and [`operator()`](../foundation/Operator/operator_call.md), which does the work described here.

## Practical Examples

### Example 1: two methods, one triangle

Both estimators called through the same type.

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
  out.print_defaults(PrintOptions{}.fixed(2));
  const pluss::Series cumulative = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                       110.0, 200.0, 250.0, 0.0,
                                                       120.0, 215.0, 0.0, 0.0,
                                                       130.0, 0.0, 0.0, 0.0}, {4, 4});
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative);
  const std::vector<actuarial::Reserving> methods = {
      actuarial::operators::chain_ladder(),
      actuarial::operators::bornhuetter_ferguson(pluss::make_series({250.0, 280.0, 300.0, 330.0}), pluss::make_series({0.96, 0.90, 0.75, 0.50}))};
  for (const auto &method : methods)
    out.writeln(method.key(), " total reserve =", method(triangle).reserve.sum().item<double>());
}
```

Output

```text
chain_ladder  total reserve = 284.09
bornhuetter_ferguson  total reserve = 278.00
```

### Example 2: one estimator, several triangles

The method is bound once and applied to different data.

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
  out.print_defaults(PrintOptions{}.fixed(2));
  const pluss::Series cumulative = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                       110.0, 200.0, 250.0, 0.0,
                                                       120.0, 215.0, 0.0, 0.0,
                                                       130.0, 0.0, 0.0, 0.0}, {4, 4});
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative);
  const actuarial::Reserving cl = actuarial::operators::chain_ladder();
  const actuarial::Triangle bigger = actuarial::triangle::cumulative(cumulative.mul(1.5));
  out.writeln("reserve    =", cl(triangle).reserve.sum().item<double>());
  out.writeln("1.5 x data =", cl(bigger).reserve.sum().item<double>());
}
```

Output

```text
reserve    = 284.09
1.5 x data = 426.13
```

### Example 3: the key

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
  out.writeln(actuarial::operators::chain_ladder().key());
}
```

Output

```text
chain_ladder
```

### Example 4: an empty operator

A default-constructed operator is not defined and throws when called.

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
  out.writeln("defined:", actuarial::Reserving{}.defined());
  report([&] { (void)actuarial::Reserving{}(actuarial::Triangle{}); });
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

[`ReserveResult`](ReserveResult.md) is what it returns, and [`Triangle`](Triangle.md) is its input.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ReserveResult.html">
    <span class="label">Previous</span>
    <span class="title">ReserveResult</span>
  </a>
  <a class="next-link" href="cumulative.html">
    <span class="label">Next</span>
    <span class="title">cumulative</span>
  </a>
</div>
<!-- apiref:nav:end -->
