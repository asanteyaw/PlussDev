# `lymph::Policy`

## Summary

`lymph::Policy` describes data-admission and schema-governance rules around a
workflow.

Use it when you need explicit control over:

- raw-frame admission
- schema expectations
- runtime acceptance or rejection behavior
- row admissibility before the model sees the data

## Why Policy Deserves Its Own Object

Many codebases bury these rules inside ad hoc pre-fit and pre-infer code:

- drop this if present
- throw if these are missing
- filter nulls here but not there

That looks harmless until training and inference start drifting apart.

By making policy explicit, Lymph gives the project:

- one place for admission rules
- more reproducible train/infer behavior
- easier persistence of runtime expectations
- fewer silent schema mismatches

## Core Idea

`Policy` is governance, not feature engineering.

It does not mean:

- scale this column
- encode that category
- impute this missing value

It means:

- these columns must exist
- these columns must not exist
- these columns may be dropped
- rows are inadmissible if required values are missing

That separation is what keeps policy logic from getting confused with preprocessing
logic.

## Signature

```cpp
lymph::Policy& lymph::Policy::drop(std::initializer_list<std::string> names);
lymph::Policy& lymph::Policy::require(std::initializer_list<std::string> names);
lymph::Policy& lymph::Policy::forbid(std::initializer_list<std::string> names);
pluss::table::DataFrame lymph::Policy::apply(
    const pluss::table::DataFrame& frame) const;
```

## Practical Example

This page uses the exact verifier program `lymph_policy_docverify_demo.cpp`.

### Example 1

This example defines a simple policy, applies it to a raw frame, and prints the
admitted row and feature counts after governance is enforced.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"customer_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106, 107, 108}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"observed_at", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-06T00:00:00Z",
          "2026-01-07T00:00:00Z",
          "2026-01-08T00:00:00Z",
      }))},
  });

  lymph::Policy policy{};
  policy.drop({"customer_id"});
  policy.require({"income", "balance", "late", "region", "observed_at"});

  const DataFrame admitted = policy.apply(raw);
  std::cout << "== policy_apply ==\n";
  std::cout << "raw_rows=" << raw.ncases()
            << " admitted_rows=" << admitted.ncases() << "\n";
  std::cout << "admitted_features=" << admitted.nfeatures() << "\n";

  return 0;
}
```

Output

```text
== policy_apply ==
raw_rows=8 admitted_rows=8
admitted_features=6
```

This small output block teaches the key distinction:

- the row count is preserved because no rows violate the admission rule
- the feature count drops because the explicit ID column is removed before the
  workflow proceeds

That is exactly the difference between governance and preprocessing.

## Related Pages

- [Workflow](Workflow.md)
- [Payload](Payload.md)
