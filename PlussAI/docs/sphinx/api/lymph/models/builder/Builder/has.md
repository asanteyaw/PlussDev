<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bc672e43e3f5 -->

# `lymph::models::builder::Builder::has`

[Back to `lymph::models::builder::Builder`](../Builder.md)

`has` reports whether a name is registered in the workspace. It is the cheap
guard for [`get`](get.md), which throws on an unknown name.

## Signature

```cpp
bool lymph::models::builder::Builder::has(std::string_view name) const noexcept;
```

## What It Does

`has` scans the registry for `name` and returns `true` or `false`. It never
copies a model and never throws — `noexcept` — which is exactly what makes it the
right thing to call before [`get`](get.md) or [`eject`](eject.md) when the name
came from configuration, a loop, or user input rather than a literal in the code.

For counting rather than presence, [`size`](names.md) and `empty()` are the
matching cheap accessors; for the full list of names, use [`names`](names.md).

## Parameters

`std::string_view name`
: The name to look for. Any string; an absent name simply returns `false`.

## Returns

`true` if a model is registered under `name`, `false` otherwise.

## Errors

None. `has` is `noexcept`.

## Practical Examples

### Example 1

Presence before and after an `eject`.

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

  // has is a cheap name lookup: no copy, no throw when the name is absent.
  std::cout << "has logreg  = " << std::boolalpha << builder.has("logreg") << "\n";
  std::cout << "has tree    = " << builder.has("tree") << "\n";
  std::cout << "has forest  = " << builder.has("forest") << "\n";

  (void)builder.eject("tree");
  std::cout << "has tree after eject = " << builder.has("tree") << "\n";

  return 0;
}
```

Output

```text
has logreg  = true
has tree    = true
has forest  = false
has tree after eject = false
```

### Example 2

Guarding `get` with `has` when the names come from a list.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  // get throws on an unknown name, so guard it with has when the name comes
  // from outside the code.
  const std::vector<std::string> wanted = {"logreg", "tree", "knn"};
  for (const std::string& name : wanted) {
    if (builder.has(name)) {
      std::cout << name << ": present, task_kind = "
                << static_cast<int>(builder.get(name).task_kind()) << "\n";
    } else {
      std::cout << name << ": not registered\n";
    }
  }

  return 0;
}
```

Output

```text
logreg: present, task_kind = 0
tree: not registered
knn: present, task_kind = 0
```

### Example 3

On an empty registry, `has` is always `false` and `empty()` is `true`. Uses the
fully explicit spelling.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  // A fresh registry holds nothing, so has is always false and empty is true.
  lymph::models::builder::Builder builder = lymph::models::builder::build();
  std::cout << "empty        = " << std::boolalpha << builder.empty() << "\n";
  std::cout << "has anything = " << builder.has("logreg") << "\n";

  builder.inject("logreg", lymph::models::logistic_regression());
  std::cout << "after inject: empty = " << builder.empty()
            << ", has logreg = " << builder.has("logreg") << "\n";

  return 0;
}
```

Output

```text
empty        = true
has anything = false
after inject: empty = false, has logreg = true
```

## See Also

- [get](get.md) — the call `has` guards.
- [names](names.md) — the full list, plus `size()` / `empty()`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="inject.html">
    <span class="label">Next</span>
    <span class="title">inject</span>
  </a>
</div>
<!-- apiref:nav:end -->
