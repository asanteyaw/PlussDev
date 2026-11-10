<!-- generated from ndevelopment.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3dc357a44566 -->

# `actuarial::Triangle::ndevelopment`

[Back to `actuarial::Triangle`](../Triangle.md)

The number of development ages.

## Signature

```cpp
std::int64_t actuarial::Triangle::ndevelopment() const noexcept;
```

## What It Does

The number of columns. A supplied cumulative development factor vector for [`chain_ladder`](../chain_ladder.md) must have this length. An undefined triangle has zero.

## Returns

The column count.

## Practical Examples

### Example 1: the count



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
  out.writeln(triangle.ndevelopment());
}
```

Output

```text
4
```

### Example 2: sizing a supplied pattern

A development pattern has one factor per age.

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
  const pluss::Series cdf = pluss::make_series({1.8, 1.2, 1.1, 1.0});
  out.writeln(cdf.numel() == triangle.ndevelopment());
}
```

Output

```text
true
```

### Example 3: the number of factors

A chain-ladder estimate has one age-to-age factor fewer than there are ages.

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
  out.writeln(actuarial::operators::chain_ladder()(triangle).age_to_age.numel(), triangle.ndevelopment() - 1);
}
```

Output

```text
3 3
```

### Example 4: a pattern of the wrong length

The error names the mismatch.

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
  report([&] { (void)actuarial::operators::chain_ladder(pluss::make_series({1.8, 1.2, 1.0}))(actuarial::triangle::cumulative(pluss::make_series({100.0, 180.0, 220.0, 240.0, 110.0, 200.0, 250.0, 0.0, 120.0, 215.0, 0.0, 0.0, 130.0, 0.0, 0.0, 0.0}, {4, 4}))); });
}
```

Output

```text
chain_ladder: cdf length must match development count
```

## See Also

- [`Triangle`](../Triangle.md): the type
- [`cumulative`](../cumulative.md): builds one

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="development.html">
    <span class="label">Previous</span>
    <span class="title">development</span>
  </a>
  <a class="next-link" href="norigins.html">
    <span class="label">Next</span>
    <span class="title">norigins</span>
  </a>
</div>
<!-- apiref:nav:end -->
