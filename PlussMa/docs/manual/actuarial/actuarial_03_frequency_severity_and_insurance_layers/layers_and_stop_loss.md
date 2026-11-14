<!-- generated from layers_and_stop_loss.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Layers and Stop-Loss

[Back to Frequency, Severity and Insurance Layers](../actuarial_03_frequency_severity_and_insurance_layers.md)

Reinsurance divides a loss among several payers, each responsible for a **layer**: the part of the loss between an *attachment point* {math}`a` and the attachment plus a *limit* {math}`\ell`. A layer pays nothing for a loss below {math}`a`, the excess above {math}`a` up to the limit, and the limit in full for a larger loss,

```{math}
L_{a,\ell}(X)=\min\bigl((X-a)_+,\ \ell\bigr),
```

and a reinsurer that takes only a fraction {math}`s` of the layer pays {math}`s\,L_{a,\ell}(X)`. Two special cases have names. With no limit the layer is the **excess of loss** above {math}`a`, {math}`(X-a)_+`; its expected value {math}`E[(X-a)_+]` is the **stop-loss premium**.

Layers that follow one another partition a loss: the layer from 0 to {math}`a`, the layer from {math}`a` to {math}`a+\ell`, and the excess above {math}`a+\ell` add to the ground-up loss. That identity is a useful check on any layer calculation.

```cpp
actuarial::LossTransform actuarial::operators::layer(double attachment, double limit, double share = 1.0);
pluss::Series losses::excess_loss(const pluss::Series &losses, double attachment);
pluss::Series losses::loss_layer(const pluss::Series &losses, double attachment, double limit);
pluss::Series losses::layer_premium(const pluss::Series &losses, double attachment, double limit);
pluss::Series losses::stoploss(const pluss::Series &losses, double deductible);
```

| Function | Returns | Domain |
|---|---|---|
| `operators::layer(attachment, limit, share)` | a transform giving `share · min((X − attachment)+, limit)` per loss | `attachment` ≥ 0, `limit` ≥ 0, `share` in [0, 1] |
| `losses::excess_loss(losses, attachment)` | `(X − attachment)+` per loss | `attachment` ≥ 0 |
| `losses::loss_layer(losses, attachment, limit)` | `min((X − attachment)+, limit)` per loss | `attachment` ≥ 0, `limit` ≥ 0 |
| `losses::layer_premium(losses, attachment, limit)` | the average of the layer over the sample: the empirical expected layer payment | |
| `losses::stoploss(losses, deductible)` | the average of `(X − deductible)+` over the sample: the empirical stop-loss premium | `deductible` ≥ 0 |

The per-loss functions return a `Series` of the sample's length and the averages a rank-zero `Series`. The loss sample must be one-dimensional, finite and non-negative.

## A layer on one sample

Six losses. The layer of 500 excess of 250 pays nothing for the first two, and for the others the excess over 250, up to 500. The operator with a share of one half pays half of that.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  out.writeln("losses                =", x.items<double>());
  out.writeln("excess over 250       =", losses::excess_loss(x, 250.0).items<double>());
  out.writeln("layer 500 xs 250      =", losses::loss_layer(x, 250.0, 500.0).items<double>());
  out.writeln("operator, share 1     =", actuarial::operators::layer(250.0, 500.0)(x).items<double>());
  out.writeln("operator, share 0.5   =", actuarial::operators::layer(250.0, 500.0, 0.5)(x).items<double>());
}
```

Output

```text
losses                = [80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
excess over 250       = [0.000000, 0.000000, 50.000000, 450.000000, 950.000000, 3750.000000]
layer 500 xs 250      = [0.000000, 0.000000, 50.000000, 450.000000, 500.000000, 500.000000]
operator, share 1     = [0.000000, 0.000000, 50.000000, 450.000000, 500.000000, 500.000000]
operator, share 0.5   = [0.000000, 0.000000, 25.000000, 225.000000, 250.000000, 250.000000]
```

## The premium of a layer

The expected payment of the layer per loss is the average of its per-loss payments. `layer_premium` computes it directly, and `stoploss` is the same average for the unlimited excess. The program checks both against the mean of the per-loss series.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  const actuarial::LossMeasure mean = actuarial::operators::mean();
  out.writeln("layer_premium(250, 500)   =", losses::layer_premium(x, 250.0, 500.0).item<double>(), " mean of the layer =", mean(losses::loss_layer(x, 250.0, 500.0)).item<double>());
  out.writeln("stoploss(250)             =", losses::stoploss(x, 250.0).item<double>(), " mean of the excess =", mean(losses::excess_loss(x, 250.0)).item<double>());
}
```

Output

```text
layer_premium(250, 500)   = 250.000000  mean of the layer = 250.000000
stoploss(250)             = 866.666667  mean of the excess = 866.666667
```

## Layers partition the loss

Cut the loss at 250 and at 1,000: the layer from 0 to 250, the layer of 750 excess of 250, and the excess above 1,000 add to each ground-up loss exactly.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  const pluss::Series bottom = losses::loss_layer(x, 0.0, 250.0);
  const pluss::Series middle = losses::loss_layer(x, 250.0, 750.0);
  const pluss::Series top = losses::excess_loss(x, 1000.0);
  out.writeln("0 to 250         =", bottom.items<double>());
  out.writeln("750 xs 250       =", middle.items<double>());
  out.writeln("excess of 1000   =", top.items<double>());
  out.writeln("their sum        =", bottom.add(middle).add(top).items<double>());
  out.writeln("the losses       =", x.items<double>());
}
```

Output

```text
0 to 250         = [80.000000, 150.000000, 250.000000, 250.000000, 250.000000, 250.000000]
750 xs 250       = [0.000000, 0.000000, 50.000000, 450.000000, 750.000000, 750.000000]
excess of 1000   = [0.000000, 0.000000, 0.000000, 0.000000, 200.000000, 3000.000000]
their sum        = [80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
the losses       = [80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
```

## A share of a layer

The share of the operator is the reinsurer's participation. Two reinsurers taking 60% and 40% of the same layer together pay the whole layer.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  const pluss::Series first = actuarial::operators::layer(250.0, 500.0, 0.6)(x);
  const pluss::Series second = actuarial::operators::layer(250.0, 500.0, 0.4)(x);
  out.writeln("60% of the layer =", first.items<double>());
  out.writeln("40% of the layer =", second.items<double>());
  out.writeln("together         =", first.add(second).items<double>());
  out.writeln("the whole layer  =", losses::loss_layer(x, 250.0, 500.0).items<double>());
}
```

Output

```text
60% of the layer = [0.000000, 0.000000, 30.000000, 270.000000, 300.000000, 300.000000]
40% of the layer = [0.000000, 0.000000, 20.000000, 180.000000, 200.000000, 200.000000]
together         = [0.000000, 0.000000, 50.000000, 450.000000, 500.000000, 500.000000]
the whole layer  = [0.000000, 0.000000, 50.000000, 450.000000, 500.000000, 500.000000]
```

## Invalid layers fail explicitly

The factory checks the terms when it is called: a negative attachment point and a share above one are refused. A limit of zero is accepted and pays nothing, and the functions on a sample check the sample as the other loss transforms do.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
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
  report([&] { (void)actuarial::operators::layer(-1.0, 100.0); });
  report([&] { (void)actuarial::operators::layer(100.0, 100.0, 1.5); });
  report([&] { (void)losses::excess_loss(pluss::make_series({100.0, 200.0}), -1.0); });
  const pluss::Series x = pluss::make_series({80.0, 400.0});
  out.writeln("a layer with a limit of 0 pays", losses::loss_layer(x, 100.0, 0.0).items<double>());
}
```

Output

```text
operators::layer: attachment must be >= 0
operators::layer: share must be <= 1
operators::deductible: amount must be >= 0
a layer with a limit of 0 pays [0.000000, 0.000000]
```

## Where to go next

When the severity is a known law, the same expected payments are computed without a sample ([Limited Expected Values and Parametric Layers](limited_expected_values_and_parametric_layers.md)).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="deductibles_limits_and_coinsurance.html">
    <span class="label">Previous</span>
    <span class="title">Deductibles, Limits and Coinsurance</span>
  </a>
  <a class="next-link" href="limited_expected_values_and_parametric_layers.html">
    <span class="label">Next</span>
    <span class="title">Limited Expected Values and Parametric Layers</span>
  </a>
</div>
<!-- apiref:nav:end -->
