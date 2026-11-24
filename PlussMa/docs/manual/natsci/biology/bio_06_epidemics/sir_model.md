# The SIR model

[Back to Biology 6: Epidemics](../bio_06_epidemics.md)

An infection moves through a population as people pass from susceptible to infective to recovered. The SIR model follows the three fractions in time. This page reads the outcome of an epidemic straight from its two parameters, confirms it by integrating the model, and runs three contact rates in a single batch.

## How it works

`sir_rate(x, beta, gamma)` returns the rates of the state `(S, I, R)`: susceptibles are infected at `beta S I / N` and infectives recover at `gamma I`. The ratio `sigma = beta / gamma` is the contact number, the infections one case causes while infective, and an epidemic takes off only if `sigma S0 > 1`. Two outcomes have closed forms. `final_susceptible(sigma, s0, i0)` solves for the susceptible fraction that remains when the epidemic has run out, and `peak_infective(sigma, s0, i0)` is the highest infective fraction reached. The rate function takes arrays for `beta` and `gamma` as well as for the state, so a batch of epidemics is one call to `dynamics::simulate`.

## Outcome from the parameters

The population starts with `99%` susceptible and `1%` infective, with `beta = 0.5` and `gamma = 0.2` per day. The program prints the closed-form final size and peak, integrates the model to four days, and then runs `beta = 0.3`, `0.5` and `0.8` as one batch.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace epi = biology::epidemiology;
namespace dynamics = biology::dynamics;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a population of fractions: 99% susceptible, 1% infective. Contacts spread infection at 0.5 per day,
  // and an infective recovers at 0.2 per day.
  const double beta = 0.5, gamma = 0.2;
  const double contact_number = beta / gamma;
  out.writeln("contact number sigma = beta / gamma = ", contact_number);

  // closed-form final size and peak
  const auto sigma = vec({contact_number}), s0 = vec({0.99}), i0 = vec({0.01});
  out.writeln("susceptible left at the end  = ", flat(epi::final_susceptible(sigma, s0, i0)));
  out.writeln("peak infective fraction      = ", flat(epi::peak_infective(sigma, s0, i0)));

  // integrate the model and compare
  const auto rate = [&](const pluss::Series &x) { return epi::sir_rate(x, beta, gamma); };
  const auto start = vec({0.99, 0.01, 0.0});
  for (double t : {10.0, 20.0, 40.0, 200.0})
    out.writeln("day ", t, ": (S, I, R) = ", flat(dynamics::simulate(rate, start, 0.0, t, 4000)));

  // the same epidemic under three contact rates in one batch
  const auto batch = vec({0.99, 0.01, 0.0, 0.99, 0.01, 0.0, 0.99, 0.01, 0.0}).reshape({3, 3});
  const auto betas = vec({0.3, 0.5, 0.8});
  const auto batch_rate = [&](const pluss::Series &x) { return epi::sir_rate(x, betas, gamma); };
  const auto end = dynamics::simulate(batch_rate, batch, 0.0, 300.0, 6000);
  out.writeln("recovered fraction at day 300 for beta = 0.3, 0.5, 0.8: ", flat(end.select(-1, 2)));
}
```

Output

```text
contact number sigma = beta / gamma = 2.5000
susceptible left at the end  = [0.1059]
peak infective fraction      = [0.2375]
day 10.0000: (S, I, R) = [0.7644, 0.1322, 0.1035]
day 20.0000: (S, I, R) = [0.2688, 0.2097, 0.5214]
day 40.0000: (S, I, R) = [0.1125, 0.0177, 0.8697]
day 200.0000: (S, I, R) = [0.1059, 0.0000, 0.8941]
recovered fraction at day 300 for beta = 0.3, 0.5, 0.8: [0.5936, 0.8941, 0.9804]
```

The contact number is `2.5`, so the epidemic takes off; the closed forms say the infective fraction peaks at `0.2375` and that `0.1059` of the population is never infected. The integration agrees: at day `20` the infective fraction is `0.2097` and falling after its peak, and by day `200` the susceptible fraction has settled at `0.1059`. The last line shows that the contact rate matters sharply: at `beta = 0.3` (`sigma = 1.5`) `59%` of the population is eventually infected, at `0.5` `89%`, and at `0.8` `98%`.

<!-- addex:start -->
## A latent period

Many infections have a latent period in which a person is infected but not yet infectious. The SEIR model adds a compartment for them: `epidemiology::seir_rate(x, beta, sigma, gamma, mu)` advances the state `(S, E, I, R)`, where {math}`1/\sigma` is the average latent period, and `seir_r0` is the basic reproduction number {math}`R_0 = \beta\sigma/((\sigma + \mu)(\gamma + \mu))`. The program uses {math}`\beta = 0.6`, {math}`\sigma = 0.25` and {math}`\gamma = 0.2` per day with no births or deaths.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace epi = biology::epidemiology;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // SEIR: infected people are not infectious for an average of 1/sigma = 4 days (exposed), then recover at gamma = 0.2 per day
  const double beta = 0.6, sigma = 0.25, gamma = 0.2, mu = 0.0;
  out.writeln("basic reproduction number R0 = ", flat(epi::seir_r0(vec({beta}), vec({sigma}), vec({gamma}), vec({mu}))));
  const auto rate = [&](const pluss::Series &x) { return epi::seir_rate(x, beta, sigma, gamma); };
  const auto start = vec({0.99, 0.0, 0.01, 0.0});   // (S, E, I, R)
  for (double t : {20.0, 40.0, 60.0, 150.0}) out.writeln("day ", t, ": (S, E, I, R) = ", flat(dynamics::simulate(rate, start, 0.0, t, 6000)));
}
```

Output

```text
basic reproduction number R0 = [3.0000]
day 20.0000: (S, E, I, R) = [0.6586, 0.1125, 0.0931, 0.1359]
day 40.0000: (S, E, I, R) = [0.1198, 0.0603, 0.1160, 0.7039]
day 60.0000: (S, E, I, R) = [0.0632, 0.0047, 0.0149, 0.9172]
day 150.0000: (S, E, I, R) = [0.0588, 0.0000, 0.0000, 0.9412]
```

{math}`R_0 = 3.0000`. The infection is spread over about two months (the infective fraction is still `0.1160` at day `40`), and by day `150` it has run its course with `0.0588` still susceptible, which is the same final size as the SIR model gives for a contact number of `3` in the table below: the latent period changes the timing but not the final size.

## How the outcome depends on the contact number

The final size and the peak have closed forms in the contact number, so a whole range of epidemics is one call. The program evaluates five contact numbers from `1.2` to `5`, with `99%` susceptible and `1%` infective at the start.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace epi = biology::epidemiology;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // contact numbers from just above 1 to 5 in one call: closed-form final size and peak
  const auto sigma = vec({1.2, 1.5, 2.0, 3.0, 5.0});
  const auto s0 = vec({0.99, 0.99, 0.99, 0.99, 0.99}), i0 = vec({0.01, 0.01, 0.01, 0.01, 0.01});
  out.writeln("contact number          ", flat(sigma));
  out.writeln("susceptible at the end  ", flat(epi::final_susceptible(sigma, s0, i0)));
  out.writeln("fraction ever infected  ", flat(s0.add(i0).sub(epi::final_susceptible(sigma, s0, i0))));
  out.writeln("peak infective fraction ", flat(epi::peak_infective(sigma, s0, i0)));
}
```

Output

```text
contact number          [1.2000, 1.5000, 2.0000, 3.0000, 5.0000]
susceptible at the end  [0.6522, 0.4064, 0.1998, 0.0588, 0.0069]
fraction ever infected  [0.3478, 0.5936, 0.8002, 0.9412, 0.9931]
peak infective fraction [0.0231, 0.0697, 0.1585, 0.3038, 0.4801]
```

Just above the threshold of `1`, at `1.2`, an epidemic reaches `34.78%` of the population, peaks at only `2.31%` infective and leaves `65.22%` susceptible. At `3` it reaches `94.12%` with a peak of `30.38%`, and at `5` nearly everyone is infected (`99.31%`). The final size depends steeply on the contact number only near the threshold.

## Infections that return

If recovery gives no immunity the infection can stay in the population indefinitely. `sis_rate(x, beta, gamma)` advances the state `(S, I)`, with recovered people returning to the susceptible class. The program runs three contact rates for `400` days and compares the infected fraction with the prediction {math}`1 - \gamma/\beta`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace epi = biology::epidemiology;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // SIS: recovery returns people to the susceptible state, so the infection can stay: the endemic level is 1 - gamma/beta
  const double gamma = 0.2;
  for (double beta : {0.15, 0.4, 0.8}) {
    const auto rate = [&](const pluss::Series &x) { return epi::sis_rate(x, beta, gamma); };
    const auto end = flat(dynamics::simulate(rate, vec({0.95, 0.05}), 0.0, 400.0, 8000));
    out.writeln("beta = ", beta, ": infected fraction after 400 days = ", end[1], ", predicted ", beta > gamma ? 1.0 - gamma / beta : 0.0);
  }
}
```

Output

```text
beta = 0.1500: infected fraction after 400 days = 0.0000, predicted 0.0000
beta = 0.4000: infected fraction after 400 days = 0.5000, predicted 0.5000
beta = 0.8000: infected fraction after 400 days = 0.7500, predicted 0.7500
```

With {math}`\beta = 0.15`, below {math}`\gamma = 0.2`, the infection dies out (`0.0000`); with {math}`\beta = 0.4` it settles at an infected fraction of `0.5000` and with {math}`\beta = 0.8` at `0.7500`, exactly {math}`1 - \gamma/\beta`.

## Arguments that are checked

The final-size formula needs a positive contact number, and each model needs a state with the right number of compartments. The program passes a contact number of zero, and a two-compartment state to the three-compartment model.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace epi = biology::epidemiology;
int main() {
  OutputEngine out;
  // the final-size formula requires an epidemic that can take off, and positive sizes
  try {
    (void)epi::final_susceptible(vec({0.0}), vec({0.99}), vec({0.01}));
  } catch (const pluss::Error &e) {
    out.writeln("zero contact number: ", e.message());
  }
  try {
    (void)epi::sir_rate(vec({0.9, 0.1}), 0.5, 0.2);
  } catch (const pluss::Error &e) {
    out.writeln("two compartments given to the three-compartment model: ", e.message());
  }
}
```

Output

```text
zero contact number:  natsci: final_size: contact number must be strictly positive
two compartments given to the three-compartment model:  natsci: sir: state must have 3 species on its last axis
```

Both raise a `pluss::Error` that names the function and states the requirement.

<!-- addex:end -->

## Decisions

- Use the closed forms `final_susceptible` and `peak_infective` when you want the outcome; integrate only when the timing matters.
- Use `seir_rate` when the latent period is a sizeable part of the infectious period, and `sis_rate` when recovery gives no immunity.
- Pass `beta` and `gamma` as rates per unit time, and compare the contact number with `1` first.

## Pitfalls

`gamma` is a rate, not a duration. If a person is infective for `5` days, the rate is `0.2` per day, not `5`. The program computes the contact number and the final size for `gamma = 0.2` and for `gamma = 5`, and then the peak for a contact number below one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace epi = biology::epidemiology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // gamma is a rate (per day), not the length of the infectious period: an infectious period of 5 days is gamma = 1/5
  const auto s0 = vec({0.99}), i0 = vec({0.01});
  for (double gamma : {0.2, 5.0}) {
    const double sigma = 0.5 / gamma;
    out.writeln("gamma = ", gamma, ": contact number ", sigma, ", susceptible left ", flat(epi::final_susceptible(vec({sigma}), s0, i0)));
  }
  // below the threshold no epidemic takes off
  out.writeln("sigma = 0.8, peak infective ", flat(epi::peak_infective(vec({0.8}), s0, i0)));
}
```

Output

```text
gamma = 0.2000: contact number 2.5000, susceptible left [0.1059]
gamma = 5.0000: contact number 0.1000, susceptible left [0.9889]
sigma = 0.8, peak infective [0.0415]
```

With `gamma = 0.2` the contact number is `2.5000` and `0.1059` of the population is never infected; with `gamma = 5` it is `0.1000` and the epidemic barely starts (`0.9889` left). Below the threshold (`sigma = 0.8`) the formula still returns a peak, `0.0415`, which is only the starting infection decaying: it is not a prediction of an outbreak.

## Where next

[Pharmacokinetics](../bio_07_pharmacokinetics/compartment_models.md) is another linear-flow system, and [Biology 21: Delays](../bio_21_delays.md) adds a delay to the recovery.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
