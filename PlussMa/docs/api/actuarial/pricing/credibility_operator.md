<!-- generated from credibility_operator.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::operators::credibility`

Creates a limited-fluctuation credibility operator.

## Signature

```cpp
actuarial::Credibility actuarial::operators::credibility(double full_credibility);
```

## What It Does

Binds the standard `K` into a [`Credibility`](Credibility.md). Called on a [`CredibilityInput`](CredibilityInput.md), it returns {math}`Z\,\text{experience}+(1-Z)\,\text{manual}` with {math}`Z=\text{exposure}/(\text{exposure}+K)`. In this smooth form `K` is the exposure at which `Z = 1/2`: a cell with exposure `K` trusts its own data as much as the manual rate, and credibility rises toward one as exposure grows but never reaches it. Use it to stabilise rates of thin cells; [`models::credibility_weight`](../models/credibility_weight.md) gives `Z` alone.

## Parameters

`full_credibility`
: The standard `K`, finite and greater than 0.

## Returns

A `Credibility` with key `"credibility"`.

## Errors

Throws `operators::credibility: full_credibility must be finite and > 0` when built. Calling it throws `credibility: experience, manual and exposure must have the same length`, `credibility_weight: exposure must be >= 0`, `credibility_weight: exposure must be finite`, `credibility: experience must be >= 0` and the corresponding `must be defined` errors.

## Practical Examples

### Example 1: three exposures

Zero exposure returns the manual rate; a large one nearly the experience.

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
  const actuarial::CredibilityInput input{pluss::make_series({0.80, 0.80, 0.80}), pluss::make_series({0.65, 0.65, 0.65}), pluss::make_series({0.0, 100.0, 10000.0})};
  out.writeln(actuarial::operators::credibility(100.0)(input).items<double>());
}
```

Output

```text
[0.650000, 0.725000, 0.798515]
```

### Example 2: Z by hand

Z is exposure / (exposure + K).

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
  for (const double e : {0.0, 100.0, 10000.0}) out.writeln("exposure", e, " Z", e / (e + 100.0));
}
```

Output

```text
exposure 0.000000  Z 0.000000
exposure 100.000000  Z 0.500000
exposure 10000.000000  Z 0.990099
```

### Example 3: the weight of experience falls with K

At exposure 100, the blend moves toward the manual rate as K grows.

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
  for (const double k : {1.0, 100.0, 10000.0}) out.writeln("K", k, actuarial::operators::credibility(k)(input).items<double>()[0]);
}
```

Output

```text
K 1.000000 0.798515
K 100.000000 0.725000
K 10000.000000 0.651485
```

### Example 4: a portfolio

Four cells with very different exposures.

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

### Example 5: the errors

A standard that is not positive fails at construction; bad data at the call.

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
  report([&] { (void)actuarial::operators::credibility(0.0); });
  report([&] { (void)actuarial::operators::credibility(-5.0); });
  report([&] { (void)actuarial::operators::credibility(std::nan("")); });
  report([&] { (void)actuarial::operators::credibility(100.0)(actuarial::CredibilityInput{pluss::make_series({0.8, 0.7}), pluss::make_series({0.6, 0.6, 0.6}), pluss::make_series({1.0})}); });
}
```

Output

```text
operators::credibility: full_credibility must be finite and > 0
operators::credibility: full_credibility must be finite and > 0
operators::credibility: full_credibility must be finite and > 0
credibility: experience, manual and exposure must have the same length
```

## See Also

- [`Credibility`](Credibility.md): the type
- [`CredibilityInput`](CredibilityInput.md): the data

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="price.html">
    <span class="label">Previous</span>
    <span class="title">price</span>
  </a>
</div>
<!-- apiref:nav:end -->
