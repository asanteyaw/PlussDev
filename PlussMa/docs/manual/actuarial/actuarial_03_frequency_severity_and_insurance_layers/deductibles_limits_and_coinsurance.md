<!-- generated from deductibles_limits_and_coinsurance.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Deductibles, Limits and Coinsurance

[Back to Frequency, Severity and Insurance Layers](../actuarial_03_frequency_severity_and_insurance_layers.md)

An insurance contract rarely pays a loss in full. It applies terms that change what the insurer pays per loss, and each term is a function of the loss {math}`X`:

| Term | Insurer pays | Formula |
|---|---|---|
| Ordinary deductible {math}`d` | the loss above the deductible | {math}`(X-d)_+=\max(X-d,0)` |
| Policy limit {math}`u` | the loss up to the limit | {math}`\min(X,u)` |
| Coinsurance share {math}`\alpha` | a fraction of whatever is left | {math}`\alpha\,X` |

Terms apply in sequence, and the order matters. The usual order is deductible first, then the limit on what remains, then coinsurance: {math}`\alpha\,\min\bigl((X-d)_+,\,u\bigr)`. A different order is a different contract.

In this library each term is a **loss transform**, an object created once from its parameters and applied to any sample of losses. A **loss measure** reduces a sample to one number.

```cpp
actuarial::LossTransform actuarial::operators::deductible(double amount);
actuarial::LossTransform actuarial::operators::limit(double amount);
actuarial::LossTransform actuarial::operators::coinsurance(double share);
actuarial::LossMeasure actuarial::operators::mean();
pluss::Series actuarial::LossTransform::operator()(const pluss::Series &losses) const;
pluss::Series actuarial::LossMeasure::operator()(const pluss::Series &losses) const;
actuarial::Operator<Signature> actuarial::make_operator(std::string key, Callable &&callable);
```

| Factory | Parameter and domain | Returns |
|---|---|---|
| `deductible(amount)` | `amount` ≥ 0 | a transform giving `(X − amount)+` per loss |
| `limit(amount)` | `amount` ≥ 0 | a transform giving `min(X, amount)` per loss |
| `coinsurance(share)` | `share` in [0, 1] | a transform giving `share · X` per loss |
| `mean()` | | a measure giving the average of the sample, zero for an empty sample |
| `make_operator<Signature>(key, callable)` | a descriptive `key` and an invocable | an operator of the same kind, for a term of your own |

A transform maps a one-dimensional `Series` of losses to a `Series` of the same length; a measure maps it to a rank-zero `Series`. The loss sample must be one-dimensional, finite and non-negative. `key()` returns the name given at construction.

## The three terms on one sample

Six losses, from small to large. The deductible of 250 removes the first two and reduces the rest by 250; the limit of 1,000 caps the largest two; the coinsurance of 0.8 scales every loss.

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
  const actuarial::LossTransform excess = actuarial::operators::deductible(250.0);
  const actuarial::LossTransform cap = actuarial::operators::limit(1000.0);
  const actuarial::LossTransform share = actuarial::operators::coinsurance(0.8);
  const actuarial::LossMeasure mean = actuarial::operators::mean();
  out.writeln("losses       =", x.items<double>());
  out.writeln("deductible 250 =", excess(x).items<double>());
  out.writeln("limit 1000     =", cap(x).items<double>());
  out.writeln("coinsurance .8 =", share(x).items<double>());
  out.writeln("mean of the losses", mean(x).item<double>(), " of the deductible's payments", mean(excess(x)).item<double>());
}
```

Output

```text
losses       = [80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
deductible 250 = [0.000000, 0.000000, 50.000000, 450.000000, 950.000000, 3750.000000]
limit 1000     = [80.000000, 150.000000, 300.000000, 700.000000, 1000.000000, 1000.000000]
coinsurance .8 = [64.000000, 120.000000, 240.000000, 560.000000, 960.000000, 3200.000000]
mean of the losses 1071.666667  of the deductible's payments 866.666667
```

## Composing terms, and the order

Transforms are applied by calling one on the output of another. The program applies the usual order, deductible then limit then coinsurance, and then the limit before the deductible. In the first, the limit of 1,000 applies to what remains after the deductible, so the largest loss pays 0.8 times 1,000; in the second the limit applies to the ground-up loss and the deductible then comes off, so the largest payments are 750 before the coinsurance.

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
  const actuarial::LossTransform excess = actuarial::operators::deductible(250.0);
  const actuarial::LossTransform cap = actuarial::operators::limit(1000.0);
  const actuarial::LossTransform share = actuarial::operators::coinsurance(0.8);
  out.writeln("deductible, limit, coinsurance =", share(cap(excess(x))).items<double>());
  out.writeln("deductible then limit          =", cap(excess(x)).items<double>());
  out.writeln("limit then deductible          =", excess(cap(x)).items<double>());
}
```

Output

```text
deductible, limit, coinsurance = [0.000000, 0.000000, 40.000000, 360.000000, 760.000000, 800.000000]
deductible then limit          = [0.000000, 0.000000, 50.000000, 450.000000, 950.000000, 1000.000000]
limit then deductible          = [0.000000, 0.000000, 50.000000, 450.000000, 750.000000, 750.000000]
```

## What a deductible is worth

The mean of the transformed losses is the expected payment per loss. Raising the deductible removes small losses entirely and trims the rest, so the expected payment falls: a deductible of 1,000 removes half of the ground-up mean of this sample although four of its six losses are below 1,000. The program prints the expected payment per loss for four deductibles and the share of the ground-up mean that the deductible eliminates.

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
  const double ground_up = mean(x).item<double>();
  for (const double d : {0.0, 100.0, 500.0, 1000.0}) {
    const double paid = mean(actuarial::operators::deductible(d)(x)).item<double>();
    out.writeln("deductible", d, " expected payment", paid, " share eliminated", 1.0 - paid / ground_up);
  }
}
```

Output

```text
deductible 0.000000  expected payment 1071.666667  share eliminated 0.000000
deductible 100.000000  expected payment 975.000000  share eliminated 0.090202
deductible 500.000000  expected payment 733.333333  share eliminated 0.315708
deductible 1000.000000  expected payment 533.333333  share eliminated 0.502333
```

## A term of your own

A franchise deductible pays the whole loss once it exceeds the deductible, and nothing below it. It is not one of the standard factories, and `make_operator` builds it from a function with a key. The program wraps it, applies it, and prints its key.

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
  const actuarial::LossTransform franchise = actuarial::make_operator<pluss::Series(const pluss::Series &)>(
      "franchise-250", [](const pluss::Series &losses) { return losses.gt(250.0).where(losses, losses.mul(0.0)); });
  out.writeln("franchise 250 =", franchise(x).items<double>(), " key", std::string(franchise.key()), " defined", franchise.defined());
  out.writeln("ordinary 250  =", actuarial::operators::deductible(250.0)(x).items<double>(), " key", std::string(actuarial::operators::deductible(250.0).key()));
}
```

Output

```text
franchise 250 = [0.000000, 0.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]  key franchise-250  defined true
ordinary 250  = [0.000000, 0.000000, 50.000000, 450.000000, 950.000000, 3750.000000]  key deductible
```

## Invalid terms and samples fail explicitly

The factories check the contract terms when they are called, before any loss is seen: a negative deductible and a share above one are refused. Applying a transform checks the sample: it must be one-dimensional, finite and non-negative. An operator that was never built is refused when it is called.

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
  const actuarial::LossTransform excess = actuarial::operators::deductible(250.0);
  report([&] { (void)actuarial::operators::deductible(-1.0); });
  report([&] { (void)actuarial::operators::coinsurance(1.5); });
  report([&] { (void)excess(pluss::make_series({1.0, std::nan("")})); });
  report([&] { (void)excess(pluss::make_series({-5.0, 100.0})); });
  report([&] { (void)excess(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2})); });
  report([&] { (void)actuarial::LossTransform{}(pluss::make_series({1.0})); });
  out.writeln("a limit of 0 and a coinsurance of 0 are accepted: ", actuarial::operators::limit(0.0)(pluss::make_series({50.0})).items<double>()[0] == 0.0 ? "they pay nothing" : "unexpected");
}
```

Output

```text
operators::deductible: amount must be >= 0
operators::coinsurance: share must be <= 1
deductible: losses must be finite
deductible: losses must be >= 0
deductible: losses must be 1D
actuarial Operator is undefined
a limit of 0 and a coinsurance of 0 are accepted:  they pay nothing
```

## Where to go next

A band of losses between an attachment point and a limit is the loss transform of a reinsurance treaty ([Layers and Stop-Loss](layers_and_stop_loss.md)). Expected payments under a parametric severity need no sample ([Limited Expected Values and Parametric Layers](limited_expected_values_and_parametric_layers.md)).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="aggregate_losses.html">
    <span class="label">Previous</span>
    <span class="title">Aggregate Losses</span>
  </a>
  <a class="next-link" href="layers_and_stop_loss.html">
    <span class="label">Next</span>
    <span class="title">Layers and Stop-Loss</span>
  </a>
</div>
<!-- apiref:nav:end -->
