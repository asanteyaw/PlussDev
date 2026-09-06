<!-- pluss-hand-authored -->

# `lymph::calibrate_with_platt`

[Back to `metrics`](../metrics.md)

`calibrate_with_platt` and its three siblings —
`calibrate_with_isotonic`, `calibrate_with_temperature`, and
`without_calibration` — are **`TrainOptions` presets**: each takes a
`TrainOptions` and returns it with only the score-calibration fields changed.
Hand the result to `Model::learn(...)` and the fitted model maps its raw scores
through a calibrator before producing the probabilities that `infer` and
probability-based metrics use.

## Signature

```cpp
lymph::TrainOptions lymph::calibrate_with_platt(
    lymph::TrainOptions options = {},
    std::int64_t calibration_iterations = 400,
    double calibration_tolerance = 1e-6);

lymph::TrainOptions lymph::calibrate_with_isotonic(lymph::TrainOptions options = {});

lymph::TrainOptions lymph::calibrate_with_temperature(
    lymph::TrainOptions options = {},
    std::int64_t calibration_iterations = 400,
    double calibration_tolerance = 1e-6);

lymph::TrainOptions lymph::without_calibration(lymph::TrainOptions options = {});
```

All four live directly at `lymph::` scope.

## What It Does

Each helper copies the passed `options` and sets:

| Helper | `options.calibration` | Also sets |
|--------|-----------------------|-----------|
| `calibrate_with_platt` | `ScoreCalibration::Platt` | `calibration_iterations`, `calibration_tolerance` |
| `calibrate_with_isotonic` | `ScoreCalibration::Isotonic` | — |
| `calibrate_with_temperature` | `ScoreCalibration::Temperature` | `calibration_iterations`, `calibration_tolerance` |
| `without_calibration` | `ScoreCalibration::None` | — |

Every other field of `options` is left as it was, so these compose with training
settings you already configured. `without_calibration` is the way to strip
calibration back off a `TrainOptions` that has it.

- **Platt** — fits a 1-D logistic (sigmoid) on the held scores.
- **Isotonic** — fits a monotone step function; more flexible, needs more data.
- **Temperature** — a single scalar that rescales the logits; the lightest fix
  for an over/under-confident model.

## Parameters

`options` — `lymph::TrainOptions`
: The base training options to layer calibration onto. Defaulted, so
  `calibrate_with_platt()` alone gives a fresh `TrainOptions` with just
  calibration set.

`calibration_iterations` — `std::int64_t` *(Platt / temperature)*
: Max optimizer iterations for the calibration fit. Default `400`.

`calibration_tolerance` — `double` *(Platt / temperature)*
: Convergence tolerance for that fit. Default `1e-6`.

## Returns

A `lymph::TrainOptions` — pass it straight to `Model::learn` /
`Plan::run` / any API that takes `TrainOptions`.

## Errors

- None at call time; these are pure struct edits. A calibration fit that cannot
  converge surfaces later, during `learn`.

## Practical Examples

<!-- these examples establish: platt on/off at learn time, the four modes, and composition with existing options. -->

### Example 1

`calibrate_with_platt()` at `learn` time sets the mode and the fit knobs.
Calibration reshapes the **probabilities** `infer` and probability-based metrics
use; the raw `infer_scores` decision function is deliberately untouched.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 5.5,
       6.0, 6.5, 7.0, 7.5, 8.0, 8.5}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x", Thread(x)}, {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"x"};
  data_options.label_feature = "target";

  const lymph::TrainOptions calibrated = lymph::calibrate_with_platt();
  std::cout << "calibration mode       = " << static_cast<int>(calibrated.calibration)
            << " (Platt)\n";
  std::cout << "calibration_iterations = " << calibrated.calibration_iterations << "\n";
  std::cout << "calibration_tolerance  = " << calibrated.calibration_tolerance << "\n";

  const lymph::models::Model plain =
      lymph::models::logistic_regression().learn(frame, data_options);
  const lymph::models::Model platt =
      lymph::models::logistic_regression().learn(frame, data_options, calibrated);

  std::cout << "platt model trained    = " << std::boolalpha << platt.trained() << "\n";
  std::cout << "infer_scores identical = "
            << (lymph::metrics::max_abs_diff(plain.infer_scores(frame, data_options),
                                             platt.infer_scores(frame, data_options)) == 0.0)
            << "  (calibration does not touch the raw decision function)\n";

  return 0;
}
```

Output

```text
calibration mode       = 1 (Platt)
calibration_iterations = 400
calibration_tolerance  = 1e-06
platt model trained    = true
infer_scores identical = true  (calibration does not touch the raw decision function)
```

### Example 2

The four modes and their `ScoreCalibration` values.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  const lymph::TrainOptions none = lymph::without_calibration();
  const lymph::TrainOptions platt = lymph::calibrate_with_platt();
  const lymph::TrainOptions isotonic = lymph::calibrate_with_isotonic();
  const lymph::TrainOptions temperature = lymph::calibrate_with_temperature();

  std::cout << "without_calibration    -> " << static_cast<int>(none.calibration) << "\n";
  std::cout << "calibrate_with_platt   -> " << static_cast<int>(platt.calibration) << "\n";
  std::cout << "calibrate_with_isotonic -> " << static_cast<int>(isotonic.calibration) << "\n";
  std::cout << "calibrate_with_temperature -> " << static_cast<int>(temperature.calibration) << "\n";

  return 0;
}
```

Output

```text
without_calibration    -> 0
calibrate_with_platt   -> 1
calibrate_with_isotonic -> 2
calibrate_with_temperature -> 3
```

### Example 3

Layering calibration onto a `TrainOptions` you already configured — and
stripping it back off.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::TrainOptions base{};
  base.max_iterations = 750;
  base.tolerance = 1e-7;
  base.random = {.seed = 42ULL, .deterministic = true};

  const lymph::TrainOptions with_platt = lymph::calibrate_with_platt(base, 200);
  std::cout << "max_iterations kept       = " << with_platt.max_iterations << "\n";
  std::cout << "seed kept                 = " << with_platt.random.seed << "\n";
  std::cout << "calibration               = " << static_cast<int>(with_platt.calibration) << "\n";
  std::cout << "calibration_iterations    = " << with_platt.calibration_iterations << "\n";

  const lymph::TrainOptions stripped = lymph::without_calibration(with_platt);
  std::cout << "after without_calibration  = " << static_cast<int>(stripped.calibration)
            << "  (max_iterations still " << stripped.max_iterations << ")\n";

  return 0;
}
```

Output

```text
max_iterations kept       = 750
seed kept                 = 42
calibration               = 1
calibration_iterations    = 200
after without_calibration  = 0  (max_iterations still 750)
```

## See Also

- [`Threshold`](Threshold.md) — after calibrating, pick the operating point.
- `lymph::options::Train` — the `calibration` / `calibration_iterations` /
  `calibration_tolerance` fields these helpers set.
- [`models::Model`](../models/Model/learn.md) — where the returned `TrainOptions`
  is consumed.
