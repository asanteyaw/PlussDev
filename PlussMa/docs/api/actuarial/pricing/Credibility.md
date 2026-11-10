<!-- generated from Credibility.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::Credibility`

A limited-fluctuation credibility blend with its full-credibility standard already bound.

## Signature

```cpp
using actuarial::Credibility = actuarial::Operator<pluss::Series(const actuarial::CredibilityInput &)>;
```

## What It Represents

An [`Operator`](../foundation/Operator.md) from a [`CredibilityInput`](CredibilityInput.md) to a series of blended rates. [`credibility`](credibility_operator.md) creates it with the standard `K` fixed. A larger `K` gives less weight to experience at the same exposure; zero exposure gives the manual rate. Its key is `"credibility"`.

## Methods

An operator is called like a function and has the four members of [`Operator`](../foundation/Operator.md): [`defined`](../foundation/Operator/defined.md), [`key`](../foundation/Operator/key.md), [`operator bool`](../foundation/Operator/operator_bool.md) and [`operator()`](../foundation/Operator/operator_call.md), which does the work described here.

## Practical Examples

### Example 1: the effect of K

A larger standard trusts the experience less.

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
  const actuarial::CredibilityInput input{pluss::make_series({0.80}), pluss::make_series({0.65}), pluss::make_series({100.0})};
  for (const double k : {10.0, 100.0, 1000.0})
    out.writeln("K =", k, " blend =", actuarial::operators::credibility(k)(input).items<double>()[0]);
}
```

Output

```text
K = 10.000000  blend = 0.786364
K = 100.000000  blend = 0.725000
K = 1000.000000  blend = 0.663636
```

### Example 2: blending a whole book

Many cells in one call.

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
  const actuarial::CredibilityInput input{pluss::make_series({0.90, 0.60, 0.75, 0.55}), pluss::make_series({0.65, 0.65, 0.65, 0.65}), pluss::make_series({50.0, 400.0, 5.0, 2000.0})};
  out.writeln(actuarial::operators::credibility(100.0)(input).items<double>());
}
```

Output

```text
[0.733333, 0.610000, 0.654762, 0.554762]
```

### Example 3: the key

The key of the operator.

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
  out.writeln(actuarial::operators::credibility(100.0).key());
}
```

Output

```text
credibility
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
  out.writeln("defined:", actuarial::Credibility{}.defined());
  report([&] { (void)actuarial::Credibility{}(actuarial::CredibilityInput{}); });
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

[`credibility`](credibility_operator.md) creates it, and [`CredibilityInput`](CredibilityInput.md) is its input.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Pricing.html">
    <span class="label">Previous</span>
    <span class="title">Pricing</span>
  </a>
  <a class="next-link" href="price.html">
    <span class="label">Next</span>
    <span class="title">price</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
