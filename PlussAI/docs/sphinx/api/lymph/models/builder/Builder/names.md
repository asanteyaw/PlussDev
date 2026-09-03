<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 46adbb6190a4 -->

# `lymph::models::builder::Builder::names`

[Back to `lymph::models::builder::Builder`](../Builder.md)

`names` returns every registered name, in the order the models were injected.

## Signature

```cpp
std::vector<std::string> lymph::models::builder::Builder::names() const;

// The cheap counting siblings:
std::int64_t lymph::models::builder::Builder::size() const noexcept;
bool lymph::models::builder::Builder::empty() const noexcept;
```

## What It Does

`names` builds and returns the full list of keys in the workspace, in **insertion
order** — the order matters because it is the order [`benchmark`](benchmark.md)
and [`report`](report.md) present candidates in, and it is what makes a results
table readable.

When you only need a count, `size()` and `empty()` answer without allocating the
string vector. When you only want a glance at the first few entries,
[`list(n)`](list.md) is the bounded preview. `names` is the one that gives you
everything.

## Parameters

None.

## Returns

A `std::vector<std::string>` containing one entry per registered model, in
insertion order. Empty when the registry is empty.

## Errors

None.

## Practical Examples

### Example 1

Inject four models one at a time; `names` reflects the insertion order exactly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build();
  builder.inject("logreg", lymph::models::logistic_regression());
  builder.inject("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3}));
  builder.inject("knn", lymph::models::neighbor::knn_classifier({.k = 3}));
  builder.inject("forest", lymph::models::ensemble::random_forest_classifier());

  // names returns every registered name in the order it was injected -- the
  // full identity of the registry, not a preview.
  std::cout << "count = " << builder.names().size() << "\n";
  for (const std::string& name : builder.names()) std::cout << "  " << name << "\n";

  return 0;
}
```

Output

```text
count = 4
  logreg
  tree
  knn
  forest
```

### Example 2

`names` (complete) next to `list(2)` (bounded preview).

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
      {"forest", lymph::models::ensemble::random_forest_classifier()},
  });

  // names is the complete list; list(n) is a bounded preview of the first n.
  const std::vector<std::string> all = builder.names();
  const std::vector<std::string> head = builder.list(2);

  std::cout << "names (" << all.size() << "): ";
  for (std::size_t i = 0; i < all.size(); ++i) std::cout << (i ? ", " : "") << all[i];
  std::cout << "\n";

  std::cout << "list(2) (" << head.size() << "): ";
  for (std::size_t i = 0; i < head.size(); ++i) std::cout << (i ? ", " : "") << head[i];
  std::cout << "\n";

  return 0;
}
```

Output

```text
names (4): logreg, tree, knn, forest
list(2) (2): logreg, tree
```

### Example 3

`size()` and `empty()` track the registry without building the name vector. Uses
the fully explicit spelling.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::builder::Builder builder = lymph::models::builder::build();
  std::cout << "empty registry: size = " << builder.size()
            << ", empty = " << std::boolalpha << builder.empty()
            << ", names = " << builder.names().size() << "\n";

  builder.inject("logreg", lymph::models::logistic_regression());
  builder.inject("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3}));

  // size() and empty() are the cheap counting siblings of names(): a count
  // and a yes/no, without building the string vector.
  std::cout << "after two injects: size = " << builder.size()
            << ", empty = " << builder.empty()
            << ", names = " << builder.names().size() << "\n";

  return 0;
}
```

Output

```text
empty registry: size = 0, empty = true, names = 0
after two injects: size = 2, empty = false, names = 2
```

## See Also

- [list](list.md) — the bounded preview.
- [report](report.md) — names plus per-model status, as a DataFrame.
- [has](has.md) — presence of a single name.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="list.html">
    <span class="label">Previous</span>
    <span class="title">list</span>
  </a>
  <a class="next-link" href="report.html">
    <span class="label">Next</span>
    <span class="title">report</span>
  </a>
</div>
<!-- apiref:nav:end -->
