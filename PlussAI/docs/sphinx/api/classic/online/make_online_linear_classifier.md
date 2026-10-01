# `pluss::ai::classic::online::make_online_linear_classifier`

Construct an [`OnlineLinearClassifier`](OnlineLinearClassifier.md),
validating [`OnlineLinearOptions`](OnlineLinearOptions.md) immediately —
the entry point to reach for instead of the constructor whenever a bad
option should fail loudly and early, rather than silently waiting for the
first [`learn`](OnlineLinearClassifier.md#methods) call to surface it (see
[`OnlineLinearClassifier`'s Construction](OnlineLinearClassifier.md#construction)).

## Signature

```cpp
classic::StatusOr<classic::online::OnlineLinearClassifier> classic::online::make_online_linear_classifier(
    const classic::online::OnlineLinearOptions& options = {});
```

## What It Does

Validates every field of `options` against the constraints its objective
and regularization kind impose, then constructs an
[`OnlineLinearClassifier`](OnlineLinearClassifier.md) only once validation
passes -- an invalid option combination is rejected here, at construction
time, rather than surfacing later as an obscure failure the first time
[`learn`](OnlineLinearClassifier.md#methods) is called.

## Parameters

`options` (`OnlineLinearOptions`, default `{}`)
: Validated in full before anything is constructed — see
  [`OnlineLinearOptions`](OnlineLinearOptions.md) for every field, and
  `## Errors` below for exactly which combinations are rejected.

## Returns

A `StatusOr<OnlineLinearClassifier>` — `.ok()` and a fully-constructed
(but not yet `trained()`) classifier when `options` passes validation.

## Errors

- `options.family.objective` is neither `LinearObjective::Logistic` nor
  `::Hinge` — `Status::invalid_argument`.
- `options.family.regularization` is neither `RegularizationKind::None`
  nor `::L2` — `Status::invalid_argument`.
- `options.learning_rate` is not finite or `<= 0` —
  `Status::invalid_argument`.
- `options.family.regularization_strength` is not finite or `< 0` —
  `Status::invalid_argument`.

## Practical Examples

A successful construction, and the three distinct fields that can make
this call fail — `objective`, `regularization`, and `learning_rate`.

### Example 1

The default `OnlineLinearOptions` fails (its default `objective` is
`LeastSquares`, a regression-only value) — a realistic `Logistic`
configuration succeeds.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::StatusOr<cls::online::OnlineLinearClassifier> default_options =
      cls::online::make_online_linear_classifier();
  std::cout << "default OnlineLinearOptions ok = " << std::boolalpha << default_options.ok() << "\n";

  cls::online::OnlineLinearOptions logistic{};
  logistic.family.objective = cls::LinearObjective::Logistic;
  const cls::StatusOr<cls::online::OnlineLinearClassifier> made =
      cls::online::make_online_linear_classifier(logistic);
  std::cout << "Logistic objective ok           = " << made.ok() << "\n";
  std::cout << "trained() on a fresh classifier  = " << made.value().trained() << "\n";

  return 0;
}
```

Output

```text
default OnlineLinearOptions ok = false
Logistic objective ok           = true
trained() on a fresh classifier  = false
```

### Example 2

An unsupported `regularization` (`L1`, which the batch `Linear` family
allows but this incremental path does not).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Hinge;
  options.family.regularization = cls::RegularizationKind::L1;

  const cls::StatusOr<cls::online::OnlineLinearClassifier> made =
      cls::online::make_online_linear_classifier(options);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "OnlineLinearClassifier supports RegularizationKind::None or RegularizationKind::L2."
```

### Example 3

A non-positive `learning_rate`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  options.learning_rate = 0.0;

  const cls::StatusOr<cls::online::OnlineLinearClassifier> made =
      cls::online::make_online_linear_classifier(options);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "OnlineLinearOptions.learning_rate must be finite and > 0."
```

## Changelog

- **0.1** — `make_online_linear_classifier`.

## See Also

- [`OnlineLinearClassifier`](OnlineLinearClassifier.md) — the returned
  type, including the constructor this bypasses eager validation for.
- [`OnlineLinearOptions`](OnlineLinearOptions.md) — every field validated
  here.
- [`make_online_linear_regressor`](make_online_linear_regressor.md) — the
  regression counterpart.
