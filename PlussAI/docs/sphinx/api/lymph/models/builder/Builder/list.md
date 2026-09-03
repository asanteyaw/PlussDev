<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d0664f44a260 -->

# `lymph::models::builder::Builder::list`

[Back to `lymph::models::builder::Builder`](../Builder.md)

`list` returns the first *n* registered names — a bounded preview of the
workspace.

## Signature

```cpp
std::vector<std::string> lymph::models::builder::Builder::list(
    std::int64_t n = -1) const;
```

## What It Does

`list(n)` returns the first `n` names, in insertion order. It is the plain-list
half of the split between a quick glance and a full readout:

- `list(n)` — the first `n` names, for a fast "what's in here"
- [`names()`](names.md) — every name
- [`report(n)`](report.md) — the first `n` entries as a status table

The default argument `-1` means "all of them", so `list()` and `list(-1)` return
the same thing as `names()`. Asking for more names than the registry holds is not
an error — `list` clamps to the registry size (Example 3).

## Parameters

`std::int64_t n` — default `-1`
: How many names to return, from the front. `-1` (or any value `>= size()`)
  returns them all.

## Returns

A `std::vector<std::string>` of at most `n` names, in insertion order.

## Errors

None. Out-of-range `n` is clamped, not rejected.

## Practical Examples

### Example 1

A three-name preview of a five-model registry.

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
      {"boost", lymph::models::ensemble::gradient_boost_classifier()},
  });

  // list(n) is a quick "what's in here" preview: the first n names, in
  // insertion order.
  const std::vector<std::string> preview = builder.list(3);
  std::cout << "list(3):";
  for (const std::string& name : preview) std::cout << " " << name;
  std::cout << "\n";

  return 0;
}
```

Output

```text
list(3): logreg tree knn
```

### Example 2

`list()`, `list(-1)`, and `names()` all return the whole set.

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

  // The default argument (-1) means "all of them", so list() and list(-1) are
  // the same as names() here.
  const std::vector<std::string> a = builder.list();
  const std::vector<std::string> b = builder.list(-1);
  std::cout << "list()   size = " << a.size() << "\n";
  std::cout << "list(-1) size = " << b.size() << "\n";
  std::cout << "names()  size = " << builder.names().size() << "\n";

  return 0;
}
```

Output

```text
list()   size = 3
list(-1) size = 3
names()  size = 3
```

### Example 3

`n` larger than the registry is clamped, not an error.

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

  // Asking for more names than exist is not an error -- list clamps to the
  // registry size.
  const std::vector<std::string> over = builder.list(10);
  std::cout << "list(10) on a size-2 registry returned " << over.size() << " names:";
  for (const std::string& name : over) std::cout << " " << name;
  std::cout << "\n";

  return 0;
}
```

Output

```text
list(10) on a size-2 registry returned 2 names: logreg tree
```

## See Also

- [names](names.md) — the full list, plus `size()` / `empty()`.
- [report](report.md) — the first *n* entries as a status table.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="inject.html">
    <span class="label">Previous</span>
    <span class="title">inject</span>
  </a>
  <a class="next-link" href="names.html">
    <span class="label">Next</span>
    <span class="title">names</span>
  </a>
</div>
<!-- apiref:nav:end -->
