<!-- generated from a_reserving_review.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A Reserving Review

[Back to Worked Problems](../actuarial_05_worked_problems.md)

**The problem.** A commercial-lines insurer has six accident years of cumulative reported claims, evaluated at 12-month intervals up to the end of 2025, with earned premium for each year. It must set the reserve for claims already incurred but not yet reported or settled. The review has five parts: examine **how stable the development pattern is**, estimate the reserve by **chain ladder**, allow for a **tail** beyond the last observed age, estimate it again by **Bornhuetter-Ferguson** from an expected loss ratio, and decide which to rely on for each year and **how much the answer depends on the choices made**.

This is the standard structure of a reserving analysis, and it uses everything in [Actuarial 4](../actuarial_04_ratemaking_credibility_and_loss_reserving.md): the labelled `Triangle`, age-to-age factors, the chain-ladder and Bornhuetter-Ferguson estimators, and their relation to credibility.

## The plan

The cumulative claims of origin year {math}`i` at development age {math}`j` are {math}`C_{i,j}`. Chain ladder projects each year's latest figure by the factors of earlier years; it assumes that the future will develop like the past, and it is only as good as that assumption. Bornhuetter-Ferguson instead takes an a priori ultimate {math}`U_i=\text{premium}_i\times\text{expected loss ratio}` and adds to the reported amount the unreported share of it. The two are linked: with {math}`p_i=1/\mathrm{CDF}_i` the proportion reported,

```{math}
\text{BF}_i=p_i\,\underbrace{\frac{C_i}{p_i}}_{\text{chain ladder}}+(1-p_i)\,U_i ,
```

so Bornhuetter-Ferguson is **a credibility-weighted average of the chain-ladder ultimate and the a priori ultimate with weight {math}`p_i`**, the proportion reported: mature years lean on the data, immature ones on the prior. Seeing that identity hold on real numbers makes the choice of method much easier to defend.

## Step 1: is the pattern stable?

Chain ladder multiplies by a single factor for each age, so it matters whether the individual years agree about that factor. The program prints the link ratio, {math}`C_{i,j+1}/C_{i,j}`, for every origin and age where both are observed, and then three ways of averaging them: the volume-weighted average the library estimates, the simple average, and the average of the latest three years. If the three averages differ materially, or the ratios in a column trend, the pattern is not stable and the choice of average matters.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series triangle = pluss::make_series({1000.0, 1900.0, 2350.0, 2550.0, 2650.0, 2700.0,
                                                     1100.0, 2050.0, 2550.0, 2800.0, 2900.0, 0.0,
                                                     1150.0, 2250.0, 2800.0, 3050.0, 0.0, 0.0,
                                                     1300.0, 2500.0, 3150.0, 0.0, 0.0, 0.0,
                                                     1450.0, 2900.0, 0.0, 0.0, 0.0, 0.0,
                                                     1480.0, 0.0, 0.0, 0.0, 0.0, 0.0}, {6, 6});
  const actuarial::Triangle data = actuarial::triangle::cumulative(triangle, pluss::series::undefined(), {"2020", "2021", "2022", "2023", "2024", "2025"}, {"12", "24", "36", "48", "60", "72"});
  const pluss::Series premium = pluss::make_series({4000.0, 4400.0, 4800.0, 5300.0, 5900.0, 6400.0});
  const std::vector<double> latest = {2700.0, 2900.0, 3050.0, 3150.0, 2900.0, 1480.0};
  const pluss::Series reported = pluss::make_series(latest);
  const auto row = [&](const char *name, const pluss::Series &s) {
    out.write(name);
    for (const double v : s.items<double>()) out.write(" ", v);
    out.writeln();
  };
  const auto v = triangle.reshape({36}).items<double>();
  out.writeln("link ratios by origin (rows) and age step (columns 12-24, 24-36, 36-48, 48-60, 60-72)");
  std::vector<std::vector<double>> by_age(5);
  for (int i = 0; i < 6; ++i) {
    out.write(2020 + i, ":");
    for (int j = 0; j < 5; ++j) {
      if (v[6 * i + j + 1] > 0.0) {
        const double f = v[6 * i + j + 1] / v[6 * i + j];
        by_age[j].push_back(f);
        out.write(" ", f);
      } else {
        out.write("       -");
      }
    }
    out.writeln();
  }
  const pluss::Series volume = models::age_factors(triangle);
  out.write("volume-weighted:");
  for (const double f : volume.items<double>()) out.write(" ", f);
  out.writeln();
  out.write("simple average:  ");
  for (const auto &col : by_age) { double s = 0.0; for (const double f : col) s += f; out.write(" ", s / static_cast<double>(col.size())); }
  out.writeln();
  out.write("latest three:    ");
  for (const auto &col : by_age) { double s = 0.0; const std::size_t n = std::min<std::size_t>(3, col.size()); for (std::size_t k = col.size() - n; k < col.size(); ++k) s += col[k]; out.write(" ", s / static_cast<double>(n)); }
  out.writeln();
}
```

Output

```text
link ratios by origin (rows) and age step (columns 12-24, 24-36, 36-48, 48-60, 60-72)
2020 :  1.9000  1.2368  1.0851  1.0392  1.0189
2021 :  1.8636  1.2439  1.0980  1.0357       -
2022 :  1.9565  1.2444  1.0893       -       -
2023 :  1.9231  1.2600       -       -       -
2024 :  2.0000       -       -       -       -
2025 :       -       -       -       -       -
volume-weighted:  1.9333  1.2471  1.0909  1.0374  1.0189
simple average:    1.9286  1.2463  1.0908  1.0375  1.0189
latest three:      1.9599  1.2494  1.0908  1.0375  1.0189
```

## Step 2: the chain-ladder reserve

With the volume-weighted factors the operator returns the cumulative factors, the ultimate and the reserve of each year. The program also reports the percentage of the ultimate already reported, the reciprocal of the cumulative factor, which says how mature each year is and, by the identity above, how much the Bornhuetter-Ferguson method will lean on the prior later.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  const pluss::Series triangle = pluss::make_series({1000.0, 1900.0, 2350.0, 2550.0, 2650.0, 2700.0,
                                                     1100.0, 2050.0, 2550.0, 2800.0, 2900.0, 0.0,
                                                     1150.0, 2250.0, 2800.0, 3050.0, 0.0, 0.0,
                                                     1300.0, 2500.0, 3150.0, 0.0, 0.0, 0.0,
                                                     1450.0, 2900.0, 0.0, 0.0, 0.0, 0.0,
                                                     1480.0, 0.0, 0.0, 0.0, 0.0, 0.0}, {6, 6});
  const actuarial::Triangle data = actuarial::triangle::cumulative(triangle, pluss::series::undefined(), {"2020", "2021", "2022", "2023", "2024", "2025"}, {"12", "24", "36", "48", "60", "72"});
  const pluss::Series premium = pluss::make_series({4000.0, 4400.0, 4800.0, 5300.0, 5900.0, 6400.0});
  const std::vector<double> latest = {2700.0, 2900.0, 3050.0, 3150.0, 2900.0, 1480.0};
  const pluss::Series reported = pluss::make_series(latest);
  const auto row = [&](const char *name, const pluss::Series &s) {
    out.write(name);
    for (const double v : s.items<double>()) out.write(" ", v);
    out.writeln();
  };
  const actuarial::ReserveResult cl = actuarial::operators::chain_ladder()(data);
  row("age-to-age factors   ", cl.age_to_age);
  row("cumulative factors   ", cl.cdf);
  out.writeln("origin    reported   ultimate   reserve   percent reported");
  const auto ult = cl.ultimate.items<double>(), res = cl.reserve.items<double>(), cdf = cl.cdf.items<double>();
  for (int i = 0; i < 6; ++i) out.writeln(2020 + i, "   ", latest[i], "   ", ult[i], "   ", res[i], "   ", 1.0 / cdf[5 - i]);
  out.writeln("total reported", reported.sum().item<double>(), " total ultimate", cl.ultimate.sum().item<double>(), " total reserve", cl.reserve.sum().item<double>());
}
```

Output

```text
age-to-age factors     1.933  1.247  1.091  1.037  1.019
cumulative factors     2.780  1.438  1.153  1.057  1.019  1.000
origin    reported   ultimate   reserve   percent reported
2020     2700.000     2700.000     0.000     1.000
2021     2900.000     2954.717     54.717     0.981
2022     3050.000     3223.717     173.717     0.946
2023     3150.000     3632.087     482.087     0.867
2024     2900.000     4170.174     1270.174     0.695
2025     1480.000     4114.571     2634.571     0.360
total reported 16180.000  total ultimate 20795.266  total reserve 4615.266
```

## Step 3: a tail factor

The triangle stops at 72 months, but claims in this line keep developing after that. The library's chain-ladder estimator ends the pattern at 1 by definition, so a tail beyond the last age is applied as a further multiplication of the cumulative factors. The program shows the effect of tail factors of 1.00, 1.02 and 1.05 on the total reserve, and the reserve of the oldest year, which has no reserve at all unless a tail is allowed.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  const pluss::Series triangle = pluss::make_series({1000.0, 1900.0, 2350.0, 2550.0, 2650.0, 2700.0,
                                                     1100.0, 2050.0, 2550.0, 2800.0, 2900.0, 0.0,
                                                     1150.0, 2250.0, 2800.0, 3050.0, 0.0, 0.0,
                                                     1300.0, 2500.0, 3150.0, 0.0, 0.0, 0.0,
                                                     1450.0, 2900.0, 0.0, 0.0, 0.0, 0.0,
                                                     1480.0, 0.0, 0.0, 0.0, 0.0, 0.0}, {6, 6});
  const actuarial::Triangle data = actuarial::triangle::cumulative(triangle, pluss::series::undefined(), {"2020", "2021", "2022", "2023", "2024", "2025"}, {"12", "24", "36", "48", "60", "72"});
  const pluss::Series premium = pluss::make_series({4000.0, 4400.0, 4800.0, 5300.0, 5900.0, 6400.0});
  const std::vector<double> latest = {2700.0, 2900.0, 3050.0, 3150.0, 2900.0, 1480.0};
  const pluss::Series reported = pluss::make_series(latest);
  const auto row = [&](const char *name, const pluss::Series &s) {
    out.write(name);
    for (const double v : s.items<double>()) out.write(" ", v);
    out.writeln();
  };
  const actuarial::ReserveResult cl = actuarial::operators::chain_ladder()(data);
  out.writeln("tail    reserve of 2020    total reserve    increase over no tail");
  const double base = cl.reserve.sum().item<double>();
  for (const double tail : {1.00, 1.02, 1.05}) {
    const pluss::Series ultimate = cl.ultimate.mul(tail);
    const pluss::Series reserve = ultimate.sub(reported);
    out.writeln(tail, "   ", reserve.items<double>()[0], "   ", reserve.sum().item<double>(), "   ", reserve.sum().item<double>() / base - 1.0);
  }
}
```

Output

```text
tail    reserve of 2020    total reserve    increase over no tail
1.000     0.000     4615.266     0.000
1.020     54.000     5031.171     0.090
1.050     135.000     5655.029     0.225
```

## Step 4: Bornhuetter-Ferguson, and the identity

The a priori ultimates are the earned premiums times an expected loss ratio of 70%. The share reported comes from the chain-ladder pattern: the reciprocals of the cumulative factors, matched to each year's latest age. The program computes the Bornhuetter-Ferguson ultimate and reserve, and then verifies the identity numerically: the credibility-weighted combination of the chain-ladder and a priori ultimates, weighted by the share reported, is the Bornhuetter-Ferguson ultimate.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  const pluss::Series triangle = pluss::make_series({1000.0, 1900.0, 2350.0, 2550.0, 2650.0, 2700.0,
                                                     1100.0, 2050.0, 2550.0, 2800.0, 2900.0, 0.0,
                                                     1150.0, 2250.0, 2800.0, 3050.0, 0.0, 0.0,
                                                     1300.0, 2500.0, 3150.0, 0.0, 0.0, 0.0,
                                                     1450.0, 2900.0, 0.0, 0.0, 0.0, 0.0,
                                                     1480.0, 0.0, 0.0, 0.0, 0.0, 0.0}, {6, 6});
  const actuarial::Triangle data = actuarial::triangle::cumulative(triangle, pluss::series::undefined(), {"2020", "2021", "2022", "2023", "2024", "2025"}, {"12", "24", "36", "48", "60", "72"});
  const pluss::Series premium = pluss::make_series({4000.0, 4400.0, 4800.0, 5300.0, 5900.0, 6400.0});
  const std::vector<double> latest = {2700.0, 2900.0, 3050.0, 3150.0, 2900.0, 1480.0};
  const pluss::Series reported = pluss::make_series(latest);
  const auto row = [&](const char *name, const pluss::Series &s) {
    out.write(name);
    for (const double v : s.items<double>()) out.write(" ", v);
    out.writeln();
  };
  const actuarial::ReserveResult cl = actuarial::operators::chain_ladder()(data);
  const pluss::Series prior = premium.mul(0.70);
  std::vector<double> share(6);
  const auto cdf = cl.cdf.items<double>();
  for (int i = 0; i < 6; ++i) share[i] = 1.0 / cdf[5 - i];
  const pluss::Series pct = pluss::make_series(share);
  const actuarial::ReserveResult bf = actuarial::operators::bornhuetter_ferguson(prior, pct)(data);
  row("a priori ultimate    ", prior);
  row("share reported       ", pct);
  row("chain-ladder ultimate", cl.ultimate);
  row("BF ultimate          ", bf.ultimate);
  row("credibility mix      ", pct.mul(cl.ultimate).add(pct.mul(-1.0).add(1.0).mul(prior)));
  row("BF reserve           ", bf.reserve);
  out.writeln("total reserve: chain ladder", cl.reserve.sum().item<double>(), " Bornhuetter-Ferguson", bf.reserve.sum().item<double>());
}
```

Output

```text
a priori ultimate      2800.000  3080.000  3360.000  3710.000  4130.000  4480.000
share reported         1.000  0.981  0.946  0.867  0.695  0.360
chain-ladder ultimate  2700.000  2954.717  3223.717  3632.087  4170.174  4114.571
BF ultimate            2700.000  2957.037  3231.061  3642.428  4157.937  4348.556
credibility mix        2700.000  2957.037  3231.061  3642.428  4157.937  4348.556
BF reserve             0.000  57.037  181.061  492.428  1257.937  2868.556
total reserve: chain ladder 4615.266  Bornhuetter-Ferguson 4857.020
```

## Step 5: how sensitive is the answer?

Two questions decide how much to trust the numbers. First, if the latest diagonal had come in 20% higher for the youngest year, how much would each method's reserve for that year move? Second, how far do the choices of average move the total chain-ladder reserve? The program answers both: the first by re-running the estimators with the new first diagonal figure for 2025, the second by using the simple and latest-three averages of Step 1 as the pattern.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  const pluss::Series triangle = pluss::make_series({1000.0, 1900.0, 2350.0, 2550.0, 2650.0, 2700.0,
                                                     1100.0, 2050.0, 2550.0, 2800.0, 2900.0, 0.0,
                                                     1150.0, 2250.0, 2800.0, 3050.0, 0.0, 0.0,
                                                     1300.0, 2500.0, 3150.0, 0.0, 0.0, 0.0,
                                                     1450.0, 2900.0, 0.0, 0.0, 0.0, 0.0,
                                                     1480.0, 0.0, 0.0, 0.0, 0.0, 0.0}, {6, 6});
  const actuarial::Triangle data = actuarial::triangle::cumulative(triangle, pluss::series::undefined(), {"2020", "2021", "2022", "2023", "2024", "2025"}, {"12", "24", "36", "48", "60", "72"});
  const pluss::Series premium = pluss::make_series({4000.0, 4400.0, 4800.0, 5300.0, 5900.0, 6400.0});
  const std::vector<double> latest = {2700.0, 2900.0, 3050.0, 3150.0, 2900.0, 1480.0};
  const pluss::Series reported = pluss::make_series(latest);
  const auto row = [&](const char *name, const pluss::Series &s) {
    out.write(name);
    for (const double v : s.items<double>()) out.write(" ", v);
    out.writeln();
  };
  const pluss::Series prior = premium.mul(0.70);
  const actuarial::ReserveResult cl = actuarial::operators::chain_ladder()(data);
  std::vector<double> share(6);
  const auto cdf = cl.cdf.items<double>();
  for (int i = 0; i < 6; ++i) share[i] = 1.0 / cdf[5 - i];
  const pluss::Series pct = pluss::make_series(share);

  const auto shifted_values = triangle.reshape({36}).items<double>();
  std::vector<double> bumped = shifted_values;
  bumped[30] *= 1.20;  // 2025, age 12
  const actuarial::Triangle data_up = actuarial::triangle::cumulative(pluss::make_series(bumped, {6, 6}));
  const actuarial::ReserveResult cl_up = actuarial::operators::chain_ladder()(data_up);
  const actuarial::ReserveResult bf_up = actuarial::operators::bornhuetter_ferguson(prior, pct)(data_up);
  const actuarial::ReserveResult bf = actuarial::operators::bornhuetter_ferguson(prior, pct)(data);
  out.writeln("2025 reserve if the first diagonal figure is 20% higher:");
  out.writeln("  chain ladder:", cl.reserve.items<double>()[5], "->", cl_up.reserve.items<double>()[5], " change", cl_up.reserve.items<double>()[5] - cl.reserve.items<double>()[5]);
  out.writeln("  Bornhuetter-Ferguson:", bf.reserve.items<double>()[5], "->", bf_up.reserve.items<double>()[5], " change", bf_up.reserve.items<double>()[5] - bf.reserve.items<double>()[5]);

  const auto v = triangle.reshape({36}).items<double>();
  std::vector<std::vector<double>> by_age(5);
  for (int i = 0; i < 6; ++i)
    for (int j = 0; j < 5; ++j)
      if (v[6 * i + j + 1] > 0.0) by_age[j].push_back(v[6 * i + j + 1] / v[6 * i + j]);
  std::vector<double> simple, last3;
  for (const auto &col : by_age) {
    double s = 0.0, s3 = 0.0;
    for (const double f : col) s += f;
    const std::size_t n = std::min<std::size_t>(3, col.size());
    for (std::size_t k = col.size() - n; k < col.size(); ++k) s3 += col[k];
    simple.push_back(s / static_cast<double>(col.size()));
    last3.push_back(s3 / static_cast<double>(n));
  }
  const auto total = [&](const pluss::Series &factors) { return models::ladder_reserve(triangle, models::ladder_cdf(factors)).sum().item<double>(); };
  out.writeln("total chain-ladder reserve by pattern:");
  out.writeln("  volume-weighted:", total(models::age_factors(triangle)));
  out.writeln("  simple average :", total(pluss::make_series(simple)));
  out.writeln("  latest three   :", total(pluss::make_series(last3)));
}
```

Output

```text
2025 reserve if the first diagonal figure is 20% higher:
  chain ladder: 2634.571 -> 3161.486  change 526.914
  Bornhuetter-Ferguson: 2868.556 -> 2868.556  change 0.000
total chain-ladder reserve by pattern:
  volume-weighted: 4615.266
  simple average : 4599.907
  latest three   : 4687.383
```

## Reading the results

**The pattern.** The link ratios are tight where there is a lot of data and loose at the first step. Between 12 and 24 months they run from 1.864 to 2.000, a spread of 7%; between 24 and 36 months from 1.237 to 1.260; between 36 and 48 from 1.085 to 1.098; between 48 and 60 from 1.036 to 1.039. The three averages agree almost exactly from the second step on, and differ only at the first, where the latest three years average 1.960 against the volume-weighted 1.933, a 1.4% gap caused by the last year's 2.000. The factors are stable enough for chain ladder to be meaningful, and the early pattern is where the judgement lies.

**Chain ladder.** The cumulative factors are 2.780, 1.438, 1.153, 1.057, 1.019 and 1.000. The oldest year is fully developed (reserve 0), and the reserves grow with immaturity: 54.7, 173.7, 482.1, 1,270.2 and **2,634.6 for 2025**, which is only 36.0% reported. The total reserve is **4,615.3**, 28.5% of the 16,180 reported so far, on a total ultimate of 20,795.3. Over half the reserve (57%) belongs to the youngest year, whose estimate is a 1,480 first-year figure multiplied by 2.78, so it carries all the uncertainty of one data point and the first link ratio.

**The tail.** The oldest year has no reserve only because the estimator ends the pattern at 1. A tail factor of 1.02 adds 54 to 2020 and raises the total reserve by 9.0% to 5,031.2; a tail of 1.05 adds 135 to 2020 and raises the total by 22.5% to 5,655.0. The tail factor is a single number not observed in this data, and it moves the reserve more than any other choice in the review. It needs its own evidence, from older triangles or industry patterns, and a range shown to the person signing the reserve.

**Bornhuetter-Ferguson.** With a 70% expected loss ratio, the a priori ultimates are 2,800 to 4,480. The Bornhuetter-Ferguson ultimates are 2,700, 2,957.0, 3,231.1, 3,642.4, 4,157.9 and 4,348.6, and the **credibility mix reproduces them to every printed digit**: the identity holds, so the Bornhuetter-Ferguson ultimate really is the chain-ladder ultimate weighted by the proportion reported plus the prior weighted by the rest. The two methods agree closely for the mature years (2021: 2,954.7 against 2,957.0) and split for the youngest: for 2025 chain ladder gives 4,114.6 and Bornhuetter-Ferguson 4,348.6, because only 36% is reported, so the prior gets 64% of the weight and the prior (4,480) is higher than what the 1,480 reported figure implies. The total reserve is 4,857.0 against 4,615.3, **5.2% higher**, almost entirely from 2025 (+234.0).

**Sensitivity.** Raising the 2025 first-year figure by 20% raises the chain-ladder reserve for 2025 by 526.9, from 2,634.6 to 3,161.5, which is the same 20%: chain ladder passes the change of the observed figure straight through to the reserve. The Bornhuetter-Ferguson reserve does not move at all (2,868.6 before and after): the reserve depends only on the prior and the proportion reported, and the extra reported claims simply move into the ultimate. This is the trade between the methods: chain ladder responds fully to a bad or good year, including when the reaction is noise; Bornhuetter-Ferguson ignores the diagonal, so it will not notice if the prior is wrong. The choice of average moves the total by far less: 4,599.9 for the simple average (−0.3%) and 4,687.4 for the latest three (+1.6%) against 4,615.3 for the volume-weighted.

**A selection.** Taken together, a defensible choice relies on chain ladder for 2020 to 2023, which are at least 86.7% reported and where the two methods agree to within 0.3%; weights Bornhuetter-Ferguson more heavily for 2024 and 2025, where chain ladder multiplies a small and noisy figure by a large factor; and states the tail assumption as a range. The reserve to hold lies between 4,615 and 4,857 before any tail, and the tail alone could add roughly a fifth on top: the analysis does not produce a single number so much as show which assumptions the number depends on.

## What to carry forward

- Look at the individual link ratios before using any average of them: stability is what justifies chain ladder.
- Bornhuetter-Ferguson is the credibility-weighted combination of chain ladder and a prior, with the proportion reported as the weight; verify the identity on your own data.
- Chain ladder reacts fully to the diagonal and Bornhuetter-Ferguson not at all: use that to decide which years each should govern.
- Report the tail factor and the method as the main sources of uncertainty; the choice of average is usually minor.
- Judge maturity by the proportion reported, which is `1 / cdf`, and let it set the weights.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="a_rate_review_by_territory.html">
    <span class="label">Previous</span>
    <span class="title">A Rate Review by Territory</span>
  </a>
</div>
<!-- apiref:nav:end -->
