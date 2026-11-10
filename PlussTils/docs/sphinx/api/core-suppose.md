# `pluss::tils::suppose`

[Back to Core Utilities](core.md) · [Value pipelines](core-flow.md)

`suppose` starts a named, checked argument about a Pluss value. It evaluates a premise, checks each named derivation through `then`, and checks a final claim through `conclude`. The result records which steps passed, failed, or were skipped.

This checks one concrete value at runtime. It does not prove a statement for every possible value.

## Signature

```cpp
#include <pluss/tils/suppose.h>

auto argument = pluss::tils::suppose("premise", pluss_value, predicate);
auto result = argument
    .then("derivation", predicate)
    .conclude("conclusion", predicate);
```

The value must be a supported Pluss type. The first release supports the core Series, Note, Timeline, Duration, and Queue types and the public PlussMic table, kindle, scribble, and bundle types. Native C++ values are deliberately outside this API. Each predicate is a callable taking `const T&` and returning a Boolean result; it receives the same Pluss value throughout the argument.

## What `then` means

`then("name", check)` checks a proposition about the Pluss value. It does not transform the value or visit the elements of a container. If the premise or an earlier derivation fails, the callable is skipped and the named step is recorded as skipped. `conclude("name", check)` performs the final check and returns `SuppositionResult`.

`result.verified()` is true only when the premise, every derivation, and the conclusion pass. `result.applicable()` is false when the premise is not met; that is distinct from a failed derivation or conclusion. `result.status()` distinguishes all four outcomes, `result.steps()` supplies the ordered record, and `result.failed_step()` names the first failed check.

## Ownership and cost

An lvalue Pluss object is observed by const reference, without copying it. An rvalue is moved into the argument so it remains alive through the checks. Callbacks are templated and invoked directly, without `std::function`. The step record owns its names and uses a vector, so creating an argument has an allocation cost. Four steps are reserved up front, and checks after a failure are not called. Use ordinary predicates in a hot loop when no named record is needed.

## Errors

Empty premise, derivation, or conclusion names throw `std::invalid_argument`. Exceptions from a predicate propagate to the caller. An unsupported value type fails at compile time.

## Example 1: a checked argument

```cpp
#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>
#include <iostream>

int main() {
  auto values = pluss::make_series({2.0, 4.0, 6.0});
  auto result = pluss::tils::suppose("three observations", values,
      [](const pluss::Series& x) { return x.numel() == 3; })
      .then("sum is positive", [](const pluss::Series& x) {
        return x.sum().item<double>() > 0.0;
      })
      .conclude("mean is positive", [](const pluss::Series& x) {
        return x.mean().item<double>() > 0.0;
      });
  std::cout << std::boolalpha << result.verified() << '\n';
}
```

Output

```text
true
```

## Example 2: a premise that does not apply

```cpp
#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>
#include <iostream>

int main() {
  auto values = pluss::make_series({2.0, 4.0, 6.0});
  int checks = 0;
  auto result = pluss::tils::suppose("four observations", values,
      [](const pluss::Series& x) { return x.numel() == 4; })
      .then("not evaluated", [&](const pluss::Series&) { ++checks; return true; })
      .conclude("also not evaluated", [&](const pluss::Series&) { ++checks; return true; });
  std::cout << std::boolalpha << result.applicable() << ' ' << checks << '\n';
}
```

Output

```text
false 0
```

## Example 3: locating a failed derivation

```cpp
#include <pluss/pluss.h>
#include <pluss/tils/suppose.h>
#include <iostream>

int main() {
  auto values = pluss::make_series({2.0, 4.0, 6.0});
  auto result = pluss::tils::suppose("three observations", values,
      [](const pluss::Series& x) { return x.numel() == 3; })
      .then("sum is negative", [](const pluss::Series& x) {
        return x.sum().item<double>() < 0.0;
      })
      .conclude("mean is negative", [](const pluss::Series& x) {
        return x.mean().item<double>() < 0.0;
      });
  std::cout << result.failed_step() << '\n';
}
```

Output

```text
sum is negative
```
