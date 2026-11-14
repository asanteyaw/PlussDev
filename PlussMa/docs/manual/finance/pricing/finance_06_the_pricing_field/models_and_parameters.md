<!-- generated from models_and_parameters.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Models and parameters

[Back to The Pricing Field](../finance_06_the_pricing_field.md)

A `module::Model` is a value that holds exactly one of six models. The six are plain structs of `Series`, one `Series` per parameter, and the `Model` is a thin wrapper around a `std::variant` of them. This page covers what each model is, the order of its parameters in the flat vector that optimizers use, how to read and write parameters, how to ask which model is held, and which fields are optional.

## The six models

All of them describe the asset under the pricing measure, with drift {math}`r-q`:

```{math}
\begin{aligned}
\textbf{BlackScholes:}\quad & dS_t=(r-q)S_t\,dt+\sigma S_t\,dW_t,\\[2pt]
\textbf{Heston:}\quad & dS_t=(r-q)S_t\,dt+\sqrt{v_t}\,S_t\,dW^S_t,\quad dv_t=\kappa(\theta-v_t)\,dt+\sigma\sqrt{v_t}\,dW^v_t,\quad d\langle W^S,W^v\rangle=\rho\,dt,\\[2pt]
\textbf{Bates:}\quad & \text{Heston, plus jumps: } \frac{dS_t}{S_{t-}}=(r-q-\lambda k)\,dt+\sqrt{v_t}\,dW^S_t+(e^{J}-1)\,dN_t,\\[2pt]
\textbf{SABR:}\quad & dF_t=\alpha_tF_t^{\beta}\,dW^F_t,\quad d\alpha_t=\nu\alpha_t\,dW^\alpha_t,\quad d\langle W^F,W^\alpha\rangle=\rho\,dt,\\[2pt]
\textbf{RoughHeston:}\quad & v_t=v_0+\int_0^t K(t-s)\,\big[\kappa(\theta-v_s)\,ds+\sigma\sqrt{v_s}\,dW_s\big],\quad K(u)=\tfrac{u^{\alpha}}{\Gamma(\alpha+1)},\\[2pt]
\textbf{RoughBergomi:}\quad & v_t=\xi\exp\!\Big(\eta\,Y_t-\tfrac12\eta^2t^{2H}\Big),\quad Y_t=\sqrt{2H}\int_0^t(t-s)^{H-1/2}\,dW_s,\quad H=\alpha+\tfrac12.
\end{aligned}
```

In the Bates model {math}`N` is a Poisson process of intensity {math}`\lambda` (`jump_intensity`), the log-jump {math}`J\sim\mathcal N(\mu_J,\sigma_J^2)` has mean `jump_mean` and standard deviation `jump_vol`, and {math}`k=e^{\mu_J+\sigma_J^2/2}-1` is the mean relative jump size, subtracted from the drift so that {math}`e^{-(r-q)t}S_t` stays a martingale. SABR models a forward {math}`F`; its parameter `beta` is the backbone exponent and `alpha0` is {math}`\alpha_0`. In the two rough models `alpha` is {math}`H-\tfrac12`, so `alpha = -0.4` is a Hurst exponent {math}`H=0.1`, rougher than Brownian motion ({math}`H=\tfrac12`), and `alpha = 0` is the smooth limit; for Heston itself the Feller condition {math}`2\kappa\theta\ge\sigma^2` keeps {math}`v_t` strictly positive.

## The parameter vector

`model.parameters()` returns a flat `Series` of the model's main parameters, in a fixed order, and `model.parameters(values)` writes them back. This is the interface for an optimizer, which sees a vector, not six differently named structs.

| Model | Parameters, in order | Count |
|---|---|---|
| `BlackScholes` | `volatility` | 1 |
| `Sabr` | `alpha0`, `beta`, `nu`, `rho` | 4 |
| `Heston` | `kappa`, `theta`, `sigma`, `rho`, `v0` | 5 |
| `Bates` | `kappa`, `theta`, `sigma`, `rho`, `v0`, `jump_intensity`, `jump_mean`, `jump_vol` | 8 |
| `RoughHeston` | `kappa`, `theta`, `sigma`, `alpha`, `rho`, `v0` | 6 |
| `RoughBergomi` | `xi`, `eta`, `alpha`, `rho` | 4 |

The other fields (`mu`, `variance_floor`, `alpha_floor`, the SABR `vol_type` and `approx`) are settings, not fitted parameters, and are not in the vector.

```cpp
Series module::Model::parameters(Config cfg = {}) const;
void module::Model::parameters(const Series &params, Config cfg = {});
void module::Model::parameters(std::span<const double> params, Config cfg = {});
void module::Model::parameters(const std::vector<double> &params, Config cfg = {});
template <std::size_t N> void module::Model::parameters(const std::array<double, N> &params, Config cfg = {});
std::string module::Model::name() const;
template <SupportedModel M> bool module::Model::holds() const;
template <SupportedModel M> M &module::Model::as();
template <typename F> decltype(auto) module::Model::visit(F &&f);
```

| Call | Meaning |
|---|---|
| `Model(model)` | a `Model` is built from, and assigned from, any of the six model structs |
| `parameters(cfg)` | the flat parameter vector, in the order of the table above |
| `parameters(values)` | writes the vector back, from a `Series`, a `std::vector`, a `std::array` or a `std::span` |
| `name()`, `holds<M>()` | the name of the held model, and whether it is the model `M` |
| `as<M>()` | a reference to the held model, to read or change one parameter; an error if the model is not `M` |
| `visit(f)` | calls `f` with whichever model is held |

## Program 1: the six models, their vectors and a price each

The program builds one of each, with designated initialisers, so that each argument is named. It prices a one-year at-the-money call with the engine natural for the model and prints the parameter vector. The two rough models use 4,000 paths and 32 steps, so their prices carry simulation error of order 0.2.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto v = [](double x) { return pluss::make_series({x}); };
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0), .market = fin::market::make(100.0, 0.02, 0.0)};
  field.controls.risk = mod::Risk::PriceOnly;
  field.controls.mc.paths = 4000;
  field.controls.mc.steps = 32;
  field.controls.mc.seed = 1;

  const std::vector<std::pair<mod::Model, mod::Engine>> models{
      {fin::model::BlackScholes{.volatility = v(0.2)}, mod::Engine::Analytic},
      {fin::model::Sabr{.alpha0 = v(0.2), .beta = v(1.0), .nu = v(0.4), .rho = v(-0.4)}, mod::Engine::Analytic},
      {fin::model::Heston{.kappa = v(1.5), .theta = v(0.04), .sigma = v(0.3), .rho = v(-0.7), .v0 = v(0.04)}, mod::Engine::Cos},
      {fin::model::Bates{.kappa = v(1.5), .theta = v(0.04), .sigma = v(0.3), .rho = v(-0.7), .v0 = v(0.04), .jump_intensity = v(0.1), .jump_mean = v(-0.1), .jump_vol = v(0.15)}, mod::Engine::Cos},
      {fin::model::RoughHeston{.kappa = v(1.5), .theta = v(0.04), .sigma = v(0.3), .alpha = v(-0.2), .rho = v(-0.7), .v0 = v(0.04)}, mod::Engine::MonteCarlo},
      {fin::model::RoughBergomi{.xi = v(0.04), .eta = v(1.5), .alpha = v(-0.4), .rho = v(-0.7)}, mod::Engine::MonteCarlo},
  };
  for (const auto &[model, engine] : models) {
    field.model = model;
    field.controls.engine = engine;
    out.writeln(model.name(), field.engine_name(), "price", field.price().price.items<double>()[0]);
    out.writeln("   parameters", model.parameters().items<double>());
  }
}
```

Output

```text
BlackScholes Analytic price 8.9160
   parameters [0.2000]
Sabr Analytic price 8.9970
   parameters [0.2000, 1.0000, 0.4000, -0.4000]
Heston Cos price 8.6281
   parameters [1.5000, 0.0400, 0.3000, -0.7000, 0.0400]
Bates Cos price 8.9151
   parameters [1.5000, 0.0400, 0.3000, -0.7000, 0.0400, 0.1000, -0.1000, 0.1500]
RoughHeston MonteCarlo price 8.4725
   parameters [1.5000, 0.0400, 0.3000, -0.2000, -0.7000, 0.0400]
RoughBergomi MonteCarlo price 8.1512
   parameters [0.0400, 1.5000, -0.4000, -0.7000]
```

Each vector is in the order of the table. The prices differ because the models do: Black-Scholes at 20% gives 8.9160; Heston, whose variance averages 0.04 but is random and negatively correlated with the asset, gives 8.6281; Bates adds negative jumps and gives 8.9151; and SABR with {math}`\beta=1` and {math}`\nu=0.4` prices at a slightly higher volatility than its {math}`\alpha_0` because the convexity from the random volatility lifts at-the-money options. The rough prices (8.47 and 8.15) are Monte Carlo estimates.

## Program 2: reading and writing parameters

There are five ways to write a parameter vector, and all of them land in the same fields. The program sets a Heston model from a `std::vector`, a `std::array`, a `std::span`, a `Series` and a `Float32` `Series`, printing the vector and dtype after each, and then makes three mistakes.

The dtype deserves attention. The numeric overloads write `Float64` parameters. The `Series` overload keeps the dtype and platform of the `Series` you pass, because the parameters become that `Series`: a `Float32` vector gives `Float32` parameters. Pricing converts to `Float64` internally either way, but `Float32` storage rounds the parameters (0.04 becomes 0.039999999), which matters for the last digits of a calibration.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Model model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04});
  out.writeln("get      ", model.parameters().items<double>(), model.parameters().dtype());

  model.parameters(std::vector<double>{2.0, 0.05, 0.4, -0.6, 0.05});
  out.writeln("vector   ", model.parameters().items<double>());
  model.parameters(std::array<double, 5>{1.0, 0.03, 0.2, -0.5, 0.03});
  out.writeln("array    ", model.parameters().items<double>());
  const std::vector<double> storage{3.0, 0.06, 0.5, -0.8, 0.06};
  model.parameters(std::span<const double>(storage));
  out.writeln("span     ", model.parameters().items<double>());
  model.parameters(pluss::make_series({1.5, 0.04, 0.3, -0.7, 0.04}));
  out.writeln("series   ", model.parameters().items<double>(), model.parameters().dtype());
  model.parameters(pluss::make_series({1.5, 0.04, 0.3, -0.7, 0.04}).to(pluss::ScalarType::Float32));
  out.writeln("float32  ", model.parameters().items<double>(), model.parameters().dtype(), " kappa dtype", model.as<fin::model::Heston>().kappa.dtype());

  const auto attempt = [&](const char *what, auto &&call) {
    try {
      call();
      out.writeln(std::string(what) + ": accepted");
    } catch (const std::exception &e) {
      std::string message = e.what();
      out.writeln(std::string(what) + ": " + message.substr(0, message.find('\n')));
    }
  };
  attempt("4 values for Heston", [&] { model.parameters(std::vector<double>{1.0, 2.0, 3.0, 4.0}); });
  attempt("matrix of values", [&] { model.parameters(pluss::series::zeros({5, 1}, as_f64())); });
  attempt("2x5 matrix", [&] { model.parameters(pluss::series::zeros({2, 5}, as_f64())); });
}
```

Output

```text
get       [1.5000, 0.0400, 0.3000, -0.7000, 0.0400] float64
vector    [2.0000, 0.0500, 0.4000, -0.6000, 0.0500]
array     [1.0000, 0.0300, 0.2000, -0.5000, 0.0300]
span      [3.0000, 0.0600, 0.5000, -0.8000, 0.0600]
series    [1.5000, 0.0400, 0.3000, -0.7000, 0.0400] float64
float32   [1.5000, 0.0400, 0.3000, -0.7000, 0.0400] float32  kappa dtype float32
4 values for Heston: subsp::math::finance::module::Model::parameters: Heston expects 5 parameters (kappa, theta, sigma, rho, v0)
matrix of values: accepted
2x5 matrix: subsp::math::finance::module::model.parameters: params must be vector-like
```

Each setter produced the expected vector. The `Float32` line shows both the vector and the field `kappa` in `Float32`. For the three failures:

- A vector of the wrong length is an error naming the model and the count it expects (5 for Heston).
- A `[5, 1]` column is accepted since it is vector-like; a matrix such as `[2, 5]` is rejected.
- Nothing is changed on a failed call: the check precedes the write.

## Program 3: which model is held

`holds<M>()` tests for a model, `name()` returns its name, `as<M>()` returns a reference to it (to read or modify one parameter), and `visit` calls a function with whichever model is held, so that a single piece of generic code works for all six. Assigning a model struct or a `Model` to a `Model` replaces what it holds. The program changes one field of a Black-Scholes model through `as`, visits a model with a function that behaves differently for the Heston family, and then calls `as<Sabr>()` on a Heston model, which is an error naming both models. At the end it shows the template overload `Field::price(m)`, which prices a bare model struct and gives the same result as pricing a `Model`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto v = [](double x) { return pluss::make_series({x}); };
  mod::Model model = fin::model::BlackScholes{.volatility = v(0.2)};
  out.writeln(model.name(), "holds BlackScholes", model.holds<fin::model::BlackScholes>(), " holds Heston", model.holds<fin::model::Heston>());
  model.as<fin::model::BlackScholes>().volatility = v(0.3);
  out.writeln("volatility now", model.parameters().items<double>());

  const auto describe = [](const mod::Model &m) {
    return m.visit([](const auto &held) -> std::string {
      using M = std::decay_t<decltype(held)>;
      if constexpr (std::is_same_v<M, fin::model::Heston> || std::is_same_v<M, fin::model::Bates>) return "has a variance process: v0 = " + std::to_string(held.v0.template items<double>()[0]);
      else return "no initial variance";
    });
  };
  out.writeln("describe BS    ", describe(model));
  model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04});
  out.writeln("describe Heston", describe(model), " parameters", model.parameters().numel());
  try {
    model.as<fin::model::Sabr>();
  } catch (const std::exception &e) {
    std::string message = e.what();
    out.writeln(message.substr(0, message.find('\n')));
  }

  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0), .market = fin::market::make(100.0, 0.02, 0.0), .model = model,
                   .controls = {.engine = mod::Engine::Cos}};
  out.writeln("template overload", field.price(model.as<fin::model::Heston>()).price.items<double>(), " Model overload", field.price(model).price.items<double>());
}
```

Output

```text
BlackScholes holds BlackScholes true  holds Heston false
volatility now [0.3000]
describe BS     no initial variance
describe Heston has a variance process: v0 = 0.040000  parameters 5
subsp::math::finance::module::Model::as: the model holds Heston, not Sabr
template overload [8.6281]  Model overload [8.6281]
```

`as<M>()` returns a reference, so `model.as<BlackScholes>().volatility = ...` edits the model in place. The wrong-model error says "the model holds Heston, not Sabr", so a mismatch is diagnosed at once. `visit` takes a generic lambda; inside it use `if constexpr` on the type for the parts that differ between models, as here, where only Heston and Bates have `v0`.

## Program 4: SABR and its approximations

SABR has a closed-form approximation for the Black implied volatility. With {math}`z=\dfrac{\nu}{\alpha}(FK)^{\frac{1-\beta}{2}}\ln\dfrac FK` and {math}`x(z)=\ln\dfrac{\sqrt{1-2\rho z+z^2}+z-\rho}{1-\rho}`, the Hagan expansion is

```{math}
\sigma_B(K,F)\approx
\frac{\alpha}{(FK)^{\frac{1-\beta}{2}}\Big[1+\frac{(1-\beta)^2}{24}\ln^2\frac FK+\frac{(1-\beta)^4}{1920}\ln^4\frac FK\Big]}
\cdot\frac{z}{x(z)}\cdot
\Big[1+\Big(\frac{(1-\beta)^2\alpha^2}{24(FK)^{1-\beta}}+\frac{\rho\beta\nu\alpha}{4(FK)^{\frac{1-\beta}{2}}}+\frac{2-3\rho^2}{24}\nu^2\Big)T\Big].
```

The `Analytic` engine takes the forward {math}`F=S\,e^{(r-q)T}` from the market, evaluates the expansion at each strike, and prices with the Black formula (lognormal quote) or the Bachelier formula (normal quote). The `SabrVolType` field of the model chooses the quote, `Lognormal` or `Normal`, and `approx.model` chooses between `Hagan2002` and `Obloj2008`; `approx.shift` displaces the forward for negative rates. Obloj's version replaces the first factor with a form that is more accurate for {math}`\beta<1` far from the forward, and agrees with Hagan's at {math}`\beta=1`; the example therefore uses {math}`\beta=0.5`, with {math}`\alpha_0=2` so that the at-the-money volatility is about 20%.

The program prices five strikes under the four combinations, then reads the implied volatility of the price the `Field` returns at strike 90 with `implied_vol()`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({80.0, 90.0, 100.0, 110.0, 120.0}), .maturity = pluss::make_series({1.0})},
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Sabr, {2.0, 0.5, 0.5, -0.4}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  const double forward = 100.0 * std::exp(0.02);
  const auto k = field.instrument.strike.items<double>();
  const auto show = [&](const std::string &label) {
    const auto c = field.price().price.items<double>();
    std::string row = label;
    for (std::size_t i = 0; i < k.size(); ++i) {
      row += " " + std::to_string(c[i]).substr(0, 7);
    }
    out.writeln(row);
  };
  out.writeln("strikes                  80      90      100     110     120");
  show("lognormal, Hagan 2002   ");
  auto sabr = field.model.as<fin::model::Sabr>();
  sabr.approx.model = fin::pricing::SabrApproxModel::Obloj2008;
  field.model = sabr;
  show("lognormal, Obloj 2008   ");
  sabr.vol_type = fin::pricing::SabrVolType::Normal;
  field.model = sabr;
  show("normal, Obloj 2008      ");
  sabr.approx.model = fin::pricing::SabrApproxModel::Hagan2002;
  field.model = sabr;
  show("normal, Hagan 2002      ");
  sabr.vol_type = fin::pricing::SabrVolType::Lognormal;
  sabr.approx.model = fin::pricing::SabrApproxModel::Hagan2002;
  field.model = sabr;
  out.writeln("forward used by the Field", forward);
  out.writeln("implied vol of the Field price at 90", field.implied_vol().items<double>()[1]);
}
```

Output

```text
strikes                  80      90      100     110     120
lognormal, Hagan 2002    23.3799 15.4712 9.04624 4.55844 1.98346
lognormal, Obloj 2008    23.3804 15.4713 9.04624 4.55842 1.98342
normal, Obloj 2008       23.3839 15.4736 9.04695 4.55804 1.98284
normal, Hagan 2002       23.3839 15.4736 9.04695 4.55804 1.98284
forward used by the Field 102.0201
implied vol of the Field price at 90 0.2213
```

The two expansions differ in the fourth decimal in the wings (23.3799 and 23.3804 at strike 80; 4.55844 and 4.55842 at strike 110) and agree at the money: the correction terms are the same to first order. The normal quote prices differ from the lognormal one in the third or fourth decimal, because the two are different approximations to the same price: both expansions are only accurate to the order of the terms kept. The Hagan and Obloj normal-volatility prices coincide here. The forward the `Field` used is {math}`100\,e^{0.02}=102.0201`, and the implied volatility of its price at strike 90 is 0.2213, the SABR Black volatility at that forward: the `Analytic` engine is the expansion priced through Black and nothing more. For long maturities or a large {math}`\nu` the expansion is less accurate; simulate (`MonteCarlo`) and compare.

## Program 5: optional fields and defaults

Several fields of the model structs are optional and may be left undefined: `mu` and `variance_floor` in the Heston family, `alpha_floor` in SABR, and the three jump fields of the Bates model. The rules, shown by the program, are:

- **`mu` is not used by the `Field`.** The drift of the asset is always {math}`r-q` from the market, so a `mu` set on the model has no effect on a price or a simulation made through the `Field`. It exists for the standalone path generators, where there is no market.
- **`variance_floor` and `alpha_floor`** are floors applied to the variance (or to {math}`\alpha`) in a discretised simulation, to keep the Euler step from producing a negative value. They affect simulation only, and default to {math}`10^{-12}`.
- **The Bates jump fields default** to `jump_intensity = 0.15`, `jump_mean = -0.08` and `jump_vol = 0.20` if left undefined. A Bates model without them is therefore a Bates model with those jumps, not a Heston model.

The program prices a Heston model with and without `mu`, and a Bates model without jump fields, with the defaults written out, and with a zero intensity.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto v = [](double x) { return pluss::make_series({x}); };
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0), .market = fin::market::make(100.0, 0.02, 0.0)};
  field.controls.engine = mod::Engine::Cos;
  const auto heston = [&](fin::model::Heston h) {
    h.kappa = v(1.5); h.theta = v(0.04); h.sigma = v(0.3); h.rho = v(-0.7); h.v0 = v(0.04);
    return h;
  };
  field.model = heston({});
  out.writeln("heston                       ", field.price().price.items<double>());
  field.model = heston({.mu = v(0.5)});
  out.writeln("heston with mu = 0.5         ", field.price().price.items<double>());

  fin::model::Bates bates{.kappa = v(1.5), .theta = v(0.04), .sigma = v(0.3), .rho = v(-0.7), .v0 = v(0.04)};
  field.model = bates;
  out.writeln("bates without jump parameters", field.price().price.items<double>());
  bates.jump_intensity = v(0.15); bates.jump_mean = v(-0.08); bates.jump_vol = v(0.20);
  field.model = bates;
  out.writeln("bates with 0.15, -0.08, 0.20 ", field.price().price.items<double>());
  bates.jump_intensity = v(0.0);
  field.model = bates;
  field.model = bates;
  out.writeln("bates with no jumps          ", field.price().price.items<double>(), " heston", field.price(heston({})).price.items<double>());
}
```

Output

```text
heston                        [8.62811]
heston with mu = 0.5          [8.62811]
bates without jump parameters [9.18701]
bates with 0.15, -0.08, 0.20  [9.18701]
bates with no jumps           [8.62811]  heston [8.62811]
```

The two expansions differ in the fourth decimal in the wings (23.3799 and 23.3804 at strike 80; 4.55844 and 4.55842 at strike 110) and agree at the money: the correction terms are the same to first order. The normal quote prices differ from the lognormal one in the third or fourth decimal, because the two are different approximations to the same price: both expansions are only accurate to the order of the terms kept. The Hagan and Obloj normal-volatility prices coincide here. The forward the `Field` used is {math}`100\,e^{0.02}=102.0201`, and the implied volatility of its price at strike 90 is 0.2213, the SABR Black volatility at that forward: the `Analytic` engine is the expansion priced through Black and nothing more. For long maturities or a large {math}`\nu` the expansion is less accurate; simulate (`MonteCarlo`) and compare.

## Program 5: optional fields and defaults

Several fields of the model structs are optional and may be left undefined: `mu` and `variance_floor` in the Heston family, `alpha_floor` in SABR, and the three jump fields of the Bates model. The rules, shown by the program, are:

- **`mu` is not used by the `Field`.** The drift of the asset is always {math}`r-q` from the market, so a `mu` set on the model has no effect on a price or a simulation made through the `Field`. It exists for the standalone path generators, where there is no market.
- **`variance_floor` and `alpha_floor`** are floors applied to the variance (or to {math}`\alpha`) in a discretised simulation, to keep the Euler step from producing a negative value. They affect simulation only, and default to {math}`10^{-12}`.
- **The Bates jump fields default** to `jump_intensity = 0.15`, `jump_mean = -0.08` and `jump_vol = 0.20` if left undefined. A Bates model without them is therefore a Bates model with those jumps, not a Heston model.

The program prices a Heston model with and without `mu`, and a Bates model without jump fields, with the defaults written out, and with a zero intensity.B

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto v = [](double x) { return pluss::make_series({x}); };
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({80.0, 90.0, 100.0, 110.0, 120.0}), .maturity = pluss::make_series({1.0})},
                   .market = fin::market::make(100.0, 0.02, 0.0)};
  field.controls.risk = mod::Risk::PriceOnly;
  const std::vector<std::pair<mod::Model, mod::Engine>> models{
      {fin::model::BlackScholes{.volatility = v(0.2)}, mod::Engine::Analytic},
      {fin::model::Sabr{.alpha0 = v(0.2), .beta = v(1.0), .nu = v(0.5), .rho = v(-0.4)}, mod::Engine::Analytic},
      {fin::model::Heston{.kappa = v(1.5), .theta = v(0.04), .sigma = v(0.3), .rho = v(-0.7), .v0 = v(0.04)}, mod::Engine::Cos},
      {fin::model::Bates{.kappa = v(1.5), .theta = v(0.04), .sigma = v(0.3), .rho = v(-0.7), .v0 = v(0.04), .jump_intensity = v(0.1), .jump_mean = v(-0.1), .jump_vol = v(0.15)}, mod::Engine::Cos},
  };
  out.writeln("implied vol at strike     80      90      100     110     120");
  for (const auto &[model, engine] : models) {
    field.model = model;
    field.controls.engine = engine;
    const auto smile = field.implied_vol().items<double>();
    std::string row = model.name() + std::string(26 - model.name().size(), ' ');
    for (std::size_t i = 0; i < smile.size(); ++i) {
      row += std::to_string(smile[i]).substr(0, 6) + "  ";
    }
    out.writeln(row);
  }
}
```

Output

```text
implied vol at strike     80      90      100     110     120
BlackScholes              0.2000  0.2000  0.2000  0.2000  0.2000  
Sabr                      0.2322  0.2158  0.2032  0.1945  0.1898  
Heston                    0.2281  0.2099  0.1926  0.1767  0.1633  
Bates                     0.2348  0.2170  0.1999  0.1841  0.1706  
```

Black-Scholes gives a flat 20%. Every other model slopes down across the strikes, from about 23% at strike 80 to between 16% and 19% at strike 120; this is the equity skew, which a constant-volatility model cannot produce. Heston's skew is the steepest (22.8% to 16.3%), since {math}`\rho=-0.7`. Bates lies above Heston by about 0.7 vol points at every strike, because the jumps add variance on top of the diffusion. SABR is flatter at the right end (19.0% at 120) since its correlation, {math}`-0.4`, is weaker. Reading a model's smile this way, from a `Field` and its `implied_vol()`, is the first thing to do with any new set of parameters: if the smile does not look like the market's, the calibration of Page 5 will not fit.

## Checklist

1. Know the parameter order of the model you optimise: `parameters()` is `kappa, theta, sigma, rho, v0` for Heston, and the others in the table above.
2. Write parameters through `parameters(...)` and read one parameter through `as<M>()`; `holds<M>()` before `as<M>()` when the model is not known in advance.
3. Parameters set from a `Float32` `Series` are stored in `Float32`; use the numeric overloads or a `Float64` `Series`.
4. Do not rely on `mu`: the `Field` always drifts at {math}`r-q`.
5. State the Bates jump fields explicitly, or accept the documented defaults (0.15, −0.08, 0.20) on purpose.
6. Compare models by their implied-volatility smile, not by price alone.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="engines_and_greeks.html">
    <span class="label">Previous</span>
    <span class="title">Engines and Greeks</span>
  </a>
  <a class="next-link" href="losses_and_objectives.html">
    <span class="label">Next</span>
    <span class="title">Losses and objectives</span>
  </a>
</div>
<!-- apiref:nav:end -->
