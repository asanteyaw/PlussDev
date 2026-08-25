# `lymph::Builder`

## Summary

`lymph::Builder` is the named candidate-model coordinator in Lymph.

Use it when you want a disciplined workspace of candidate models before
spotchecking, benchmarking, or search.

`Builder` is where a messy pile of "maybe try this one too" becomes a named,
auditable shortlist.

## What It Does

`Builder` is not itself the learner. It is the organizing surface around several
learners.

Its job is to answer questions such as:

- which candidate models are currently under consideration?
- what stable names identify them?
- how do I benchmark or spotcheck them fairly under one validation contract?
- after comparison, which model is best and which is worst?

This is the object that turns model selection from ad hoc scripting into a real
workspace.

## Why It Matters

You could technically keep candidate models in a plain container and write a loop.

That is exactly the sort of code that becomes fragile quickly:

- names drift away from models
- overwrite behavior becomes unclear
- reporting becomes manual
- benchmark outcomes are harder to preserve and inspect

`Builder` fixes that by giving the candidate set a real contract:

- entries are named
- duplicate names are rejected
- cheap-copy `Model` handles remain easy to move around
- reporting is a first-class surface

## Signature

```cpp
lymph::Builder lymph::builder::build(...);
pluss::table::DataFrame lymph::Builder::report() const;
lymph::Model lymph::Builder::best() const;
lymph::Model lymph::Builder::worst() const;
```

## Practical Example

This page uses the exact verifier program `lymph_model_builder_docverify_demo.cpp`.

### Example 1

This example constructs a two-model shortlist and prints the builder board before
any comparison run happens.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classic = pluss::ai::classic;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });
  (void)raw;

  classic::LinearFamilyOptions logistic_options{};
  logistic_options.regularization = classic::RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  lymph::Builder builder = lymph::builder::build({
      {"logreg", lymph::model::logistic_regression(logistic_options)},
      {"tree", lymph::model::decision_tree_classifier()},
  });

  std::cout << "== builder_report ==\n";
  std::cout << builder.report() << "\n";

  return 0;
}
```

Output

```text
== builder_report ==
===================================================
     | name    task_kind       trained  stage_count
---------------------------------------------------
0    | logreg  classification  false    0
1    | tree    classification  false    0
---------------------------------------------------
[2 cs x 4 feat]
```

That board is what makes `Builder` more than a model bag. It keeps candidate
identity explicit before any benchmarking or spotchecking happens.

## Related Pages

- [Model](Model.md)
- [Suite](../tune/Suite.md)
- [Search](../tune/Search.md)
