<!-- generated from claims_triangles.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Claims Triangles

[Back to Ratemaking, Credibility and Loss Reserving](../actuarial_04_ratemaking_credibility_and_loss_reserving.md)

A claim is not paid when it happens. Losses that occurred in a year, the **origin**, are reported and paid over the following years, the **development** periods. A **claims triangle** records the cumulative amounts, with one row for each origin year and one column for each development period. It is a triangle because the most recent origin has had only one period to develop: the cells below the diagonal have not happened yet.

```{math}
\begin{array}{c|ccccc}
 & 12 & 24 & 36 & 48 & 60 \\ \hline
2020 & C_{0,0} & C_{0,1} & C_{0,2} & C_{0,3} & C_{0,4}\\
2021 & C_{1,0} & C_{1,1} & C_{1,2} & C_{1,3} & \cdot\\
2022 & C_{2,0} & C_{2,1} & C_{2,2} & \cdot & \cdot\\
2023 & C_{3,0} & C_{3,1} & \cdot & \cdot & \cdot\\
2024 & C_{4,0} & \cdot & \cdot & \cdot & \cdot
\end{array}
```

The unobserved cells are unknown, not zero, and the library keeps the two apart: a `Triangle` carries the values together with an explicit **observation mask**. When no mask is given, the older convention is used: a positive cell is observed and a zero is not.

```cpp
actuarial::Triangle actuarial::triangle::cumulative(const pluss::Series &values, const pluss::Series &observed = pluss::series::undefined(), std::vector<std::string> origins = {}, std::vector<std::string> development = {});
bool actuarial::Triangle::defined() const noexcept;
const pluss::Series &actuarial::Triangle::values() const noexcept;
const pluss::Series &actuarial::Triangle::observed() const noexcept;
const std::vector<std::string> &actuarial::Triangle::origins() const noexcept;
const std::vector<std::string> &actuarial::Triangle::development() const noexcept;
std::int64_t actuarial::Triangle::norigins() const noexcept;
std::int64_t actuarial::Triangle::ndevelopment() const noexcept;
pluss::Series models::age_factors(const pluss::Series &cumulative_triangle);
```

| Name | Meaning |
|---|---|
| `triangle::cumulative(values, observed, origins, development)` | a `Triangle` from a two-dimensional `Series` `[origins, development]` of cumulative amounts; `observed` is an optional mask of the same shape (1 for observed); `origins` and `development` are optional labels |
| `values()`, `observed()` | the cumulative amounts and the mask |
| `origins()`, `development()` | the labels, one per row and column |
| `norigins()`, `ndevelopment()` | the number of rows and columns |
| `models::age_factors(triangle)` | the volume-weighted age-to-age factors: for each pair of adjacent periods, the sum of the later column over the sum of the earlier, taken over the origins observed in both |

The observed cumulative amounts must be non-decreasing along each origin, since a cumulative total cannot fall.

## A first triangle

Five origin years and five development periods of cumulative paid claims, in thousands. The zero padding below the diagonal marks the unobserved cells. The program builds the triangle, reads the mask and the counts, and computes the age-to-age factors, for instance (180 + 200 + 215 + 235) / (100 + 110 + 120 + 130) = 1.8043 for the step from the first to the second period.

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
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0, 250.0,
                                                    110.0, 200.0, 245.0, 265.0, 0.0,
                                                    120.0, 215.0, 260.0, 0.0, 0.0,
                                                    130.0, 235.0, 0.0, 0.0, 0.0,
                                                    140.0, 0.0, 0.0, 0.0, 0.0}, {5, 5});
  const actuarial::Triangle t = actuarial::triangle::cumulative(triangle);
  out.writeln("origins", t.norigins(), " development periods", t.ndevelopment(), " defined", t.defined());
  out.writeln("observed cells =", t.observed().reshape({25}).sum().item<double>(), "of 25");
  out.writeln("age-to-age factors =", models::age_factors(triangle).items<double>());
  out.writeln("first step by hand =", (180.0 + 200.0 + 215.0 + 235.0) / (100.0 + 110.0 + 120.0 + 130.0));
}
```

Output

```text
origins 5  development periods 5  defined true
observed cells = 15.000000 of 25
age-to-age factors = [1.804348, 1.218487, 1.086022, 1.041667]
first step by hand = 1.804348
```

## An explicit mask and labels

Zeros are ambiguous when a cell is genuinely observed as zero, for example a recent origin that has had no payments yet. Under the zero-padding convention that cell would be read as unobserved. An explicit mask says which cells are observed whatever their value, and labels name the origins and development periods for reports. Below, the first cell of the second row is observed and zero, and the second is not observed.

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
  const pluss::Series values = pluss::make_series({100.0, 180.0, 0.0, 0.0}, {2, 2});
  const pluss::Series mask = pluss::make_series({1.0, 1.0, 1.0, 0.0}, {2, 2});
  const actuarial::Triangle t = actuarial::triangle::cumulative(values, mask, {"2023", "2024"}, {"12", "24"});
  out.writeln("origins     ", t.origins(), " development", t.development());
  out.writeln("observed    ", t.observed().reshape({4}).items<double>());
}
```

Output

```text
origins      [2023, 2024]  development [12, 24]
observed     [1.000000, 1.000000, 1.000000, 0.000000]
```

## Invalid triangles fail explicitly

The values must form a two-dimensional matrix, the labels must match the rows and columns, and the observed cumulative amounts must not decrease along an origin. An age-to-age factor needs observed cells in both periods.

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
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2}), pluss::series::undefined(), {"only one"}, {}); });
  report([&] { (void)actuarial::triangle::cumulative(pluss::make_series({100.0, 90.0, 0.0, 0.0}, {2, 2})); });
  report([&] { (void)models::age_factors(pluss::make_series({0.0, 0.0, 0.0, 0.0}, {2, 2})); });
}
```

Output

```text
triangle::cumulative: values must be 2D
triangle::cumulative: origin labels must match origin count
triangle::cumulative: observed cumulative values must be non-decreasing
chain_ladder: each development pair needs observed exposure
```

## Where to go next

The age-to-age factors are what the chain ladder method projects the unobserved cells with ([Chain Ladder](chain_ladder.md)).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="credibility.html">
    <span class="label">Previous</span>
    <span class="title">Credibility</span>
  </a>
  <a class="next-link" href="chain_ladder.html">
    <span class="label">Next</span>
    <span class="title">Chain Ladder</span>
  </a>
</div>
<!-- apiref:nav:end -->
