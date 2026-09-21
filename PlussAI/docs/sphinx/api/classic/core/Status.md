# `pluss::ai::classic::Status`

`Status` is the result every fallible `classic` call that produces no value
returns — `Flow::learn`, `Flow::set_stage_route`, `SimpleImputer::learn`, and
every other verb that mutates state or does work without handing back a new
object. It carries exactly two things: whether the call succeeded, and, if
not, why.

`Status` never throws on its own and holds no exception. Reading a `Status`
you did not check first is always safe — `.code` and `.message` are plain
data — but *trusting* the call's side effects without checking `.ok()` first
is not: a non-ok `Status` means the operation did not do what its name says,
even though calling `.code` or `.message` on it will not crash.

## Signature

```cpp
bool classic::Status::ok() const noexcept;

static classic::Status classic::Status::success();
static classic::Status classic::Status::invalid_argument(std::string message);
static classic::Status classic::Status::precondition(std::string message);
static classic::Status classic::Status::shape_mismatch(std::string message);
static classic::Status classic::Status::not_implemented(std::string message);
static classic::Status classic::Status::numerical_failure(std::string message);
static classic::Status classic::Status::internal(std::string message);
```

## Construction

You almost never build a `Status` yourself except inside a callable you hand
back to `classic` (a custom `TransformerBase`, a tuning sampler). Ordinary code
*receives* one as a return value. When you do need to build one — implementing
your own `EstimatorBase`/`TransformerBase`, or just to signal a precondition
from your own code in the same idiom — use the named factory that matches the
failure, not the aggregate initializer directly; the factories are what every
other page's `Status::xxx(...)` cross-reference points at.

`Status{}` (or the default constructor) is equivalent to `Status::success()` —
`code` defaults to `StatusCode::Ok` and `.ok()` is `true`.

## State

`code` — `StatusCode`
: Which of seven outcomes this is. Exactly one is success; the other six are
  all failure, distinguished by what kind:

  `Ok`
  : Success. The only value `.ok()` accepts.

  `InvalidArgument`
  : A parameter's value is not acceptable on its own terms — wrong range, an
    empty required list.

  `Precondition`
  : The call is not valid in the object's current state — learning an
    already-trained estimator without `warm_start`, for instance.

  `ShapeMismatch`
  : A `Series` or batch dimension does not match what the call needs.

  `NotImplemented`
  : A real gap — the operation is not supported for this type.

  `NumericalFailure`
  : The underlying computation could not produce a finite result — a
    singular matrix, a non-converging solve.

  `Internal`
  : A library-side inconsistency, not something a caller's input can fix.

`message` — `std::string`
: A human-readable explanation, always present on a failure `Status` and
  usually empty on success. Never parsed programmatically — branch on `code`,
  read `message` for a human.

## Methods

`ok()`
: `true` for `StatusCode::Ok`, `false` for every other code. The only check
  you need before deciding whether a call did what it says.

`Status::success()`
: The success value. Equivalent to `Status{}`.

`Status::invalid_argument(message)`
: A parameter's value is not acceptable on its own terms — independent of any
  other state (an empty required list, a ratio outside `[0, 1]`, a negative
  count).

`Status::precondition(message)`
: The call is not valid given the object's current state — not a bad
  parameter value in isolation, but a bad value *right now* (calling `learn`
  on an object that requires it not be trained yet, without `warm_start`).

`Status::shape_mismatch(message)`
: A `Series` or batch dimension does not match what the operation needs — a
  label vector whose length does not match the sample count, a fold count
  larger than the sample count.

`Status::not_implemented(message)`
: The operation is real but genuinely unsupported for this type — the default
  `dump_state()` / `load_state()` on an `EstimatorBase` that has not
  overridden them, for example.

`Status::numerical_failure(message)`
: The computation ran but could not produce a usable result — non-finite
  values, a solve that failed to converge, a singular system.

`Status::internal(message)`
: A library-side inconsistency reached that should not be reachable from any
  valid call. Report it; it is not something caller input can work around.

## Practical Example

Every factory, and the two things you actually do with a `Status`: check
`ok()`, and — on failure — read `code` and `message`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::Status ok = cls::Status::success();
  const cls::Status bad_arg = cls::Status::invalid_argument("feature_names must not be empty");
  const cls::Status precond = cls::Status::precondition("learn() requires an untrained estimator");
  const cls::Status shape = cls::Status::shape_mismatch("y.numel() must equal x.sizes().front()");
  const cls::Status unimpl = cls::Status::not_implemented("dump_state() is not implemented for this estimator");
  const cls::Status numeric = cls::Status::numerical_failure("Cholesky factorization encountered a non-finite pivot");
  const cls::Status internal = cls::Status::internal("unreachable branch in family dispatch");

  const cls::Status* all[] = {&ok, &bad_arg, &precond, &shape, &unimpl, &numeric, &internal};
  for (const cls::Status* s : all) {
    std::cout << "code = " << static_cast<int>(s->code) << ", ok = " << std::boolalpha << s->ok()
              << ", message = \"" << s->message << "\"\n";
  }

  // Status{} default-constructs to the same thing success() returns.
  const cls::Status defaulted{};
  std::cout << "default-constructed ok = " << defaulted.ok() << "\n";

  return 0;
}
```

Output

```text
code = 0, ok = true, message = ""
code = 1, ok = false, message = "feature_names must not be empty"
code = 2, ok = false, message = "learn() requires an untrained estimator"
code = 3, ok = false, message = "y.numel() must equal x.sizes().front()"
code = 4, ok = false, message = "dump_state() is not implemented for this estimator"
code = 5, ok = false, message = "Cholesky factorization encountered a non-finite pivot"
code = 6, ok = false, message = "unreachable branch in family dispatch"
default-constructed ok = true
```

A `Status` as it actually arrives — the return value of a real call, checked
with `ok()` before anything else, is on [`StatusOr`](StatusOr.md) (the same
pattern, plus a value), since almost every fallible `classic` call that
*produces* something returns `StatusOr<T>` rather than a bare `Status`. See
that page's Example 1 for the receiving side of this exact idiom.

## Changelog

- **0.1** — the seven-`StatusCode` result type and its named factories.

## See Also

- [`StatusOr<T>`](StatusOr.md) — the same convention, carrying a value.
- [`estimators`](../estimators.md) — every method there returns `Status` or
  `StatusOr<T>`.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
