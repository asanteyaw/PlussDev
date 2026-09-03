<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 98f3f4a49839 -->

# `lymph::models::builder::Builder::eject`

[Back to `lymph::models::builder::Builder`](../Builder.md)

`eject` removes one model from the workspace by name and hands it back to the
caller.

## Signature

```cpp
lymph::models::Model lymph::models::builder::Builder::eject(std::string_view name);
```

## What It Does

`eject` finds the entry registered under `name`, removes it from the registry,
and returns the `Model` by value. The `Builder` no longer holds it —
[`size`](names.md) drops by one and [`has`](has.md) returns `false` for that
name.

The returned `Model` is a full, independent value you can keep working with:
train it, save it, or hand it elsewhere. `eject` is the first half of a
deliberate replacement — `eject` the stale entry, then [`inject`](inject.md) the
new one under the same name (Example 2), which is the only supported way to
overwrite a name since `inject` refuses duplicates.

## Parameters

`std::string_view name`
: The registered name to remove. Must be present.

## Returns

The `Model` that was registered under `name`, moved out of the registry.

## Errors

- A `name` that is not registered throws (`"no model registered under ..."`);
  the registry is left unchanged.

## Practical Examples

### Example 1

Eject one entry from a three-model registry and inspect what came back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  // eject removes the entry and hands the Model back to you. The Builder no
  // longer holds it.
  const lymph::models::Model taken = builder.eject("tree");

  std::cout << "size after eject   = " << builder.size() << "\n";
  std::cout << "has tree           = " << std::boolalpha << builder.has("tree") << "\n";
  std::cout << "ejected task_kind  = " << static_cast<int>(taken.task_kind()) << "\n";
  std::cout << "ejected trained    = " << taken.trained() << "\n";

  return 0;
}
```

Output

```text
size after eject   = 2
has tree           = false
ejected task_kind  = 0
ejected trained    = false
```

### Example 2

The replacement pattern: eject, do work on the returned model, inject it back
under the same name.

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
  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.4, 3.8, 4.2, 4.6}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  lymph::models::builder::Builder builder = lymph::models::builder::build({
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // eject returns the Model as a value you can keep working with -- here,
  // train it and put a fitted version back under the same name.
  lymph::models::Model tree = builder.eject("tree");
  tree.learn_(x, y);
  builder.inject("tree", tree);

  std::cout << "size            = " << builder.size() << "\n";
  std::cout << "re-injected trained = " << std::boolalpha
            << builder.get("tree").trained() << "\n";

  return 0;
}
```

Output

```text
size            = 1
re-injected trained = true
```

### Example 3

Ejecting a name that was never registered throws, and leaves the registry
untouched.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
  });

  // Ejecting a name that was never registered is an error.
  try {
    const lymph::models::Model missing = builder.eject("forest");
    std::cout << "unexpectedly ejected a model\n";
  } catch (const std::exception& err) {
    std::cout << "eject(\"forest\") threw: " << err.what() << "\n";
  }

  std::cout << "size unchanged = " << builder.size() << "\n";

  return 0;
}
```

Output

```text
eject("forest") threw: lymph::Builder::eject: no model registered under "forest".
size unchanged = 1
```

## See Also

- [inject](inject.md) — the other half of a deliberate replacement.
- [has](has.md) — check presence before ejecting a name from outside the code.
- [get](get.md) — read a stored model without removing it.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="best.html">
    <span class="label">Previous</span>
    <span class="title">best</span>
  </a>
  <a class="next-link" href="get.html">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>
<!-- apiref:nav:end -->
