<!-- generated from defined.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1b9359dc08db -->

# `actuarial::Triangle::defined`

[Back to `actuarial::Triangle`](../Triangle.md)

Tells whether the triangle was built by `cumulative`.

## Signature

```cpp
bool actuarial::Triangle::defined() const noexcept;
```

## What It Does

True for a triangle returned by [`triangle::cumulative`](../cumulative.md), false for a default-constructed one. An undefined triangle has no dimensions (both counts are zero), undefined `values` and `observed`, and no labels, and every estimator refuses it, so test `defined` where a triangle may not have been set.

## Returns

`true` if the triangle was built by `cumulative`.

## Practical Examples

### Example 1: built and empty

A built triangle is defined; the default one is not.

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
  const pluss::Series cumulative = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                       110.0, 200.0, 250.0, 0.0,
                                                       120.0, 215.0, 0.0, 0.0,
                                                       130.0, 0.0, 0.0, 0.0}, {4, 4});
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative);
  out.writeln("built:", triangle.defined(), " default:", actuarial::Triangle{}.defined());
}
```

Output

```text
built: true  default: false
```

### Example 2: what an empty triangle reports

No dimensions and no data.

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
  const actuarial::Triangle none;
  out.writeln("origins", none.norigins(), " ages", none.ndevelopment(), " labels", none.origins().size(), " values defined", none.values().defined());
}
```

Output

```text
origins 0  ages 0  labels 0  values defined false
```

### Example 3: estimators refuse it

Both estimators report the missing triangle.

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
  report([&] { (void)actuarial::operators::chain_ladder()(actuarial::Triangle{}); });
  report([&] { (void)actuarial::operators::bornhuetter_ferguson(pluss::make_series({1.0}), pluss::make_series({0.5}))(actuarial::Triangle{}); });
}
```

Output

```text
chain_ladder: triangle must be defined
bornhuetter_ferguson: triangle must be defined
```

## See Also

- [`Triangle`](../Triangle.md): the type
- [`cumulative`](../cumulative.md): builds one

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="development.html">
    <span class="label">Next</span>
    <span class="title">development</span>
  </a>
</div>
<!-- apiref:nav:end -->
