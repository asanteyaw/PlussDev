<!-- generated from origins.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fb23d72ae6d3 -->

# `actuarial::Triangle::origins`

[Back to `actuarial::Triangle`](../Triangle.md)

The row labels.

## Signature

```cpp
const std::vector<std::string> &actuarial::Triangle::origins() const noexcept;
```

## What It Does

Returns the labels of the origin periods in row order, or an empty vector when none were given. Labels are for reporting; they must match the number of rows when supplied and have no effect on the estimates.

## Returns

A const reference to a vector of `norigins` strings, or an empty vector.

## Practical Examples

### Example 1: labelled rows

The labels given at construction.

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
  for (const auto &o : triangle.origins()) out.write(o, " ");
  out.writeln();
}
```

Output

```text
2021  2022  2023  2024
```

### Example 2: no labels

An unlabelled triangle returns an empty vector, so fall back to the index.

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
  out.writeln("labels:", triangle.origins().size());
}
```

Output

```text
labels: 0
```

### Example 3: labelling a result

Pair the labels with the reserves.

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
  const auto r = actuarial::operators::chain_ladder()(triangle).reserve.items<double>();
  for (std::size_t i = 0; i < r.size(); ++i) out.writeln(triangle.origins()[i], r[i]);
}
```

Output

```text
2021 0.000000
2022 22.727273
2023 75.095694
2024 186.263593
```

### Example 4: a wrong count

The number of labels must equal the number of rows.

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
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({1.0, 2.0, 0.0, 0.0}, {2, 2}), pluss::series::undefined(), {"2023"}, {}); });
}
```

Output

```text
triangle::cumulative: origin labels must match origin count
```

## See Also

- [`Triangle`](../Triangle.md): the type
- [`cumulative`](../cumulative.md): builds one

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="observed.html">
    <span class="label">Previous</span>
    <span class="title">observed</span>
  </a>
  <a class="next-link" href="values.html">
    <span class="label">Next</span>
    <span class="title">values</span>
  </a>
</div>
<!-- apiref:nav:end -->
