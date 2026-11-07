<!-- generated from the_cos_pricer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The COS pricer

[Back to Heston and Bates](../finance_03_heston_and_bates.md)

The COS method (Fang and Oosterlee) expands the density of ln(S_T / K) in a cosine series on a finite interval [a, b]. The coefficients of the series are values of the characteristic function, and the payoff of a call or put has closed-form cosine coefficients, so the price is a short sum. Its error falls exponentially in the number of terms for smooth densities, which makes it the most accurate of the Heston pricers for a given effort.

```cpp
double prc::heston_price_cos(type, spot, strike, rate, dividend_yield, maturity, const vol::HestonParams &, const prc::CosOptions & = {}, Config = {});
Series prc::heston_price_cos(type, const Series &spot, const Series &strike, rate, dividend_yield, const Series &maturity, const vol::HestonParams &, const prc::CosOptions & = {}, Config = {});
```

| `CosOptions` | Meaning | Default |
|---|---|---|
| `num_terms` | the number of cosine terms; at least 2 | 1024 |
| `truncation` | the half-width L of the interval, in standard deviations of ln S_T | 12 |
| `cumulant_step` | the finite-difference step used to estimate the mean and variance of ln S_T from a characteristic function that is supplied as a function ([Generic characteristic functions](generic_characteristic_functions.md)); at least 1e-6 | 1e-3 |

## Terms and truncation

Too few terms cannot represent the density, and an interval that is too narrow cuts off its tails; beyond those thresholds the price is converged to about 10⁻⁸. The program prints the error against the integration price.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  const double spot = 100.0, strike = 100.0, rate = 0.03, dividend = 0.01, maturity = 1.0;
  const vol::HestonParams params;
  const double exact = prc::heston_price(prc::Call, spot, strike, rate, dividend, maturity, params);
  out.print_defaults(PrintOptions{}.fixed(8));
  out.writeln("reference (integration)", exact);
  out.print_defaults(PrintOptions{}.sci(2));
  out.writeln("num_terms:");
  for (std::int64_t terms : {16, 32, 64, 128, 256, 1024}) {
    prc::CosOptions o;
    o.num_terms = terms;
    out.writeln("  ", terms, " error", std::abs(prc::heston_price_cos(prc::Call, spot, strike, rate, dividend, maturity, params, o) - exact));
  }
  out.writeln("truncation (half-width in standard deviations):");
  for (double L : {1.0, 2.0, 4.0, 8.0, 12.0, 40.0}) {
    prc::CosOptions o;
    o.truncation = L;
    out.writeln("  ", L, " error", std::abs(prc::heston_price_cos(prc::Call, spot, strike, rate, dividend, maturity, params, o) - exact));
  }
}
```

Output

```text
reference (integration) 8.54225698
num_terms:
   16  error 7.14e+00
   32  error 4.50e-01
   64  error 7.11e-03
   128  error 3.07e-06
   256  error 1.22e-08
   1024  error 1.22e-08
truncation (half-width in standard deviations):
   1.00e+00  error 1.91e-01
   2.00e+00  error 2.31e-03
   4.00e+00  error 2.70e-07
   8.00e+00  error 1.22e-08
   1.20e+01  error 1.22e-08
   4.00e+01  error 2.28e-08
```

The error falls steeply with the number of terms: 7.1 at 16 terms, 0.45 at 32, 0.007 at 64, 3 × 10⁻⁶ at 128, and 10⁻⁸ from 256 terms on, which is the accuracy of the reference. Likewise for the interval: a half-width of one standard deviation cuts off too much (0.19), two give 2.3 × 10⁻³, four give 3 × 10⁻⁷, and from eight on the error is 10⁻⁸ (a very wide interval, 40, wastes terms and is slightly worse).

## Strikes and maturities

The method is accurate across strikes, including far from the money, and for long maturities, where the integration pricer in its original form fails. The program prices calls over strikes and maturities with COS and with the integration pricer.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const vol::HestonParams params;
  out.writeln("maturity strike   COS         integration    difference");
  for (double maturity : {0.1, 1.0, 5.0}) {
    for (double strike : {50.0, 100.0, 200.0}) {
      const double cos = prc::heston_price_cos(prc::Call, 100.0, strike, 0.03, 0.01, maturity, params);
      const double integral = prc::heston_price(prc::Call, 100.0, strike, 0.03, 0.01, maturity, params);
      out.writeln(maturity, strike, cos, integral, cos - integral);
    }
  }
}
```

Output

```text
maturity strike   COS         integration    difference
0.100000 50.000000 50.049825 50.049825 0.000000
0.100000 100.000000 2.599695 2.599695 0.000000
0.100000 200.000000 0.000000 0.000000 0.000000
1.000000 50.000000 50.526636 50.526635 0.000000
1.000000 100.000000 8.542257 8.542257 0.000000
1.000000 200.000000 0.000014 0.000014 0.000000
5.000000 50.000000 52.998567 52.998566 0.000000
5.000000 100.000000 20.564342 20.564342 0.000000
5.000000 200.000000 0.863249 0.863249 0.000000
```

COS and integration agree to six decimals at every strike and maturity, including a strike of 200 where the option is worth 1.4 × 10⁻⁵ and a maturity of five years, where the original integration form fails.

## Batches

The `Series` overload prices element *i* of spots, strikes and maturities together. The three inputs must have one common length, with an input of length one repeated against the others, and the result has that length. A surface is priced in one call by listing its points. A maturity of zero gives the intrinsic value, and a mismatch of lengths is an error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const vol::HestonParams params;
  const auto spot = pluss::make_series(std::vector<double>{90.0, 100.0, 110.0}, {3}, as_f64());
  const auto strike = pluss::make_series(std::vector<double>{100.0, 100.0, 100.0}, {3}, as_f64());
  const auto maturity = pluss::make_series(std::vector<double>{0.5, 1.0, 2.0}, {3}, as_f64());

  out.writeln("series COS ", prc::heston_price_cos(prc::Call, spot, strike, 0.03, 0.01, maturity, params).pull<double>());
  out.writeln("one by one ", prc::heston_price_cos(prc::Call, 90.0, 100.0, 0.03, 0.01, 0.5, params), prc::heston_price_cos(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, params), prc::heston_price_cos(prc::Call, 110.0, 100.0, 0.03, 0.01, 2.0, params));

  // A strike ladder at one spot and one maturity: the length-one inputs are repeated.
  const auto ladder = pluss::make_series(std::vector<double>{80.0, 90.0, 100.0, 110.0, 120.0}, {5}, as_f64());
  const auto one_spot = pluss::make_series(std::vector<double>{100.0}, {1}, as_f64());
  const auto one_maturity = pluss::make_series(std::vector<double>{1.0}, {1}, as_f64());
  out.writeln("strike ladder", prc::heston_price_cos(prc::Call, one_spot, ladder, 0.03, 0.01, one_maturity, params).pull<double>());

  out.writeln("zero maturity in the first element", prc::heston_price_cos(prc::Call, spot, strike, 0.03, 0.01, pluss::make_series(std::vector<double>{0.0, 1.0, 2.0}, {3}, as_f64()), params).pull<double>());
  try {
    prc::heston_price_cos(prc::Call, spot, strike, 0.03, 0.01, pluss::make_series(std::vector<double>{0.5, 1.0}, {2}, as_f64()), params);
  } catch (const std::exception &e) {
    const std::string message = e.what();
    const auto start = message.find("option_price_cos: ") + 18;
    out.writeln("lengths 3, 3 and 2 ->", message.substr(start, message.find('\n') - start));
  }
}
```

Output

```text
series COS  [1.416390, 8.542257, 19.611113]
one by one  1.416390 8.542257 19.611113
strike ladder [22.831101, 14.963291, 8.542257, 4.019179, 1.476924]
zero maturity in the first element [0.000000, 8.542257, 19.611113]
lengths 3, 3 and 2 -> spot, strike and maturity must have the same length (or length 1)
```

The batch equals the three scalar prices (1.4164, 8.5423 and 19.6111). A single spot and maturity are repeated against a ladder of five strikes, which prices the strike ladder in one call. A zero maturity gives the intrinsic value for its element (0 for spot 90 and strike 100), and lengths of 3, 3 and 2 are rejected.

## Invalid settings

`num_terms` below 2, a non-positive `truncation`, and a `cumulant_step` below 1e-6 are errors.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  auto attempt = [&](const char *label, auto edit) {
    prc::CosOptions options;
    edit(options);
    try {
      prc::heston_price_cos(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, vol::HestonParams{}, options);
      out.writeln(label, "-> no error");
    } catch (const std::exception &e) {
      const std::string message = e.what();
      const auto start = message.find("heston_price_cos: ") + 18;
      out.writeln(label, "->", message.substr(start, message.find('\n') - start));
    }
  };
  attempt("num_terms = 1     ", [](prc::CosOptions &o) { o.num_terms = 1; });
  attempt("truncation = 0    ", [](prc::CosOptions &o) { o.truncation = 0.0; });
  attempt("cumulant_step = 0 ", [](prc::CosOptions &o) { o.cumulant_step = 0.0; });
  attempt("cumulant_step = 1e-8", [](prc::CosOptions &o) { o.cumulant_step = 1e-8; });
}
```

Output

```text
num_terms = 1      -> num_terms must be >= 2
truncation = 0     -> truncation must be finite and > 0
cumulant_step = 0  -> cumulant_step must be finite and >= 1e-6
cumulant_step = 1e-8 -> cumulant_step must be finite and >= 1e-6
```

Each setting is rejected with its message. A `cumulant_step` below 1e-6 is refused because the variance, a second difference of the characteristic function, would be lost to rounding.
