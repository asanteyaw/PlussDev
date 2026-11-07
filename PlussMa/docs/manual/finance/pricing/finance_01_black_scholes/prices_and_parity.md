<!-- generated from prices_and_parity.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Prices and put-call parity

[Back to Black-Scholes](../finance_01_black_scholes.md)

The Black-Scholes model prices a European option on an asset whose price moves as a geometric Brownian motion with constant volatility σ. With spot *S*, strike *K*, continuously compounded rate *r*, continuous dividend yield *q* and time to maturity *T*, the price of a call is

C = S e^(−qT) N(d₁) − K e^(−rT) N(d₂), with d₁ = [ln(S/K) + (r − q + σ²/2) T] / (σ √T) and d₂ = d₁ − σ √T,

and the put is P = K e^(−rT) N(−d₂) − S e^(−qT) N(−d₁), where N is the standard normal distribution function.

```cpp
double prc::black_scholes_price(prc::OptionType type, double spot, double strike, double rate,
                                double dividend_yield, double maturity, double volatility);
```

`type` is `prc::Call` or `prc::Put`. Rates and yields are annual and continuously compounded, `maturity` is in years, and `volatility` is annual. The rate and the dividend yield may be negative; the spot, strike and volatility must be positive and every input finite.

## The formula, checked

The program prices a call and a put and recomputes both from the formula, with the standard normal distribution function taken from the distribution classes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double spot = 100.0, strike = 105.0, rate = 0.05, dividend = 0.02, maturity = 0.75, vol = 0.25;

  const double d1 = (std::log(spot / strike) + (rate - dividend + 0.5 * vol * vol) * maturity) / (vol * std::sqrt(maturity));
  const double d2 = d1 - vol * std::sqrt(maturity);
  const distr::Normal standard(0.0, 1.0);
  auto N = [&](double x) { return standard.cdf(pluss::make_series(std::vector<double>{x}, {1}, as_f64())).pull<double>()[0]; };
  const double call = spot * std::exp(-dividend * maturity) * N(d1) - strike * std::exp(-rate * maturity) * N(d2);
  const double put = strike * std::exp(-rate * maturity) * N(-d2) - spot * std::exp(-dividend * maturity) * N(-d1);

  out.writeln("d1", d1, " d2", d2);
  out.writeln("call", prc::black_scholes_price(prc::Call, spot, strike, rate, dividend, maturity, vol), " by hand", call);
  out.writeln("put ", prc::black_scholes_price(prc::Put, spot, strike, rate, dividend, maturity, vol), " by hand", put);
}
```

Output

```text
d1 -0.013176  d2 -0.229682
call 7.356284  by hand 7.356284
put  9.980504  by hand 9.980504
```

The library's call and put agree with the formula to the digits shown. Here d₁ and d₂ are slightly negative because the strike is above the forward, so the call is worth less than the put (7.356 against 9.981).

## Put-call parity

A call bought and a put sold at the same strike and maturity replicate a forward contract, whatever the volatility: C − P = S e^(−qT) − K e^(−rT). The relation is model free, so it also tests the pricer. The program checks it across strikes and shows that the difference does not depend on σ.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const double spot = 100.0, rate = 0.05, dividend = 0.02, maturity = 0.75;

  out.writeln("strike   vol    call - put     S e^-qT - K e^-rT     difference");
  for (double strike : {70.0, 90.0, 100.0, 110.0, 140.0}) {
    for (double vol : {0.10, 0.40}) {
      const double c = prc::black_scholes_price(prc::Call, spot, strike, rate, dividend, maturity, vol);
      const double p = prc::black_scholes_price(prc::Put, spot, strike, rate, dividend, maturity, vol);
      const double forward = spot * std::exp(-dividend * maturity) - strike * std::exp(-rate * maturity);
      out.writeln(strike, vol, c - p, forward, c - p - forward);
    }
  }
}
```

Output

```text
strike   vol    call - put     S e^-qT - K e^-rT     difference
70.00000000 0.10000000 31.08758472 31.08758472 0.00000000
70.00000000 0.40000000 31.08758472 31.08758472 0.00000000
90.00000000 0.10000000 11.82369637 11.82369637 0.00000000
90.00000000 0.40000000 11.82369637 11.82369637 0.00000000
100.00000000 0.10000000 2.19175219 2.19175219 0.00000000
100.00000000 0.40000000 2.19175219 2.19175219 0.00000000
110.00000000 0.10000000 -7.44019199 -7.44019199 0.00000000
110.00000000 0.40000000 -7.44019199 -7.44019199 0.00000000
140.00000000 0.10000000 -36.33602452 -36.33602452 0.00000000
140.00000000 0.40000000 -36.33602452 -36.33602452 0.00000000
```

The difference between the call and the put equals the discounted forward at every strike (the last column is zero to eight decimals), and it is the same at a volatility of 10% as at 40%: parity does not involve σ. It is positive when the strike is below the forward and negative above it.

## Bounds and limits

The price must lie between the discounted forward intrinsic value and the discounted spot: max(S e^(−qT) − K e^(−rT), 0) ≤ C ≤ S e^(−qT). It rises with σ, from the first bound as σ → 0 (the option is then a forward) to the second as σ → ∞. The program shows the price as σ moves from 10⁻⁴ to 100, for an in-the-money, an at-the-money and an out-of-the-money strike, and also for a maturity that shrinks to zero, where the price tends to the undiscounted intrinsic value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double spot = 100.0, rate = 0.05, dividend = 0.02, maturity = 1.0;
  const double discounted_spot = spot * std::exp(-dividend * maturity);

  out.writeln("discounted spot", discounted_spot);
  for (double strike : {80.0, 100.0, 120.0}) {
    const double lower = std::max(discounted_spot - strike * std::exp(-rate * maturity), 0.0);
    out.writeln("strike", strike, " lower bound", lower);
    for (double vol : {1e-4, 0.05, 0.2, 1.0, 10.0, 100.0}) {
      out.writeln("   vol", vol, " call", prc::black_scholes_price(prc::Call, spot, strike, rate, dividend, maturity, vol));
    }
  }

  out.writeln("maturity shrinking to zero, strike 95 (intrinsic value 5):");
  for (double t : {1.0, 0.25, 0.05, 0.01, 0.0001, 0.0}) {
    out.writeln("   maturity", t, " call", prc::black_scholes_price(prc::Call, spot, 95.0, 0.0, 0.0, t, 0.2));
  }
}
```

Output

```text
discounted spot 98.019867
strike 80.000000  lower bound 21.921513
   vol 0.000100  call 21.921513
   vol 0.050000  call 21.921514
   vol 0.200000  call 22.764125
   vol 1.000000  call 45.268242
   vol 10.000000  call 98.019818
   vol 100.000000  call 98.019867
strike 100.000000  lower bound 2.896925
   vol 0.000100  call 2.896925
   vol 0.050000  call 3.711144
   vol 0.200000  call 9.227006
   vol 1.000000  call 38.443357
   vol 10.000000  call 98.019812
   vol 100.000000  call 98.019867
strike 120.000000  lower bound 0.000000
   vol 0.000100  call 0.000000
   vol 0.050000  call 0.001713
   vol 0.200000  call 2.711776
   vol 1.000000  call 32.989658
   vol 10.000000  call 98.019807
   vol 100.000000  call 98.019867
maturity shrinking to zero, strike 95 (intrinsic value 5):
   maturity 1.000000  call 10.519541
   maturity 0.250000  call 6.888063
   maturity 0.050000  call 5.272323
   maturity 0.010000  call 5.003192
   maturity 0.000100  call 5.000000
   maturity 0.000000  call 5.000000
```

At the in-the-money strike (80), the price at a volatility of 10⁻⁴ is the lower bound 21.9215, the discounted forward intrinsic value, and it rises to 98.0199 at a volatility of 100, the discounted spot. The at-the-money and out-of-the-money strikes rise between the same bounds, from 2.8969 and 0 to 98.0199. The bounds are never crossed. As the maturity shrinks the call at strike 95 falls from 10.52 toward the intrinsic value of 5: it is 5.0032 at 0.01 years and 5.0000 at 0.0001, and exactly 5 at zero.
