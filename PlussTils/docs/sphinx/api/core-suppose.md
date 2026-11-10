# `suppose`

[Back to Core Utilities](core.md) · [Value pipelines](core-flow.md)

`suppose` checks a named premise about a Pluss value, optionally derives more Pluss values, and checks a conclusion. It runs immediately for **this input**. The result identifies the first failed check and retains completed derivations. It makes no claim about every possible input.

Use `flow(value)` for an unchecked chain of transformations. `Series::traverse` selects matches from a Series; it does not check a named premise or report which step failed.

## Signature

```text
suppose(name, value, predicate)
  .then(name, derive[, verify])
  .check(name, predicate)
  .conclude(name, predicate)
```

| Part | Meaning |
| --- | --- |
| `name` | A nonempty label shown in the result trace. |
| `value` | A supported Pluss value. An lvalue is borrowed through a const reference; an rvalue is moved. |
| Premise `predicate` | A boolean test of the original value. If false, later callbacks are skipped. |
| `derive` | Produces another Pluss value from the original value and all earlier derivations. |
| Optional `verify` | Tests the new derivation against the original value and earlier derivations. |
| `check` | Tests a relation without producing a value. |
| Conclusion `predicate` | The final boolean test. It runs only if earlier checks passed. |

The original and derived values must be supported Pluss types. Native C++ values are outside this API. `then` callbacks return Pluss values; predicates return a value convertible to `bool`.

## Example 1: check a calculation

```cpp
#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>

int main() {
  auto x = pluss::make_series({2.0, 4.0, 6.0});
  auto result = pluss::tils::suppose("three observations", x,
      [](const pluss::Series& s) { return s.numel() == 3; })
      .then("sum is twelve",
          [](const pluss::Series& s) { return s.sum(); },
          [](const pluss::Series&, const pluss::Series& sum) {
            return sum.item<double>() == 12.0;
          })
      .conclude("mean is positive",
          [](const pluss::Series& s, const pluss::Series& sum) {
            return sum.item<double>() / s.numel() > 0.0;
          });

  if (!result.verified()) return 1;
  return result.derived<0>()->item<double>() == 12.0 ? 0 : 1;
}
```

The premise checks that the Series has three observations. The derivation computes its sum and verifies that the result is 12. The conclusion checks that the mean implied by that sum is positive. These are runtime checks on `x`, not a proof for arbitrary Series values.

## Example 2: inspect a failed step

```cpp
#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>
#include <iostream>

int main() {
  auto x = pluss::make_series({2.0, 4.0, 6.0});
  auto result = pluss::tils::suppose("nonempty", x,
      [](const pluss::Series& s) { return s.numel() > 0; })
      .check("all values negative",
          [](const pluss::Series& s) { return s.sum().item<double>() < 0.0; })
      .conclude("sum negative",
          [](const pluss::Series& s) { return s.sum().item<double>() < 0.0; });

  if (!result.verified()) std::cout << result.failed_step() << '\n';
}
```

Here `failed_step()` is `all values negative`. The conclusion callback is skipped. `steps()` lists each step and whether it passed, failed, was computed, or was skipped. `status()` distinguishes an unmet premise, a failed intermediate check, and a failed conclusion. `applicable()` is false only when the premise fails.

## Cost and errors

Callbacks are templates invoked directly, without `std::function`. Derived Pluss values are moved into the result. The trace stores names in a vector and has a small allocation cost. Empty names throw `std::invalid_argument`; callback exceptions propagate. Use `flow` when names and checks are unnecessary.
