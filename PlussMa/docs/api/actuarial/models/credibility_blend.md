<!-- generated from credibility_blend.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::models::credibility_blend`

Blend experience and manual pure premiums by credibility.

## Signature

```cpp
pluss::Series actuarial::models::credibility_blend(const pluss::Series &experience_pure_premium, const pluss::Series &manual_pure_premium, const pluss::Series &exposure, double credibility_k);
```

## What It Does

Returns {math}`Z\,\text{experience}+(1-Z)\,\text{manual}` with {math}`Z=E/(E+k)` ([`credibility_weight`](credibility_weight.md)), cell by cell. A cell with little exposure gets nearly the manual rate; one with a lot gets nearly its own. It is [`operators::credibility`](../pricing/credibility_operator.md) called with the three series as arguments, and is the stabilising step between measuring [`premium_curve`](premium_curve.md) and setting rates.

## Parameters

`experience_pure_premium`, `manual_pure_premium`
: Series of the same length, finite and 0 or more.

`exposure`
: Volume behind each experience figure, the same length.

`credibility_k`
: The credibility standard, finite and greater than 0.

## Returns

A series of the same length as the inputs.

## Errors

Throws `credibility_blend: experience_pure_premium and manual_pure_premium must have same length`, the `must be defined/finite/>= 0` errors for each argument, `credibility: experience, manual and exposure must have the same length`, and `operators::credibility: full_credibility must be finite and > 0` for `credibility_k`.

## Practical Examples

### Example 1: four cells

Experience around 0.8 against a manual rate of 0.65 at different exposures.

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
  out.writeln(actuarial::models::credibility_blend(pluss::make_series({0.90, 0.60, 0.75, 0.55}), pluss::make_series({0.65, 0.65, 0.65, 0.65}), pluss::make_series({50.0, 400.0, 5.0, 2000.0}), 100.0).items<double>());
}
```

Output

```text
[0.733333, 0.610000, 0.654762, 0.554762]
```

### Example 2: smoothing the cells of a book

Blend the pure premium of each cell with the book rate.

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
  const pluss::Series losses = pluss::make_series({520000.0, 690000.0, 810000.0, 1100000.0});
  const pluss::Series exposure = pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0});
  const pluss::Series premium = pluss::make_series({700.0, 720.0, 740.0, 760.0});
  const pluss::Series cells = actuarial::models::premium_curve(losses, exposure);
  const pluss::Series book = pluss::series::full({4}, actuarial::models::pure_premium(losses, exposure).item<double>());
  out.writeln("cells   =", cells.items<double>());
  out.writeln("blended =", actuarial::models::credibility_blend(cells, book, exposure, 1500.0).items<double>());
}
```

Output

```text
cells   = [520.000000, 575.000000, 540.000000, 611.111111]
blended = [548.363623, 570.707058, 553.636353, 591.184563]
```

### Example 3: the same as the operator

Identical to the credibility operator.

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
  const pluss::Series e = pluss::make_series({0.90, 0.60}), m = pluss::make_series({0.65, 0.65}), x = pluss::make_series({50.0, 400.0});
  out.writeln(actuarial::models::credibility_blend(e, m, x, 100.0).items<double>());
  out.writeln(actuarial::operators::credibility(100.0)(actuarial::CredibilityInput{e, m, x}).items<double>());
}
```

Output

```text
[0.733333, 0.610000]
[0.733333, 0.610000]
```

### Example 4: no exposure and very large exposure

The two limits: the manual rate and the experience.

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
  out.writeln(actuarial::models::credibility_blend(pluss::make_series({0.90, 0.90}), pluss::make_series({0.65, 0.65}), pluss::make_series({0.0, 1.0e9}), 100.0).items<double>());
}
```

Output

```text
[0.650000, 0.900000]
```

### Example 5: the errors

Length mismatches and a bad standard are refused.

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
  report([&] { (void)actuarial::models::credibility_blend(pluss::make_series({0.9, 0.6}), pluss::make_series({0.65}), pluss::make_series({50.0, 400.0}), 100.0); });
  report([&] { (void)actuarial::models::credibility_blend(pluss::make_series({0.9, 0.6}), pluss::make_series({0.65, 0.65}), pluss::make_series({50.0}), 100.0); });
  report([&] { (void)actuarial::models::credibility_blend(pluss::make_series({0.9}), pluss::make_series({0.65}), pluss::make_series({50.0}), 0.0); });
  report([&] { (void)actuarial::models::credibility_blend(pluss::make_series({-0.9}), pluss::make_series({0.65}), pluss::make_series({50.0}), 100.0); });
}
```

Output

```text
credibility_blend: experience_pure_premium and manual_pure_premium must have same length
credibility: experience, manual and exposure must have the same length
operators::credibility: full_credibility must be finite and > 0
credibility_blend: experience_pure_premium must be >= 0
```

## See Also

- [`credibility_weight`](credibility_weight.md): the weight alone
- [`operators::credibility`](../pricing/credibility_operator.md): the operator form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="credibility_weight.html">
    <span class="label">Previous</span>
    <span class="title">credibility_weight</span>
  </a>
  <a class="next-link" href="indicated_premium.html">
    <span class="label">Next</span>
    <span class="title">indicated_premium</span>
  </a>
</div>
<!-- apiref:nav:end -->
