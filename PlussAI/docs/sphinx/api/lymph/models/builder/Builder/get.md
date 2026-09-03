<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c018d75497ad -->

# `lymph::models::builder::Builder::get`

[Back to `lymph::models::builder::Builder`](../Builder.md)

`get` returns a reference to a stored model without removing it from the
workspace.

## Signature

```cpp
const lymph::models::Model& lymph::models::builder::Builder::get(
    std::string_view name) const;

lymph::models::Model& lymph::models::builder::Builder::get(std::string_view name);
```

## What It Does

`get` looks up `name` and returns a reference to the `Model` the registry holds
under it. Nothing is removed — after `get` the registry still has every entry,
unlike [`eject`](eject.md).

The `const` overload gives a read-only reference for inspection: check
`task_kind()`, `trained()`, `stage_count()`. The non-`const` overload gives a
mutable reference, so you can train or reconfigure an entry **in place** without
the eject/inject round trip (Example 2).

`get` throws on an unknown name. When the name is not a literal in your code,
call [`has`](has.md) first.

Note that [`best`](best.md) and [`worst`](worst.md) do **not** go through `get`:
they return fitted clones from the last benchmark, while `get` always returns the
originally registered entry.

## Parameters

`std::string_view name`
: The registered name to look up. Must be present.

## Returns

A reference to the stored `Model` — `const` or mutable depending on the overload.
The reference is valid until the entry is ejected or the `Builder` is destroyed.

## Errors

- A `name` that is not registered throws (`"no model registered under ..."`).

## Practical Examples

### Example 1

Read-only inspection. The registry is unchanged afterward.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
  });

  // get returns a reference to the stored model without removing it. The
  // registry still holds every entry afterward.
  const lymph::models::Model& tree = builder.get("tree");
  std::cout << "tree task_kind   = " << static_cast<int>(tree.task_kind()) << "\n";
  std::cout << "tree trained     = " << std::boolalpha << tree.trained() << "\n";
  std::cout << "tree stage_count = " << tree.stage_count() << "\n\n";

  std::cout << "registry still holds: size = " << builder.size()
            << ", has tree = " << builder.has("tree") << "\n";

  return 0;
}
```

Output

```text
tree task_kind   = 0
tree trained     = false
tree stage_count = 0

registry still holds: size = 2, has tree = true
```

### Example 2

The non-`const` overload trains entries in place.

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

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2})},
  });

  // The non-const get returns a mutable reference, so you can train an entry
  // in place without ejecting and re-injecting it.
  builder.get("logreg").learn_(x, y);
  builder.get("tree").learn_(x, y);

  std::cout << "logreg trained in place = " << std::boolalpha
            << builder.get("logreg").trained() << "\n";
  std::cout << "tree trained in place   = " << builder.get("tree").trained() << "\n";

  return 0;
}
```

Output

```text
logreg trained in place = true
tree trained in place   = true
```

### Example 3

`get` on an unknown name throws.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
  });

  // get on an unknown name throws rather than returning a default.
  try {
    const lymph::models::Model& missing = builder.get("tree");
    std::cout << "unexpectedly got a model\n";
  } catch (const std::exception& err) {
    std::cout << "get(\"tree\") threw: " << err.what() << "\n";
  }

  return 0;
}
```

Output

```text
get("tree") threw: lymph::Builder::get: no model registered under "tree".
```

## See Also

- [has](has.md) — the guard for `get`.
- [eject](eject.md) — take an entry out instead of borrowing it.
- [best](best.md), [worst](worst.md) — return benchmark winners, not registry
  entries.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eject.html">
    <span class="label">Previous</span>
    <span class="title">eject</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
<!-- apiref:nav:end -->
