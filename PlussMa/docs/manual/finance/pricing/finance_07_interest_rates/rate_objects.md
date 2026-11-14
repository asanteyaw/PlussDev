<!-- generated from rate_objects.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Rate model objects

[Back to Interest Rates](../finance_07_interest_rates.md)

The functions of the previous pages take a parameter struct each time. The classes of `finance::models` wrap a parameter struct and give a model a name that can be stored, passed around and replaced: `VasicekRate`, `CirRate`, `HullWhiteRate` and `HjmRate`. The first three derive from `ShortRateBase`, so a piece of code written against the base class works for all of them, which is what lets you compare models with one loop.

```cpp
models::VasicekRate::VasicekRate(rates::VasicekParams params = {});
models::CirRate::CirRate(rates::CirParams params = {});
models::HullWhiteRate::HullWhiteRate(rates::HullWhiteParams params = {});
models::HjmRate::HjmRate(rates::HjmParams params = {});
const rates::VasicekParams &models::VasicekRate::params() const noexcept;
void models::VasicekRate::set_params(const rates::VasicekParams &params);
rates::RatePaths models::ShortRateBase::paths(std::int64_t steps, std::int64_t num_paths, double dt, const rates::SimOptions &options = {}, Config cfg = {}, std::uint64_t seed = 0) const;
double models::ShortRateBase::zcb(double maturity, double short_rate) const;
rates::HjmPaths models::HjmRate::paths(std::int64_t steps, std::int64_t num_paths, double dt, const rates::SimOptions &options = {}, Config cfg = {}, std::uint64_t seed = 0) const;
double models::HjmRate::zcb_mc(double maturity, std::int64_t paths = 20000, std::int64_t steps = 252, rates::SimOptions options = {}, Config cfg = {}) const;
```

| Call | Meaning |
|---|---|
| construction | takes the parameters of the model; `params()` returns them and `set_params` replaces them (the `CirRate` and `HullWhiteRate` have the same two members for their own parameter structs) |
| `paths` | the simulation of the function of the same name, with the parameters held by the object |
| `zcb` | the closed-form zero-coupon price of the model, for a maturity and the current short rate |
| `HjmRate::paths`, `zcb_mc` | the forward-rate simulation and the simulated zero price; the HJM model has no short-rate base class, since it has no closed form |

Parameters are checked when they are used, not when the object is built, so a model with invalid parameters can be created and the error is reported by the first call that needs them.

## Program 1: three models in one loop

The program stores a Vasicek, a CIR and a Hull-White model behind the base class, with the same long-run level and starting rate, and for each prints the one-year and five-year bond prices and the mean of the rate two years ahead, simulated with `Transition`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
namespace mdl = fin::models;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  std::vector<std::pair<std::string, std::unique_ptr<mdl::ShortRateBase>>> models;
  models.emplace_back("Vasicek", std::make_unique<mdl::VasicekRate>(rt::VasicekParams{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05}));
  models.emplace_back("CIR", std::make_unique<mdl::CirRate>(rt::CirParams{.kappa = 0.7, .theta = 0.03, .sigma = 0.08, .r0 = 0.05}));
  models.emplace_back("Hull-White", std::make_unique<mdl::HullWhiteRate>(rt::HullWhiteParams{.kappa = 0.7, .sigma = 0.01, .r0 = 0.05, .theta0 = 0.03}));

  out.writeln("model       P(1)      P(5)      mean of the rate in 2 years (simulated)");
  for (const auto &[name, model] : models) {
    rt::SimOptions options;
    options.scheme = rt::Scheme::Transition;
    options.seed = 1;
    const auto paths = model->paths(40, 100000, 0.05, options, as_f64());
    out.writeln(name, model->zcb(1.0, 0.05), model->zcb(5.0, 0.05), paths.rate.select(0, 40).mean().item<double>());
  }
}
```

Output

```text
model       P(1)      P(5)      mean of the rate in 2 years (simulated)
Vasicek 0.95660 0.83744 0.03488
CIR 0.95662 0.83779 0.03494
Hull-White 0.95660 0.83744 0.03491
```

The loop uses only the base-class interface, `zcb` and `paths`, so a fourth model with the same interface would be added to the vector and the loop would not change. The prices are those of the closed forms (0.95660, 0.83744 for Vasicek and Hull-White, which coincide with a flat level; 0.95662, 0.83779 for CIR), and the mean rate in two years, 0.0349, is the same under the three models, since each has the same mean {math}`\theta+(r_0-\theta)e^{-\kappa t}`.

## Program 2: changing parameters, and the HJM object

`set_params` replaces the parameters of an object, which is how a stress test or a calibration loop moves a model. The program prices a five-year bond, lowers the speed to 0.3 and doubles the volatility, and prices it again; it creates an `HjmRate`, simulates its forward curves and prices a two-year zero-coupon bond; and it shows that an invalid parameter is an error from the first call that uses it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
namespace mdl = fin::models;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mdl::VasicekRate model{rt::VasicekParams{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05}};
  out.writeln("kappa", model.params().kappa, " 5-year price", model.zcb(5.0, 0.05));
  rt::VasicekParams stressed = model.params();
  stressed.kappa = 0.3;
  stressed.sigma = 0.02;
  model.set_params(stressed);
  out.writeln("kappa", model.params().kappa, " 5-year price", model.zcb(5.0, 0.05));

  mdl::HjmRate hjm{rt::HjmParams{.r0 = 0.03, .tenors = {0.25, 0.5, 1.0, 2.0, 5.0}, .vol = {0.012, 0.011, 0.010, 0.009, 0.007}}};
  const auto sim = hjm.paths(26, 3, 1.0 / 52.0, {}, as_f64());
  out.writeln("hjm forward", sim.forward.sizes(), " zero price (2y)", hjm.zcb_mc(2.0, 100000, 100, {}, as_f64()));

  try {
    mdl::VasicekRate invalid{rt::VasicekParams{.kappa = -1.0}};
    invalid.zcb(1.0, 0.03);
  } catch (const std::exception &e) {
    std::string message = e.what();
    out.writeln(message.substr(0, message.find('\n')));
  }
}
```

Output

```text
kappa 0.70000  5-year price 0.83744
kappa 0.30000  5-year price 0.81982
hjm forward [27, 3, 5]  zero price (2y) 0.94180
subsp::math::finance::rates::vasicek_zcb: kappa must be finite and > 0
```

Slowing the reversion from 0.7 to 0.3 and doubling the volatility changes the five-year price from 0.83744 to 0.81982: the object keeps the other parameters and `params()` shows the current ones. The HJM object returns forward curves of shape `[27, 3, 5]` (26 steps, three paths, five tenors) and a two-year price of 0.94180. The last line is the error of a model constructed with `kappa = -1`: the construction succeeds and the first `zcb` call reports "kappa must be finite and > 0", naming the function and the field.

## Checklist

1. Hold models behind `ShortRateBase` when code should not care which of the three it has.
2. Replace parameters with `set_params`; read them with `params()`.
3. Expect parameter errors at the first use, not at construction.
4. Use the object when a model is passed between functions or stored, and the free functions when a single call is enough.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="forward_rate_models.html">
    <span class="label">Previous</span>
    <span class="title">Forward-rate models</span>
  </a>
</div>
<!-- apiref:nav:end -->
