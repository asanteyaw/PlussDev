<!-- generated from chain_ladder.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Chain Ladder

[Back to Ratemaking, Credibility and Loss Reserving](../actuarial_04_ratemaking_credibility_and_loss_reserving.md)

The chain ladder method predicts how the claims of each origin year will finish by assuming that they develop the way earlier years did. If a period of development multiplied the cumulative claims of every observed origin by about 1.8, the next-most-recent origin will do the same.

**Age-to-age factors.** For each step from period {math}`j` to {math}`j+1`, the factor is the total of the later column over the total of the earlier, over the origins that have both:

```{math}
f_j=\frac{\sum_{i}C_{i,j+1}}{\sum_{i}C_{i,j}} .
```

**Cumulative development factors.** The factor from period {math}`j` to the end is the product of the remaining age-to-age factors,

```{math}
\mathrm{CDF}_j=f_j\,f_{j+1}\cdots f_{n-2},\qquad \mathrm{CDF}_{n-1}=1 ,
```

so a triangle of {math}`n` periods has {math}`n-1` age-to-age factors and {math}`n` cumulative factors, the last being one: the final period is taken to be ultimate.

**Ultimate and reserve.** An origin whose latest observed cumulative amount is {math}`C_i`, at period {math}`j_i`, has the ultimate loss and **reserve**

```{math}
\hat U_i=C_i\,\mathrm{CDF}_{j_i},\qquad R_i=\hat U_i-C_i .
```

The reserve is what remains to be paid: it is the money held for claims that have already occurred.

```cpp
pluss::Series models::age_factors(const pluss::Series &cumulative_triangle);
pluss::Series models::ladder_cdf(const pluss::Series &age_to_age_factors);
pluss::Series models::ladder_ultimate(const pluss::Series &cumulative_triangle, const pluss::Series &cdf_to_ultimate);
pluss::Series models::ladder_reserve(const pluss::Series &cumulative_triangle, const pluss::Series &cdf_to_ultimate);
actuarial::Reserving actuarial::operators::chain_ladder(pluss::Series cdf = pluss::series::undefined());
actuarial::ReserveResult actuarial::Reserving::operator()(const actuarial::Triangle &triangle) const;
```

| Function or type | Meaning |
|---|---|
| `age_factors(triangle)` | the `n − 1` age-to-age factors from a two-dimensional cumulative triangle with zero padding |
| `ladder_cdf(factors)` | the `n` cumulative development factors, ending in 1 |
| `ladder_ultimate(triangle, cdf)` | the ultimate loss of each origin |
| `ladder_reserve(triangle, cdf)` | the ultimate less the latest observed cumulative amount, by origin |
| `operators::chain_ladder(cdf)` | a `Reserving` operator; with no argument it estimates the factors from the triangle it is applied to, and with a `cdf` it uses those |
| `ReserveResult{age_to_age, cdf, ultimate, reserve}` | what the operator returns: the factors, the cumulative factors, and the ultimate and reserve by origin |

The operator takes a `Triangle` (see [Claims Triangles](claims_triangles.md)); the functions take the triangle as a two-dimensional `Series` with zero padding.

## The method step by step

The triangle of the previous page. The program computes the four age-to-age factors, the five cumulative factors, and the ultimate loss and the reserve of every origin year: the oldest year is fully developed and has no reserve, and the most recent has the largest, 208.2 on a latest cumulative of 140.

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
  const pluss::Series factors = models::age_factors(triangle);
  const pluss::Series cdf = models::ladder_cdf(factors);
  out.writeln("age-to-age factors =", factors.items<double>());
  out.writeln("cumulative factors =", cdf.items<double>());
  out.writeln("ultimate           =", models::ladder_ultimate(triangle, cdf).items<double>());
  out.writeln("reserve            =", models::ladder_reserve(triangle, cdf).items<double>());
  out.writeln("total reserve      =", models::ladder_reserve(triangle, cdf).sum().item<double>());
}
```

Output

```text
age-to-age factors = [1.804348, 1.218487, 1.086022, 1.041667]
cumulative factors = [2.487187, 1.378441, 1.131272, 1.041667, 1.000000]
ultimate           = [250.000000, 276.041667, 294.130824, 323.933673, 348.206224]
reserve            = [0.000000, 11.041667, 34.130824, 88.933673, 208.206224]
total reserve      = 342.312388
```

## As an operator

The `Reserving` operator does the same from a `Triangle` and returns everything in one `ReserveResult`. It is created once and applied to any triangle, for instance to several lines of business.

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
  const actuarial::Reserving chain_ladder = actuarial::operators::chain_ladder();
  const actuarial::ReserveResult result = chain_ladder(t);
  out.writeln("age_to_age =", result.age_to_age.items<double>());
  out.writeln("cdf        =", result.cdf.items<double>());
  out.writeln("ultimate   =", result.ultimate.items<double>());
  out.writeln("reserve    =", result.reserve.items<double>());
}
```

Output

```text
age_to_age = [1.804348, 1.218487, 1.086022, 1.041667]
cdf        = [2.487187, 1.378441, 1.131272, 1.041667, 1.000000]
ultimate   = [250.000000, 276.041667, 294.130824, 323.933673, 348.206224]
reserve    = [0.000000, 11.041667, 34.130824, 88.933673, 208.206224]
```

## Chosen factors

Estimated factors are only history. An actuary often judges that the future will differ, or that a factor is too noisy to use, and supplies the cumulative development factors. The operator takes them, one per development period, and applies them to the triangle in place of the estimated ones. Here the factors are smoothed to 1.5, 1.2, 1.1, 1.05 and 1: they apply to the latest cumulative amount of each origin according to its latest period.

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
  const pluss::Series chosen = pluss::make_series({1.5, 1.2, 1.1, 1.05, 1.0});
  const actuarial::ReserveResult result = actuarial::operators::chain_ladder(chosen)(t);
  out.writeln("ultimate with chosen factors =", result.ultimate.items<double>());
  out.writeln("reserve with chosen factors  =", result.reserve.items<double>());
  out.writeln("the latest cumulative amounts are 250, 265, 260, 235, 140; the second origin: 265 * 1.05 =", 265.0 * 1.05);
}
```

Output

```text
ultimate with chosen factors = [250.000000, 278.250000, 286.000000, 282.000000, 210.000000]
reserve with chosen factors  = [0.000000, 13.250000, 26.000000, 47.000000, 70.000000]
the latest cumulative amounts are 250, 265, 260, 235, 140; the second origin: 265 * 1.05 = 278.250000
```

## Sensitivity to one factor

A reserve is a product of factors, so an error in one carries into every origin that still has to pass through that step. The program raises the first age-to-age factor from the estimated 1.80 by 5% and prints the change in the reserve of each origin; only the origin that has not yet passed the first step, the most recent, is affected. Its ultimate loss rises by exactly 5%, and its reserve, which is the ultimate less the 140 already paid, rises by more than 5%: by 17.4 on 208.2.

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
  const pluss::Series factors = models::age_factors(triangle);
  auto shifted = factors.items<double>();
  shifted[0] *= 1.05;
  const pluss::Series base = models::ladder_reserve(triangle, models::ladder_cdf(factors));
  const pluss::Series more = models::ladder_reserve(triangle, models::ladder_cdf(pluss::make_series(shifted)));
  out.writeln("reserve            =", base.items<double>());
  out.writeln("reserve, first +5% =", more.items<double>());
  out.writeln("change             =", more.sub(base).items<double>());
}
```

Output

```text
reserve            = [0.000000, 11.041667, 34.130824, 88.933673, 208.206224]
reserve, first +5% = [0.000000, 11.041667, 34.130824, 88.933673, 225.616535]
change             = [0.000000, 0.000000, 0.000000, 0.000000, 17.410311]
```

## Invalid input fails explicitly

The factors must be estimable: every development step needs an observed pair of cells. Chosen factors must be one per development period, and the triangle must be defined.

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
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0, 250.0,
                                                    110.0, 200.0, 245.0, 265.0, 0.0,
                                                    120.0, 215.0, 260.0, 0.0, 0.0,
                                                    130.0, 235.0, 0.0, 0.0, 0.0,
                                                    140.0, 0.0, 0.0, 0.0, 0.0}, {5, 5});
  const actuarial::Triangle t = actuarial::triangle::cumulative(triangle);
  report([&] { (void)models::age_factors(pluss::make_series({0.0, 0.0, 0.0, 0.0}, {2, 2})); });
  report([&] { (void)actuarial::operators::chain_ladder(pluss::make_series({1.5, 1.2}))(t); });
  report([&] { (void)actuarial::operators::chain_ladder()(actuarial::Triangle{}); });
}
```

Output

```text
chain_ladder: each development pair needs observed exposure
chain_ladder: cdf length must match development count
chain_ladder: triangle must be defined
```

## Where to go next

A triangle with little history in an origin year gives a noisy chain ladder reserve. Combining it with an expected loss gives a steadier one ([Bornhuetter-Ferguson](bornhuetter_ferguson.md)).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="claims_triangles.html">
    <span class="label">Previous</span>
    <span class="title">Claims Triangles</span>
  </a>
  <a class="next-link" href="bornhuetter_ferguson.html">
    <span class="label">Next</span>
    <span class="title">Bornhuetter-Ferguson</span>
  </a>
</div>
<!-- apiref:nav:end -->
