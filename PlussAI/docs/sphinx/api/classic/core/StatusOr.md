# `pluss::ai::classic::StatusOr`

`StatusOr<T>` is what almost every `classic` call that produces something
returns instead of a bare `T`: a [`Status`](Status.md) plus a `T` that is only
meaningful when that status is ok. `make_classifier` returns
`StatusOr<std::unique_ptr<ClassifierBase>>`, `crossval_plan` returns
`StatusOr<SplitPlan>`, `Flow::infer` returns `StatusOr<Series>` — the pattern
is the same everywhere: check `.ok()`, then read `.value()`.

`T` must be default-constructible — `StatusOr<T>` always holds a real `T`
object, constructed either from the value you (or the call) supplied, or
default-constructed when there is no value to supply. This is the detail that
makes `StatusOr` sharper than it looks: **`.value()` never checks anything.**
It is not a `std::optional`-style accessor that throws or asserts on an empty
state — it unconditionally returns whatever `T` the object is holding, which on
a failure is a *default-constructed* `T` (`0` for `int`, `pluss::series::undefined()`
for `Series`, an empty vector, …), not a sentinel that flags itself as invalid.
Reading `.value()` without checking `.ok()` first compiles, runs, and hands you
a silently wrong answer instead of a crash. The only correct pattern is:

```cpp
cls::StatusOr<T> result = some_call(...);
if (!result.ok()) { /* handle result.status() */ }
else { /* use result.value() */ }
```

## Signature

```cpp
// classic::StatusOr<T>, for any value type T:
classic::StatusOr<T>::StatusOr(classic::Status status, T value);
explicit classic::StatusOr<T>::StatusOr(T value);
explicit classic::StatusOr<T>::StatusOr(classic::Status status);

bool classic::StatusOr<T>::ok() const noexcept;
const classic::Status& classic::StatusOr<T>::status() const noexcept;
const T& classic::StatusOr<T>::value() const noexcept;
T& classic::StatusOr<T>::value() noexcept;
T&& classic::StatusOr<T>::move_value() noexcept;
```

## Construction

You receive a `StatusOr<T>` from a `classic` call far more often than you
build one. The three constructors exist mainly so *your own* callables handed
to `classic` (a custom sampler, a hand-rolled `EstimatorBase`) can return one
in the same idiom:

`StatusOr(T value)` (implicit `explicit`, so `T` must convert explicitly)
: Success. `status()` becomes `Status::success()`, `ok()` is `true`.

`StatusOr(Status status)`
: Failure — or success with no value to report, though that combination is
  unusual since a genuinely-ok `Status` here still leaves `value()` returning
  a default-constructed `T`. `ok()` reports whatever `status.ok()` says.

`StatusOr(Status status, T value)`
: Both at once — a status and an explicit value. `ok()` is `status.ok()`, but
  `value` is stored regardless of whether `status` is ok. This overload exists
  for callers that want to hand back partial or diagnostic data alongside a
  failure status; `.value()` will still return exactly what you passed, even
  when `.ok()` is `false`. Prefer `StatusOr(Status)` unless you specifically
  want that.

## Methods

`ok()`
: `true` only when the object was constructed with (or defaulted to) an ok
  `Status`. This is the one call to make before touching `.value()`.

`status()`
: The full `Status` — `.code` and `.message` — regardless of `ok()`. Always
  safe to read.

`value()` (const and mutable overloads)
: The held `T`, unconditionally. See the warning above: this does not check
  `ok()`. On a failure object it returns whatever `T` was supplied to the
  constructor (or a default-constructed `T` if none was), not an error
  indicator.

`move_value()`
: `std::move`s the held `T` out. Use this on an rvalue or a `StatusOr` you own
  and are discarding, to avoid a copy of an expensive `T` (a `Series`, a
  `std::unique_ptr`, a `Flow`). Still does not check `ok()`.

## Practical Examples

### Example 1

`StatusOr<T>` as it actually arrives: the return value of a real `classic`
call, checked with `ok()` before its value is used for anything.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 10;  // more folds than samples below -- must fail

  const cls::StatusOr<cls::SplitPlan> bad = cls::crossval_plan(4, split);
  std::cout << "bad.ok()      = " << std::boolalpha << bad.ok() << "\n";
  std::cout << "bad.status()  = code " << static_cast<int>(bad.status().code) << ", \""
            << bad.status().message << "\"\n";

  split.folds = 4;
  const cls::StatusOr<cls::SplitPlan> good = cls::crossval_plan(4, split);
  std::cout << "good.ok()     = " << good.ok() << "\n";
  if (good.ok()) {
    std::cout << "good fold count = " << good.value().folds.size() << "\n";
  }

  return 0;
}
```

Output

```text
bad.ok()      = false
bad.status()  = code 1, "SplitOptions.folds must be <= sample_count."
good.ok()     = true
good fold count = 4
```

### Example 2

The three constructor forms built directly, including the sharp edge: a
failure `StatusOr` still returns a real `T` from `value()` — silently, with no
signal that anything went wrong other than `ok()`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  // 1) explicit(T) -- success, status defaults to Status::success().
  const cls::StatusOr<int> from_value(42);
  std::cout << "from_value.ok() = " << std::boolalpha << from_value.ok()
            << ", value = " << from_value.value() << "\n";

  // 2) explicit(Status) -- failure, T default-constructs (int -> 0).
  const cls::StatusOr<int> from_status(cls::Status::invalid_argument("no candidates"));
  std::cout << "from_status.ok() = " << from_status.ok()
            << ", value() returns = " << from_status.value() << "  <- default T, not a crash\n";

  // 3) (Status, T) -- has_value() tracks status.ok(), but the T is stored either way.
  const cls::StatusOr<int> mismatched(cls::Status::invalid_argument("ignored on purpose"), 7);
  std::cout << "mismatched.ok() = " << mismatched.ok()
            << ", value() returns = " << mismatched.value() << "  <- still 7, ok() lied to you if unchecked\n";

  const cls::StatusOr<int> matched(cls::Status::success(), 9);
  std::cout << "matched.ok() = " << matched.ok() << ", value() = " << matched.value() << "\n";

  // The only safe pattern: branch on ok() before reading value().
  if (!from_status.ok()) {
    std::cout << "correctly handled failure: " << from_status.status().message << "\n";
  }

  return 0;
}
```

Output

```text
from_value.ok() = true, value = 42
from_status.ok() = false, value() returns = 0  <- default T, not a crash
mismatched.ok() = false, value() returns = 7  <- still 7, ok() lied to you if unchecked
matched.ok() = true, value() = 9
correctly handled failure: no candidates
```

### Example 3

`move_value()` on an owning move-only `T` — `Flow` itself is move-only
(non-copyable), so `StatusOr<Flow>` from `make_flow` is exactly the shape
where `move_value()` matters instead of `value()`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ModelSpec spec{};
  spec.task = cls::TaskKind::Regression;
  spec.family = cls::ModelFamily::Linear;
  spec.options = cls::LinearFamilyOptions{};

  cls::StatusOr<cls::Flow> made = cls::make_flow(spec);
  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";

  cls::Flow flow = made.move_value();  // moves the Flow out; made.value() would copy-construct and fail to compile
  std::cout << "flow.has_model_spec() = " << flow.has_model_spec() << "\n";
  std::cout << "flow.trained()        = " << flow.trained() << "\n";

  return 0;
}
```

Output

```text
made.ok() = true
flow.has_model_spec() = true
flow.trained()        = false
```

## Changelog

- **0.1** — the `Status` + `T` result type; three constructors, `ok()` /
  `status()` / `value()` / `move_value()`.

## See Also

- [`Status`](Status.md) — the status half, and the full `StatusCode` taxonomy.
- [`compose`](../compose.md) — `make_flow` returns `StatusOr<Flow>`, the
  move-only case Example 3 covers.
- [`estimators`](../estimators.md) — `make_classifier` / `make_regressor` /
  `make_transformer` all return a `StatusOr<std::unique_ptr<...>>`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
