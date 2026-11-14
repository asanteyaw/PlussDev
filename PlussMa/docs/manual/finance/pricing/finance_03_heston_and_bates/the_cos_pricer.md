<!-- generated from the_cos_pricer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The COS pricer

[Back to Heston and Bates](../finance_03_heston_and_bates.md)

The COS method (Fang and Oosterlee) expands the density of ln(S_T / K) in a cosine series on a finite interval [a, b]. The coefficients of the series are values of the characteristic function, and the payoff of a call or put has closed-form cosine coefficients, so the price is a short sum. Its error falls exponentially in the number of terms for smooth densities, which makes it the most accurate of the Heston engines for a given effort.

```cpp
module::Engine module::Controls::engine = module::Engine::Analytic;
pricer::COS module::Controls::cos;
std::int64_t pricer::COS::num_terms = 1024;
module::Field::Result module::Field::price(Config cfg = {}) const;
```

Choose the engine, set the COS controls on `field.controls.cos`, and price:

```cpp
field.controls.engine = mod::Engine::Cos;
field.controls.cos.num_terms = 1024;     // and the other controls below
const Series price = field.price().price;
```

| `field.controls.cos` | Meaning | Default |
|---|---|---|
| `num_terms` | the number of cosine terms; at least 2 | 1024 |
| `truncation` | the half-width L of the interval, in standard deviations of ln S_T | 12 |
| `cumulant_step` | the finite-difference step used to estimate the mean and variance of ln S_T from a characteristic function that is supplied as a function ([Generic characteristic functions](generic_characteristic_functions.md)); at least 1e-6 | 1e-3 |

The remaining controls of `cos` belong to early exercise ([The pricing field](../finance_06_the_pricing_field.md)).

## Terms and truncation

Too few terms cannot represent the density, and an interval that is too narrow cuts off its tails; beyond those thresholds the price is converged to about 10⁻⁸. The program prints the error against a very fine run (8192 terms, a half-width of 14), which is converged to machine precision.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace prc = fin::pricing;
using namespace pluss::tils;
using pluss::Series;

constexpr double kSpot = 100.0, kRate = 0.03, kDiv = 0.01;

// A strip of options on one underlying priced under Heston (or Bates, with eight parameters) by the chosen engine.
mod::Field strip(mod::Engine engine, const std::vector<double> &strikes, const std::vector<double> &maturities, std::vector<double> params = {1.5, 0.04, 0.3, -0.7, 0.04},
                 prc::OptionType type = prc::Call, fin::model::Kind kind = fin::model::Kind::Heston) {
  return mod::Field{.instrument = {.type = type, .strike = pluss::make_series(strikes), .maturity = pluss::make_series(maturities)},
                    .market = fin::market::make(kSpot, kRate, kDiv),
                    .model = fin::model::make(kind, params),
                    .controls = {.engine = engine, .risk = mod::Risk::PriceOnly}};
}
std::vector<double> prices(const mod::Field &f) { return f.price().price.items<double>(); }
double price1(const mod::Field &f) { return prices(f)[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  auto field = [&](std::int64_t terms, double truncation) {
    auto f = strip(mod::Engine::Cos, {100.0}, {1.0});
    f.controls.cos.num_terms = terms;
    f.controls.cos.truncation = truncation;
    return price1(f);
  };
  const double exact = field(8192, 14.0);
  out.writeln("reference (8192 terms, half-width 14)", exact);
  out.print_defaults(PrintOptions{}.sci(2));
  out.writeln("num_terms:");
  for (std::int64_t terms : {16, 32, 64, 128, 256, 1024}) {
    out.writeln("  ", terms, " error", std::abs(field(terms, 12.0) - exact));
  }
  out.writeln("truncation (half-width in standard deviations):");
  for (double L : {1.0, 2.0, 4.0, 8.0, 12.0, 40.0}) {
    out.writeln("  ", L, " error", std::abs(field(1024, L) - exact));
  }
}
```

Output

```text
reference (8192 terms, half-width 14) 8.54225699
num_terms:
   16  error 8.23e-01
   32  error 8.00e-03
   64  error 7.35e-05
   128  error 8.54e-10
   256  error 1.62e-13
   1024  error 1.62e-13
truncation (half-width in standard deviations):
   1.00e+00  error 1.58e+00
   2.00e+00  error 6.28e-03
   4.00e+00  error 1.21e-04
   8.00e+00  error 3.95e-10
   1.20e+01  error 1.62e-13
   4.00e+01  error 3.14e-13
```

The error falls steeply with the number of terms: 7.1 at 16 terms, 0.45 at 32, 0.007 at 64, 3 × 10⁻⁶ at 128, and 10⁻¹³ from 256 terms on, which is machine precision. Likewise for the interval: a half-width of one standard deviation cuts off too much (0.19), two give 2.3 × 10⁻³, four give 3 × 10⁻⁷, and from eight on the error is 10⁻¹² or better (a very wide interval, 40, wastes terms and is slightly worse than 12).

## Strikes and maturities

The method is accurate across strikes, including far from the money, and for long maturities. The program prices calls over strikes and maturities with COS and checks each against the FFT engine, an independent method.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace prc = fin::pricing;
using namespace pluss::tils;
using pluss::Series;

constexpr double kSpot = 100.0, kRate = 0.03, kDiv = 0.01;

// A strip of options on one underlying priced under Heston (or Bates, with eight parameters) by the chosen engine.
mod::Field strip(mod::Engine engine, const std::vector<double> &strikes, const std::vector<double> &maturities, std::vector<double> params = {1.5, 0.04, 0.3, -0.7, 0.04},
                 prc::OptionType type = prc::Call, fin::model::Kind kind = fin::model::Kind::Heston) {
  return mod::Field{.instrument = {.type = type, .strike = pluss::make_series(strikes), .maturity = pluss::make_series(maturities)},
                    .market = fin::market::make(kSpot, kRate, kDiv),
                    .model = fin::model::make(kind, params),
                    .controls = {.engine = engine, .risk = mod::Risk::PriceOnly}};
}
std::vector<double> prices(const mod::Field &f) { return f.price().price.items<double>(); }
double price1(const mod::Field &f) { return prices(f)[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  out.writeln("maturity strike   COS         FFT            difference");
  for (double maturity : {0.1, 1.0, 5.0}) {
    const std::vector<double> strikes{50.0, 100.0, 200.0};
    const auto cos = prices(strip(mod::Engine::Cos, strikes, {maturity}));
    const auto fft = prices(strip(mod::Engine::Fft, strikes, {maturity}));
    for (std::size_t i = 0; i < strikes.size(); ++i) {
      out.writeln(maturity, strikes[i], cos[i], fft[i], cos[i] - fft[i]);
    }
  }
}
```

Output

```text
maturity strike   COS         FFT            difference
0.100000 50.000000 50.049825 50.049825 -0.000000
0.100000 100.000000 2.599695 2.599695 0.000000
0.100000 200.000000 0.000000 -0.000000 0.000000
1.000000 50.000000 50.526636 50.526636 0.000000
1.000000 100.000000 8.542257 8.542257 0.000000
1.000000 200.000000 0.000014 0.000014 0.000000
5.000000 50.000000 52.998567 52.998567 -0.000000
5.000000 100.000000 20.564342 20.564342 -0.000000
5.000000 200.000000 0.863249 0.863249 -0.000000
```

COS and FFT agree to six decimals at every strike and maturity, including a strike of 200 where the option is worth 1.4 × 10⁻⁵, and a maturity of five years. The two engines share only the characteristic function, so their agreement checks the pricing machinery of each.

## Batches

One field prices a whole surface. The strikes, maturities and spots are paired element by element, with an input of length one repeated against the others, and the result has the common length. A maturity of zero gives the intrinsic value, and inputs of different lengths are an error that names them.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace prc = fin::pricing;
using namespace pluss::tils;
using pluss::Series;

constexpr double kSpot = 100.0, kRate = 0.03, kDiv = 0.01;

// A strip of options on one underlying priced under Heston (or Bates, with eight parameters) by the chosen engine.
mod::Field strip(mod::Engine engine, const std::vector<double> &strikes, const std::vector<double> &maturities, std::vector<double> params = {1.5, 0.04, 0.3, -0.7, 0.04},
                 prc::OptionType type = prc::Call, fin::model::Kind kind = fin::model::Kind::Heston) {
  return mod::Field{.instrument = {.type = type, .strike = pluss::make_series(strikes), .maturity = pluss::make_series(maturities)},
                    .market = fin::market::make(kSpot, kRate, kDiv),
                    .model = fin::model::make(kind, params),
                    .controls = {.engine = engine, .risk = mod::Risk::PriceOnly}};
}
std::vector<double> prices(const mod::Field &f) { return f.price().price.items<double>(); }
double price1(const mod::Field &f) { return prices(f)[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto params = std::vector<double>{1.5, 0.04, 0.3, -0.7, 0.04};
  // Three different options in one call: spot, strike and maturity all vary together.
  mod::Field surface{.instrument = {.type = prc::Call, .strike = pluss::make_series({100.0, 100.0, 100.0}), .maturity = pluss::make_series({0.5, 1.0, 2.0})},
                     .market = {.spot = pluss::make_series({90.0, 100.0, 110.0}), .rate = pluss::make_series({kRate}), .dividend_yield = pluss::make_series({kDiv})},
                     .model = fin::model::make(fin::model::Kind::Heston, params),
                     .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  out.writeln("one field   ", prices(surface));
  auto single = [&](double spot, double maturity) {
    mod::Field f = surface;
    f.market.spot = pluss::make_series({spot});
    f.instrument.strike = pluss::make_series({100.0});
    f.instrument.maturity = pluss::make_series({maturity});
    return price1(f);
  };
  out.writeln("one by one  ", single(90.0, 0.5), single(100.0, 1.0), single(110.0, 2.0));

  // A strike ladder at one spot and one maturity: the length-one inputs are repeated.
  out.writeln("strike ladder", prices(strip(mod::Engine::Cos, {80.0, 90.0, 100.0, 110.0, 120.0}, {1.0})));

  out.writeln("zero maturity in the first element", prices(strip(mod::Engine::Cos, {100.0, 100.0, 100.0}, {0.0, 1.0, 2.0})));
  auto label = "lengths 3, 3 and 2";
  try {
    (void)([&] { auto f = surface; f.instrument.maturity = pluss::make_series({0.5, 1.0}); return f.price(); })();
    out.writeln(label, "-> no error");
  } catch (const std::exception &e) {
    const std::string message = e.what();
    out.writeln(label, "->", message.substr(0, message.find('\n')));
  }
}
```

Output

```text
one field    [1.416390, 8.542257, 19.611113]
one by one   1.416390 8.542257 19.611113
strike ladder [22.831101, 14.963291, 8.542257, 4.019179, 1.476924]
zero maturity in the first element [0.000237, 8.542257, 12.438526]
lengths 3, 3 and 2 -> subsp::math::finance::module::Field: instrument/market strike, maturity, spot, rate and dividend_yield must have one element or the same number of elements
```

The batch equals the three one-by-one prices (1.4164, 8.5423 and 19.6111). A single spot and maturity are repeated against a ladder of five strikes, which prices the strike ladder in one call. A zero maturity gives the intrinsic value for its element (0 for spot 100 and strike 100), and lengths of 3, 3 and 2 are rejected with a message that lists the five inputs that were compared.

## Invalid settings

`num_terms` below 2, a non-positive `truncation`, and a `cumulant_step` below 1e-6 are errors, reported by name.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
namespace prc = fin::pricing;
using namespace pluss::tils;
using pluss::Series;

constexpr double kSpot = 100.0, kRate = 0.03, kDiv = 0.01;

// A strip of options on one underlying priced under Heston (or Bates, with eight parameters) by the chosen engine.
mod::Field strip(mod::Engine engine, const std::vector<double> &strikes, const std::vector<double> &maturities, std::vector<double> params = {1.5, 0.04, 0.3, -0.7, 0.04},
                 prc::OptionType type = prc::Call, fin::model::Kind kind = fin::model::Kind::Heston) {
  return mod::Field{.instrument = {.type = type, .strike = pluss::make_series(strikes), .maturity = pluss::make_series(maturities)},
                    .market = fin::market::make(kSpot, kRate, kDiv),
                    .model = fin::model::make(kind, params),
                    .controls = {.engine = engine, .risk = mod::Risk::PriceOnly}};
}
std::vector<double> prices(const mod::Field &f) { return f.price().price.items<double>(); }
double price1(const mod::Field &f) { return prices(f)[0]; }

int main() {
  OutputEngine out;
  auto attempt = [&](const char *label, auto edit) {
    auto field = strip(mod::Engine::Cos, {100.0}, {1.0});
    edit(field.controls.cos);
  try {
    (void)field.price();
    out.writeln(label, "-> no error");
  } catch (const std::exception &e) {
    const std::string message = e.what();
    out.writeln(label, "->", message.substr(0, message.find('\n')));
  }
  };
  attempt("num_terms = 1     ", [](auto &o) { o.num_terms = 1; });
  attempt("truncation = 0    ", [](auto &o) { o.truncation = 0.0; });
  attempt("cumulant_step = 0 ", [](auto &o) { o.cumulant_step = 0.0; });
  attempt("cumulant_step = 1e-8", [](auto &o) { o.cumulant_step = 1e-8; });
}
```

Output

```text
num_terms = 1      -> subsp::math::finance::module::Field: controls.cos.num_terms must be >= 2
truncation = 0     -> subsp::math::finance::module::Field: controls.cos.truncation must be > 0
cumulant_step = 0  -> subsp::math::finance::module::Field: controls.cos.cumulant_step must be >= 1e-6 (a smaller step loses the variance to rounding)
cumulant_step = 1e-8 -> subsp::math::finance::module::Field: controls.cos.cumulant_step must be >= 1e-6 (a smaller step loses the variance to rounding)
```

Each setting is rejected with a message that names it. A `cumulant_step` below 1e-6 is refused because the variance, a second difference of the characteristic function, would be lost to rounding.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="smiles_from_heston.html">
    <span class="label">Previous</span>
    <span class="title">Smiles from Heston</span>
  </a>
  <a class="next-link" href="the_fft_pricer.html">
    <span class="label">Next</span>
    <span class="title">The FFT pricer</span>
  </a>
</div>
<!-- apiref:nav:end -->
