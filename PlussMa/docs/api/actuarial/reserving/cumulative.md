<!-- generated from cumulative.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::triangle::cumulative`

Validates a cumulative claims matrix and builds a Triangle.

## Signature

```cpp
actuarial::Triangle actuarial::triangle::cumulative(const pluss::Series &values, const pluss::Series &observed = pluss::series::undefined(), std::vector<std::string> origins = {}, std::vector<std::string> development = {});
```

## What It Does

Checks that `values` is a cumulative claims matrix, attaches the observation mask and the optional labels, and returns the [`Triangle`](Triangle.md) every estimator consumes. The checks are the contract the estimators rely on: the matrix is finite and non-negative, cumulative amounts do not decrease along an observed row, unobserved cells are zero, and the observed cells of each row are contiguous from the left. Without a mask a cell is observed when it is positive, which is right for the usual zero-padded triangle; supply a mask when an observed cumulative amount can really be zero, since that is the only unambiguous way to keep it.

## Parameters

`values`
: Two-dimensional cumulative amounts, origins by ages, zero where unobserved.

`observed`
: Optional boolean matrix of the same shape. Default: positive cells are observed.

`origins`
: Optional labels, one per row.

`development`
: Optional labels, one per column.

## Returns

A defined `Triangle` that keeps the values, the mask, the labels and the platform.

## Errors

Throws `triangle::cumulative: values must be 2D`, `values must be finite`, `values must be >= 0`, `observed must match values shape`, `observed cells must be left-contiguous`, `unobserved cells must contain zero`, `observed cumulative values must be non-decreasing`, `origin labels must match origin count` and `development labels must match development count`.

## Practical Examples

### Example 1: a triangle from zero-padded data

The usual case: zeros mark the future.

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
  out.print_defaults(PrintOptions{}.fixed(0));
  const pluss::Series cumulative = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                       110.0, 200.0, 250.0, 0.0,
                                                       120.0, 215.0, 0.0, 0.0,
                                                       130.0, 0.0, 0.0, 0.0}, {4, 4});
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative);
  out.writeln(triangle.norigins(), triangle.ndevelopment(), triangle.observed().sum().item<double>());
}
```

Output

```text
4 4 10
```

### Example 2: with labels

Name the rows and columns.

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
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(cumulative, pluss::series::undefined(), {"2021", "2022", "2023", "2024"}, {"12", "24", "36", "48"});
  out.writeln(triangle.origins().front(), triangle.development().back());
}
```

Output

```text
2021 48
```

### Example 3: with a mask

A genuine zero first cell is kept by saying so.

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
  out.print_defaults(PrintOptions{}.fixed(0));
  const pluss::Series values = pluss::make_series({0.0, 10.0, 5.0, 0.0}, {2, 2});
  const pluss::Series mask = pluss::make_series({1.0, 1.0, 1.0, 0.0}, {2, 2}).gt(0.5);
  const actuarial::Triangle t = actuarial::triangle::cumulative(values, mask);
  out.writeln(t.observed().sum().item<double>(), t.values().reshape({4}).items<double>());
}
```

Output

```text
3 [0, 10, 5, 0]
```

### Example 4: a rectangular triangle

Three origins and three ages.

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
  out.print_defaults(PrintOptions{}.fixed(0));
  const actuarial::Triangle t = actuarial::triangle::cumulative(pluss::make_series({100.0, 150.0, 170.0, 110.0, 160.0, 0.0, 120.0, 0.0, 0.0}, {3, 3}));
  out.writeln(t.norigins(), t.ndevelopment());
}
```

Output

```text
3 3
```

### Example 5: the errors

Each rule of the contract has its own message.

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
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({1.0, 2.0})); });
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({100.0, 90.0, 0.0, 0.0}, {2, 2})); });
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({-1.0, 0.0, 0.0, 0.0}, {2, 2})); });
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({1.0, std::nan(""), 0.0, 0.0}, {2, 2})); });
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({100.0, 150.0, 120.0, 5.0}, {2, 2}), pluss::make_series({1.0, 1.0, 1.0, 0.0}, {2, 2}).gt(0.5)); });
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({100.0, 0.0, 5.0, 0.0}, {2, 2}), pluss::make_series({1.0, 0.0, 1.0, 0.0}, {2, 2}).gt(0.5)); });
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({1.0, 2.0, 0.0, 0.0}, {2, 2}), pluss::make_series({1.0, 2.0}).gt(0.5)); });
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({0.0, 10.0, 5.0, 0.0}, {2, 2})); });
}
```

Output

```text
triangle::cumulative: values must be 2D
triangle::cumulative: observed cumulative values must be non-decreasing
triangle::cumulative: values must be >= 0
triangle::cumulative: values must be finite
triangle::cumulative: unobserved cells must contain zero
triangle::cumulative: observed must match values shape
triangle::cumulative: observed cells must be left-contiguous
```

## See Also

- [`Triangle`](Triangle.md): the type and its accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Reserving.html">
    <span class="label">Previous</span>
    <span class="title">Reserving</span>
  </a>
  <a class="next-link" href="chain_ladder.html">
    <span class="label">Next</span>
    <span class="title">chain_ladder</span>
  </a>
</div>
<!-- apiref:nav:end -->
