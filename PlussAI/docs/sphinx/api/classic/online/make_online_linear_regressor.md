# `pluss::ai::classic::online::make_online_linear_regressor`

Construct an [`OnlineLinearRegressor`](OnlineLinearRegressor.md),
validating [`OnlineLinearOptions`](OnlineLinearOptions.md) immediately —
the regression counterpart to
[`make_online_linear_classifier`](make_online_linear_classifier.md), for
the same reason: fail loudly at construction rather than waiting for the
first [`learn`](OnlineLinearRegressor.md#methods) call.

## Signature

```cpp
classic::StatusOr<classic::online::OnlineLinearRegressor> classic::online::make_online_linear_regressor(
    const classic::online::OnlineLinearOptions& options = {});
```

## Parameters

`options` (`OnlineLinearOptions`, default `{}`)
: Validated in full before anything is constructed — see
  [`OnlineLinearOptions`](OnlineLinearOptions.md) for every field, and
  `## Errors` below.

## Returns

A `StatusOr<OnlineLinearRegressor>` — `.ok()` and a fully-constructed
(but not yet `trained()`) regressor when `options` passes validation.

## Errors

- `options.family.objective` is neither `LinearObjective::LeastSquares`
  nor `::RobustLeast` — `Status::invalid_argument`.
- `options.family.regularization` is neither `RegularizationKind::None`
  nor `::L2` — `Status::invalid_argument`.
- `options.learning_rate` is not finite or `<= 0` —
  `Status::invalid_argument`.
- `options.family.regularization_strength` is not finite or `< 0` —
  `Status::invalid_argument`.
- `options.family.robust_transition` is not finite or `<= 0` — checked
  **unconditionally**, regardless of `objective` — `Status::invalid_argument`.
  See [Example 5](#example-5).

## Practical Examples

The default `OnlineLinearOptions` already succeeds here (its default
`objective` is `LeastSquares`, this class's own default), an unsupported
`regularization`, a non-positive `learning_rate`, a negative
`regularization_strength`, and an invalid `robust_transition` rejected
even though `LeastSquares` never reads it.

### Example 1

The default `OnlineLinearOptions` — unlike the classifier's factory, this
one succeeds with no changes at all, since `LeastSquares` is both the
default `objective` and one of this class's two accepted values.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::StatusOr<cls::online::OnlineLinearRegressor> made =
      cls::online::make_online_linear_regressor();
  std::cout << "default OnlineLinearOptions ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "trained() on a fresh regressor = " << made.value().trained() << "\n";

  return 0;
}
```

Output

```text
default OnlineLinearOptions ok = true
trained() on a fresh regressor = false
```

### Example 2

An unsupported `regularization` (`Elastic`).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::online::OnlineLinearOptions options{};
  options.family.regularization = cls::RegularizationKind::Elastic;

  const cls::StatusOr<cls::online::OnlineLinearRegressor> made =
      cls::online::make_online_linear_regressor(options);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "OnlineLinearRegressor supports RegularizationKind::None or RegularizationKind::L2."
```

### Example 3

A negative `learning_rate`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::online::OnlineLinearOptions options{};
  options.learning_rate = -0.1;

  const cls::StatusOr<cls::online::OnlineLinearRegressor> made =
      cls::online::make_online_linear_regressor(options);
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

### Example 4

A negative `regularization_strength`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::online::OnlineLinearOptions options{};
  options.family.regularization = cls::RegularizationKind::L2;
  options.family.regularization_strength = -1.0;

  const cls::StatusOr<cls::online::OnlineLinearRegressor> made =
      cls::online::make_online_linear_regressor(options);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "LinearFamilyOptions.regularization_strength must be finite and >= 0."
```

### Example 5

`robust_transition` is invalid — rejected even though `objective` is
`LeastSquares`, which never reads it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;
  options.family.robust_transition = 0.0;

  const cls::StatusOr<cls::online::OnlineLinearRegressor> made =
      cls::online::make_online_linear_regressor(options);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "LinearFamilyOptions.robust_transition must be finite and > 0."
```

## Changelog

- **0.1** — `make_online_linear_regressor`.

## See Also

- [`OnlineLinearRegressor`](OnlineLinearRegressor.md) — the returned type.
- [`OnlineLinearOptions`](OnlineLinearOptions.md) — every field validated
  here.
- [`make_online_linear_classifier`](make_online_linear_classifier.md) —
  the classification counterpart.
