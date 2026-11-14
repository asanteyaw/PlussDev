<!-- generated from building_a_field.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Building a Field

[Back to The Pricing Field](../finance_06_the_pricing_field.md)

A `Field` is one object that holds everything needed to price a European option: **what** is priced (`instrument`), **in which market** (`market`), **under which model** (`model`), **how** (`controls`), and the simulated `paths` if there are any. Every earlier chapter called a pricing function with a long argument list; a `Field` keeps those arguments together so that you change one thing at a time (the engine, the model, a single parameter, the strikes) and price again without rebuilding the rest.

The quantity every engine computes is the discounted risk-neutral expectation of the payoff,

```{math}
C = e^{-rT}\,\mathbb{E}^{\mathbb{Q}}\big[(S_T-K)^+\big],\qquad
P = e^{-rT}\,\mathbb{E}^{\mathbb{Q}}\big[(K-S_T)^+\big],
```

where {math}`S_T` is the asset at maturity {math}`T`, {math}`K` the strike, {math}`r` the continuously compounded rate and {math}`q` the dividend yield. The model decides the law of {math}`S_T`. In the Black-Scholes model the asset is a geometric Brownian motion,

```{math}
dS_t=(r-q)\,S_t\,dt+\sigma S_t\,dW_t
\quad\Longrightarrow\quad
S_t=S_0\exp\!\Big(\big(r-q-\tfrac12\sigma^2\big)t+\sigma W_t\Big),
```

with {math}`W_t` a Brownian motion, and the expectation has a closed form. In a stochastic-volatility model such as Heston the constant {math}`\sigma` is replaced by {math}`\sqrt{v_t}`, where the variance {math}`v_t` is itself random,

```{math}
dS_t=(r-q)S_t\,dt+\sqrt{v_t}\,S_t\,dW^S_t,\qquad
dv_t=\kappa(\theta-v_t)\,dt+\sigma\sqrt{v_t}\,dW^v_t,\qquad
d\langle W^S,W^v\rangle_t=\rho\,dt,
```

and the expectation is computed by a transform method (Fourier inversion or the COS expansion) or by simulating the paths. A `Field` does not care which: you choose the model and the engine, and `price()` dispatches to the right routine. The same five members work for every model, which is what makes the object reusable.

## The five members

| Member | Type | Holds |
|---|---|---|
| `instrument` | `instrument::Vanilla` | `type` (`Call`, `Put` or `Auto`), the mask `is_call` (read when the type is `Auto`), `strike`, `maturity` |
| `market` | `market::State` | `spot`, `rate`, `dividend_yield` |
| `model` | `module::Model` | a value holding one of six models: `Heston`, `Bates`, `BlackScholes`, `Sabr`, `RoughHeston`, `RoughBergomi` |
| `controls` | `module::Controls` | the `engine`, the `risk` request, the `loss`, and the settings of each engine (`fft`, `cos`, `mc`, `analytic`) |
| `paths` | `module::Paths` | the simulated `asset`, `vol`, `variance` and the shocks; empty until `simulate()` is called |

Every numeric member is a `Series`, so a whole strip of strikes, maturities or spots is one `Field` and one call to `price()`. The members are plain data: read and assign them at any time, and a `Field` copies like any other struct.

```cpp
module::Field::Result module::Field::price(Config cfg = {}) const;
module::Field::Result module::Field::price(const Model &candidate, Config cfg = {}) const;
template <SupportedModel M> module::Field::Result module::Field::price(const M &model, Config cfg = {}) const;
void module::Field::validate() const;
std::string module::Field::model_name() const;
std::string module::Field::engine_name() const;
```

| Call | Meaning |
|---|---|
| `price(cfg)` | prices the model the `Field` holds with the engine in `controls` |
| `price(candidate, cfg)`, `price(model, cfg)` | prices another model, given as a `Model` or as a bare model struct; the `Field` is not changed |
| `validate()` | checks the members and the engine against the model, and throws if something is missing or unsupported; `price` calls it first |
| `model_name()`, `engine_name()` | the names of the held model and of the chosen engine, as strings |

`instrument::make(type, strike, maturity, cfg)`, `market::make(spot, rate, dividend_yield, cfg)` and `model::make(kind, {parameters}, cfg)` build the members from plain numbers; they are the quickest way to write a one-option `Field`. They return one-element `Series` stored as `Float64`, so a number such as `0.04` is held exactly; pass a `Config` as the last argument only to choose another dtype or platform.

The model is always named by a `model::Kind`, never inferred from how many numbers follow. The numbers are the parameters in the fixed order of the table, and a wrong count is an error that lists the order.

| `model::Kind` | Parameters, in order |
|---|---|
| `BlackScholes` | `volatility` |
| `Sabr` | `alpha0`, `beta`, `nu`, `rho` |
| `Heston` | `kappa`, `theta`, `sigma`, `rho`, `v0` |
| `Bates` | `kappa`, `theta`, `sigma`, `rho`, `v0`, `jump_intensity`, `jump_mean`, `jump_vol` |
| `RoughHeston` | `kappa`, `theta`, `sigma`, `alpha`, `rho`, `v0` |
| `RoughBergomi` | `xi`, `eta`, `alpha`, `rho` |

The engine is chosen the same way, by `controls.engine`. If `controls` is left out the engine is `Analytic` and the model is `BlackScholes`: a `Field` with nothing chosen is the Black-Scholes closed form, and it asks for the volatility it lacks. Every choice that is not Black-Scholes is made explicitly, and everything is checked when the `Field` prices, simulates or enters an objective: a wrong parameter count, a parameter outside its domain, an engine that cannot price the model or the exercise style, or members whose sizes disagree raise an error that lists every problem found, not just the first. `validate()` runs the same checks without pricing.

## Program 1: price one option, switch the engine

The program builds the `Field` from the `make` helpers and prices a one-year at-the-money call. The `controls` choose the Fourier-cosine engine with `risk = All`, so the first call returns the price and the five Greeks. It then switches to the Fourier-FFT engine, and finally to Monte Carlo, with `risk = PriceOnly` so that no bumped scenarios are simulated (Greeks by engine are the subject of page 2). Nothing else changes between the three prices: the instrument, the market and the model are the same objects.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::All}};
  out.writeln("model", field.model_name(), " engine", field.engine_name());
  const auto result = field.price();
  out.writeln("price", result.price.items<double>(), " delta", result.greeks.delta.items<double>());
  field.controls.engine = mod::Engine::Fft;
  out.writeln("fft  ", field.price().price.items<double>());
  field.controls.engine = mod::Engine::MonteCarlo;
  field.controls.risk = mod::Risk::PriceOnly;
  field.controls.mc.paths = 40000;
  field.controls.mc.seed = 1;
  out.writeln("mc   ", field.price().price.items<double>(), " engine", field.engine_name());
}
```

Output

```text
model Heston  engine Cos
price [8.628108]  delta [0.644691]
fft   [8.628108]
mc    [8.645229]  engine MonteCarlo
```

COS and FFT are two numerical evaluations of the same Fourier integral, so they agree to every printed digit, 8.628108. The Monte Carlo estimate, 8.645229, is a random estimate from 40,000 paths. Its standard error is of order {math}`s/\sqrt{N}` with {math}`s` the standard deviation of the discounted payoff (about 14 here), so about 0.07, and the gap of 0.017 is well inside it. `engine_name()` and `model_name()` return the strings `"Cos"`, `"MonteCarlo"`, `"Heston"` and so on, which is what you want in a log line or a report heading.

## Program 2: a strip of strikes is one call

`strike` is a `Series`, so a whole smile is priced in one call. `maturity`, `spot`, `rate` and `dividend_yield` may each have one element, which is then reused for every strike, or the same number of elements as the strike, in which case they pair element by element (the next program and the error program show both). The program prices five strikes against scalar market data and prints, beside each price, the Black-Scholes implied volatility of that price, which is the smile the model generates.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, as_f64()), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.02}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos}};
  const auto result = field.price();
  const auto k = field.instrument.strike.items<double>(), c = result.price.items<double>();
  out.writeln("shape", result.price.sizes(), " dtype", result.price.dtype());
  const auto smile = field.implied_vol().items<double>();   // the Black-Scholes volatility each price implies
  out.writeln("strike  price    implied vol");
  for (std::size_t i = 0; i < k.size(); ++i) {
    out.writeln(k[i], c[i], smile[i]);
  }
}
```

Output

```text
shape [5]  dtype float64
strike  price    implied vol
80.0000 23.0606 0.2281
90.0000 15.1137 0.2099
100.0000 8.6281 0.1926
110.0000 4.0596 0.1768
120.0000 1.4918 0.1634
```

The result has shape `[5]` and is `Float64` whatever the dtype of the inputs: pricing is carried out in double precision. The implied volatility falls from 22.8% at strike 80 to 16.3% at strike 120. That downward slope is the signature of the negative correlation {math}`\rho=-0.7`: when the asset falls its variance rises, which fattens the left tail and raises the price of low strikes relative to Black-Scholes at one flat volatility. With {math}`\rho=0` the smile would be symmetric, and with {math}`\rho>0` it would slope upward.

## Program 3: calls and puts in one strip

`instrument.type` is `Call`, `Put` or `Auto`. With `Call` or `Put` every element is that type. With `Auto`, `instrument.is_call` is required and has the shape of the strike: an element 1 prices a call and an element 0 a put, so one `Field` can hold a mixed book. The program prices a call and a put at each of three strikes, in the order call, put, call, put, ...

Both are the same expectation of different payoffs, so they are tied by put-call parity, which holds in every arbitrage-free model, not only in Black-Scholes:

```{math}
C-P \;=\; e^{-rT}\,\mathbb{E}^{\mathbb{Q}}\big[S_T-K\big] \;=\; S_0e^{-qT}-Ke^{-rT}.
```

Parity is a check that costs nothing: if an engine or a parameter set breaks it, something upstream is wrong.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto strike = pluss::make_series({90.0, 90.0, 100.0, 100.0, 110.0, 110.0});
  const auto is_call = pluss::make_series({1.0, 0.0, 1.0, 0.0, 1.0, 0.0});
  mod::Field field{.instrument = {.type = fin::pricing::OptionType::Auto, .is_call = is_call, .strike = strike, .maturity = pluss::make_series({0.5})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.03}), .dividend_yield = pluss::make_series({0.01})},
                   .model = fin::model::make(fin::model::Kind::Heston, {2.0, 0.05, 0.4, -0.6, 0.05}),
                   .controls = {.engine = mod::Engine::Cos}};
  const auto p = field.price().price.items<double>();
  out.writeln("strike  call       put        call - put   S e^{-qT} - K e^{-rT}");
  for (int i = 0; i < 6; i += 2) {
    const double k = strike.items<double>()[i];
    out.writeln(k, p[i], p[i + 1], p[i] - p[i + 1], 100.0 * std::exp(-0.01 * 0.5) - k * std::exp(-0.03 * 0.5));
  }
  field.instrument.type = fin::pricing::Put;
  out.writeln("all puts", field.price().price.items<double>());
}
```

Output

```text
strike  call       put        call - put   S e^{-qT} - K e^{-rT}
90.000000 13.128860 2.287687 10.841173 10.841173
100.000000 6.524857 5.534803 0.990054 0.990054
110.000000 2.401510 11.262576 -8.861065 -8.861065
all puts [2.287687, 2.287687, 5.534803, 5.534803, 11.262576, 11.262576]
```

The two right-hand columns are equal to all six printed decimals, for the three strikes: the Heston prices obey parity exactly, as they must, since {math}`\mathbb{E}^{\mathbb{Q}}[S_T]=S_0e^{(r-q)T}` is a property of the drift and does not depend on the volatility model. The last line sets `type = Put` and prices the same six strikes as puts; the `is_call` mask is then ignored, so every element is a put and the call elements (positions 0, 2, 4) now equal the put elements beside them.

An `Auto` field without `is_call` is an error, as is a mask whose length cannot be paired with the strike or an element outside [0, 1]; Program 5 shows the messages.

## Program 4: price another model without changing the Field

`price(candidate)` prices a model that is not the one the `Field` holds, and leaves the `Field` untouched. That is exactly what an optimizer needs: it tries many candidate parameter sets against one fixed instrument, market and set of controls. The candidate can be a `module::Model`, which holds any of the six models, or a model struct passed directly through the template overload. The program shifts the initial variance {math}`v_0` from 0.04 to 0.01 on a copy of the model, prices the copy, then prices a Bates model and a Black-Scholes model against the same instrument and market, and finally assigns a model to the `Field`, which does change it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos}};
  const auto base = field.price().price.items<double>()[0];

  fin::model::Heston calm = field.model.as<fin::model::Heston>();
  calm.v0 = pluss::make_series({0.01});
  out.writeln("price(calm)", field.price(calm).price.items<double>(), " price()", field.price().price.items<double>(), " base", base);
  out.writeln("field v0 still", field.model.as<fin::model::Heston>().v0.items<double>());

  mod::Model bates = fin::model::make(fin::model::Kind::Bates, {1.5, 0.04, 0.3, -0.7, 0.04, 0.1, -0.1, 0.15});
  out.writeln("bates", field.price(bates).price.items<double>());
  mod::Model flat = fin::model::BlackScholes{.volatility = pluss::make_series({0.2})};
  field.controls.engine = mod::Engine::Analytic;
  out.writeln("bs   ", field.price(flat).price.items<double>());
  field.controls.engine = mod::Engine::Cos;
  field.model = bates;
  out.writeln("field now", field.model_name(), field.price().price.items<double>());
}
```

Output

```text
price(calm) [6.975302]  price() [8.628108]  base 8.628108
field v0 still [0.040000]
bates [8.915067]
bs    [8.916037]
field now Bates [8.915067]
```

With {math}`v_0=0.01` the call is worth 6.975302 against 8.628108, and afterwards the `Field` still holds {math}`v_0=0.04` and still prices 8.628108: pricing a candidate never writes to the `Field`. The Bates model adds a compound-Poisson jump component to the asset (intensity 0.1 a year, mean log-jump −0.1, jump volatility 0.15), which raises the price to 8.915067. The Black-Scholes call at a flat 20% volatility is 8.916037, and the two are close only because the parameters were chosen so: at the money, the total variance over the year is about 0.04 in each. The last two lines show that assignment replaces the model and `model_name()` follows.

Note the engine switch before the Black-Scholes price: the `Field` held `Cos`, which does not price Black-Scholes, so the program sets `Analytic` first. The engines each model supports are tabulated on page 2.

## Program 5: what goes wrong, and what the message says

`price()` first calls `validate()`, which checks the members before any number is computed, and the engines then check what they need. The errors below all come from a correct `Field` with one thing broken. Each message names the member at fault, so a mistake is found from the message alone. The program prints the first line of each message.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const auto good = [] {
    return mod::Field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                      .market = fin::market::make(100.0, 0.02, 0.0),
                      .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04})};
  };
  const auto attempt = [&](const char *what, auto &&make_bad) {
    auto field = good();
    make_bad(field);
    try {
      field.price();
      out.writeln(std::string(what) + ": priced");
    } catch (const std::exception &e) {
      std::string message = e.what();
      out.writeln(std::string(what) + ": " + message.substr(0, message.find('\n')));
    }
  };
  attempt("no strike", [](mod::Field &f) { f.instrument.strike = pluss::series::undefined(); });
  attempt("no spot", [](mod::Field &f) { f.market.spot = pluss::series::undefined(); });
  attempt("analytic with Heston", [](mod::Field &f) { f.controls.engine = mod::Engine::Analytic; });
  attempt("cos with rough Heston", [](mod::Field &f) { f.model = fin::model::RoughHeston{}; });
  attempt("bad huber delta", [](mod::Field &f) { f.controls.loss = mod::Loss::Huber; f.controls.huber_delta = -1.0; });
  attempt("3 strikes, 4 spots", [](mod::Field &f) { f.instrument.strike = pluss::make_series({90.0, 100.0, 110.0}); f.market.spot = pluss::make_series({99.0, 100.0, 101.0, 102.0}); });
  attempt("negative maturity", [](mod::Field &f) { f.instrument.maturity = pluss::make_series({-1.0}); });
  attempt("auto without is_call", [](mod::Field &f) { f.instrument.type = fin::pricing::OptionType::Auto; });
  attempt("is_call outside [0, 1]", [](mod::Field &f) { f.instrument.type = fin::pricing::OptionType::Auto; f.instrument.is_call = pluss::make_series({2.0}); });
  attempt("model missing a parameter", [](mod::Field &f) { f.model = fin::model::Heston{}; });
}
```

Output

```text
no strike: subsp::math::finance::module::Field (2 problems):
no spot: subsp::math::finance::module::Field (2 problems):
analytic with Heston: subsp::math::finance::module::Field: controls.engine Analytic is not supported for model Heston. Supported engines: Fft, Cos, MonteCarlo, Pde
cos with rough Heston: subsp::math::finance::module::Field (7 problems):
bad huber delta: subsp::math::finance::module::Field (2 problems):
3 strikes, 4 spots: subsp::math::finance::module::Field (2 problems):
negative maturity: subsp::math::finance::module::Field (2 problems):
auto without is_call: subsp::math::finance::module::Field (2 problems):
is_call outside [0, 1]: subsp::math::finance::module::Field (2 problems):
model missing a parameter: subsp::math::finance::module::Field (6 problems):
```

Reading them in order:

- **Undefined members.** A member that is a default-constructed `Series` is undefined; `strike`, `maturity`, `spot`, `rate` and `dividend_yield` must all be set. `validate()` reports the first one missing.
- **Engine and model.** The message lists the engines the model supports, here `Fft, Cos, MonteCarlo` for Heston and only `MonteCarlo` for the rough models. Choosing the engine is always the caller's decision; a `Field` never switches silently.
- **Controls.** `huber_delta` is only checked when `loss` is `Huber`, since it is unused otherwise.
- **Shapes.** Three strikes against four spots cannot be paired element by element, and the broadcast fails. One element would have broadcast against any length. The rule is the usual one: each of the five arrays has one element or the common length.
- **Maturity.** The engines require a finite maturity of at least zero; a negative one is reported by the engine that detects it, here the Heston COS routine.
- **Mixed books.** `Auto` needs `is_call`, with values in [0, 1].
- **Missing parameters.** A `Heston{}` with no parameters set is a model whose `kappa` is undefined; the message names the parameter.

`validate()` can be called on its own, before an expensive calibration loop, to find a mistake before the first of ten thousand evaluations rather than during it.

## Checklist

1. Build the `Field` once and change only the member you are studying; use `price(candidate)` when trying other models or parameters.
2. Give `strike` its full length and let `maturity`, `spot`, `rate` and `dividend_yield` be one element or the same length.
3. Use `OptionType::Auto` with an `is_call` mask of 1s and 0s for mixed books; otherwise set `Call` or `Put`.
4. The `make` helpers store `Float64`; build `Series` you create yourself (`linspace`, `full`) with `as_f64()` for the same reason.
5. Check put-call parity on any new engine or parameter set; it must hold to rounding.
6. Call `validate()` before a long loop.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="engines_and_greeks.html">
    <span class="label">Next</span>
    <span class="title">Engines and Greeks</span>
  </a>
</div>
<!-- apiref:nav:end -->
