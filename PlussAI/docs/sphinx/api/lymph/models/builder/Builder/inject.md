<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a28ad35a2a92 -->

# `lymph::models::builder::Builder::inject`

[Back to `lymph::models::builder::Builder`](../Builder.md)

`inject` registers one or more candidate models in the workspace under stable
names. It returns the `Builder`, so calls chain.

## Signature

```cpp
// One entry.
lymph::models::builder::Builder& lymph::models::builder::Builder::inject(
    std::string name, lymph::models::Model model);

// A batch, validated as a whole, then registered in list order.
lymph::models::builder::Builder& lymph::models::builder::Builder::inject(
    std::initializer_list<std::pair<std::string, lymph::models::Model>> entries);

// A batch from a preassembled name -> Model map, registered in the map's
// own iteration order.
lymph::models::builder::Builder& lymph::models::builder::Builder::inject(
    const std::map<std::string, lymph::models::Model>& entries);

lymph::models::builder::Builder& lymph::models::builder::Builder::inject(
    const std::unordered_map<std::string, lymph::models::Model>& entries);
```

## What It Does

The single-entry form takes a `name` and a `Model` and appends the pair to the
registry. A freshly built model — `lymph::models::logistic_regression()` and its
siblings — is a prvalue and needs no `std::move`; an already-named `Model` lvalue
is copied, and `Model`'s cheap intrusive handle means that copy is nearly free
until one side is mutated.

The braced-list form registers several entries at once. It validates the whole
batch first — rejecting an empty name, a name already in the registry, or a name
that appears twice in the list — so a bad entry near the end cannot leave earlier
entries half-registered.

The `std::map` and `std::unordered_map` forms are for a registry that is already
sitting in a container — assembled by a loop, a lookup table, or a config pass —
rather than written out as a literal. A map's keys are unique by construction, so
the intra-batch duplicate scan is skipped; the empty-name and already-registered
checks still run against every entry before any entry is added, so these forms
give the same all-or-nothing guarantee as the braced list.

### Which bulk form to reach for

| Input | Registration order | Duplicate keys in the input |
| --- | --- | --- |
| `std::initializer_list` (braced list) | list order, left to right | possible — a repeated name **throws** |
| `std::map` | sorted key order | impossible — the container dedupes |
| `std::unordered_map` | unspecified — sort `names()` for a stable view | impossible — the container dedupes |

Reach for the braced list when the set of candidates is known at the call site
(Examples 1–2). Reach for a `std::map` when the candidates are built up
programmatically and you want a deterministic, alphabetical registration order
(Example 4). Reach for a `std::unordered_map` when the assembly is keyed lookups
and order was never meaningful (Example 5). All four forms return the `Builder`.

Names are the identity of the workspace. They flow into
[`report`](report.md), into the [`benchmark`](benchmark.md) board, and into the
experimental record. Choose names you will still recognise in a results table:
`logreg`, `tree`, `gbm`.

### Duplicate names are an error

Re-injecting a name that is already registered **throws**. It is not a silent
overwrite. A repeated name is almost always a typo colliding with an existing
entry, and surfacing it immediately is safer than quietly replacing a candidate.
To deliberately swap an entry, [`eject`](eject.md) the old one first, then
`inject` the replacement (Example 3).

## Parameters

`std::string name`
: The key to register under. Must be non-empty and not already present.

`lymph::models::Model model`
: The candidate. Usually a factory prvalue; may be an untrained template or an
  already-fitted model.

`std::initializer_list<std::pair<std::string, Model>> entries`
: A braced list of `{name, model}` pairs for the batch form. Validated as a
  whole before anything is registered.

`const std::map<std::string, Model>& entries` / `const std::unordered_map<std::string, Model>& entries`
: A preassembled registry. Each `Model` is copied in — cheap until one side
  mutates. Registered in the container's iteration order (sorted for `std::map`,
  unspecified for `std::unordered_map`). Validated as a whole before anything is
  registered.

## Returns

A reference to the `Builder`, so `inject` calls chain and can be followed
directly by [`benchmark`](benchmark.md) or [`report`](report.md).

## Errors

- An empty `name` throws.
- A `name` already in the registry throws (`"... is already registered; eject it
  first to replace it."`).
- In the braced-list form, a name repeated within the list throws, and nothing
  in the batch is registered.
- In the `std::map` / `std::unordered_map` forms a repeated key cannot occur, but
  a key that collides with an already-registered name still throws, and nothing
  in the batch is registered (Example 4).

## Practical Examples

### Example 1

Chained single injects. Each `inject` returns the builder, so three calls read as
one statement. Uses the `lymph::models::build` family alias.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  // build() with no arguments makes an empty registry. `lymph::models::build`
  // is the main-family alias; `lymph::models::builder::build` is the same
  // function at its full scope.
  lymph::models::Builder builder = lymph::models::build();

  // inject registers one model under a name and returns the builder, so calls
  // chain. A freshly built model needs no std::move.
  builder.inject("logreg", lymph::models::logistic_regression())
         .inject("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3}))
         .inject("knn", lymph::models::neighbor::knn_classifier({.k = 3}));

  std::cout << "size    = " << builder.size() << "\n";
  std::cout << "has knn = " << std::boolalpha << builder.has("knn") << "\n";
  const std::vector<std::string> names = builder.names();
  std::cout << "names   = " << names[0] << ", " << names[1] << ", " << names[2] << "\n";

  return 0;
}
```

Output

```text
size    = 3
has knn = true
names   = logreg, tree, knn
```

### Example 2

The braced-list form, once at construction and once after. `build({...})` is the
construction-time shortcut for a batch `inject`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  // The batch form takes a braced list of {name, model} pairs and validates
  // the whole set before touching the registry, so a bad entry cannot leave a
  // half-populated Builder behind. `build({...})` is a construction-time
  // shortcut for exactly this.
  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression()},
      {"tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::neighbor::knn_classifier({.k = 3})},
  });

  // inject can also take a braced list to add several at once after construction.
  builder.inject({
      {"forest", lymph::models::ensemble::random_forest_classifier()},
      {"boost", lymph::models::ensemble::gradient_boost_classifier()},
  });

  std::cout << "size = " << builder.size() << "\n";
  for (const std::string& name : builder.names()) std::cout << "  - " << name << "\n";

  return 0;
}
```

Output

```text
size = 5
  - logreg
  - tree
  - knn
  - forest
  - boost
```

### Example 3

A duplicate name throws; the deliberate replacement is `eject` then `inject`.
Uses the fully explicit `lymph::models::builder::` spelling.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  // Full-scope spelling: the implementation lives in lymph::models::builder,
  // and this works identically to the lymph::models::build() alias.
  lymph::models::builder::Builder builder = lymph::models::builder::build();
  builder.inject("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3}));

  // Re-injecting a name that already exists is a mistake, not a silent
  // overwrite -- inject throws so a typo colliding with an existing entry is
  // caught at once.
  try {
    builder.inject("tree", lymph::models::logistic_regression());
  } catch (const std::exception& err) {
    std::cout << "second inject(\"tree\") threw: " << err.what() << "\n";
  }

  // The deliberate way to replace an entry: eject the old one, then inject.
  const lymph::models::Model removed = builder.eject("tree");
  builder.inject("tree", lymph::models::logistic_regression());
  std::cout << "after replace: size = " << builder.size()
            << ", has tree = " << std::boolalpha << builder.has("tree") << "\n";

  return 0;
}
```

Output

```text
second inject("tree") threw: lymph::Builder::inject: "tree" is already registered; eject it first to replace it.
after replace: size = 1, has tree = true
```

### Example 4

A preassembled `std::map` handed to `inject` in one call. `std::map` iterates in
sorted key order, so that is the registration order. Validation still covers the
whole map before any entry is added: a map whose keys collide with the registry
registers nothing.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <map>
#include <string>

int main() {
  namespace lymph = pluss::ai::lymph;

  // A preassembled registry: a std::map built up elsewhere (here, a short
  // loop standing in for a config-driven assembly) and handed to inject in
  // one call. Map keys are already unique, so inject skips the intra-batch
  // duplicate scan the braced-list form runs -- it still validates every
  // entry (empty name, name already registered) before adding any.
  std::map<std::string, lymph::models::Model> planned;
  planned.emplace("logreg", lymph::models::logistic_regression());
  planned.emplace("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 3}));
  planned.emplace("knn", lymph::models::neighbor::knn_classifier({.k = 3}));

  lymph::models::Builder builder = lymph::models::build();
  builder.inject(planned);

  // std::map iterates in sorted key order, so that is the registration order.
  std::cout << "size = " << builder.size() << "\n";
  for (const std::string& name : builder.names()) std::cout << "  - " << name << "\n";

  // Validation covers the whole map before any entry is added: a map that
  // collides with an existing name registers nothing.
  std::map<std::string, lymph::models::Model> conflicting;
  conflicting.emplace("forest", lymph::models::ensemble::random_forest_classifier());
  conflicting.emplace("tree", lymph::models::logistic_regression());
  try {
    builder.inject(conflicting);
  } catch (const std::exception& err) {
    std::cout << "conflicting map threw: " << err.what() << "\n";
  }
  std::cout << "size after failed inject = " << builder.size()
            << ", has forest = " << std::boolalpha << builder.has("forest") << "\n";

  return 0;
}
```

Output

```text
size = 3
  - knn
  - logreg
  - tree
conflicting map threw: lymph::Builder::inject: "tree" is already registered; eject it first to replace it.
size after failed inject = 3, has forest = false
```

### Example 5

The same bulk form from a `std::unordered_map`, for a registry assembled by
keyed lookup where order never mattered. Iteration order is unspecified, so
`names()` is sorted before printing for a stable view. Uses the fully explicit
`lymph::models::builder::` spelling.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  // The same bulk form, this time from a std::unordered_map. Useful when the
  // registry is assembled by name lookup and insertion order never mattered.
  // inject accepts it directly; because an unordered_map has no meaningful
  // iteration order, the resulting registration order is unspecified -- sort
  // names() if you need a stable view.
  std::unordered_map<std::string, lymph::models::Model> pool;
  pool.emplace("logreg", lymph::models::logistic_regression());
  pool.emplace("tree", lymph::models::tree::decision_tree_classifier({.max_depth = 2}));
  pool.emplace("gbm", lymph::models::ensemble::gradient_boost_classifier());

  // Full-scope spelling on the alias-free path.
  lymph::models::builder::Builder builder = lymph::models::builder::build();
  builder.inject(pool);

  std::vector<std::string> names = builder.names();
  std::sort(names.begin(), names.end());

  std::cout << "size = " << builder.size() << "\n";
  for (const std::string& name : names) std::cout << "  - " << name << "\n";
  std::cout << "has gbm = " << std::boolalpha << builder.has("gbm") << "\n";

  return 0;
}
```

Output

```text
size = 3
  - gbm
  - logreg
  - tree
has gbm = true
```

## See Also

- [eject](eject.md) — remove an entry; the first half of a deliberate replace.
- [has](has.md) — check a name before assuming it is free.
- [report](report.md) — see everything currently registered.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="list.html">
    <span class="label">Next</span>
    <span class="title">list</span>
  </a>
</div>
<!-- apiref:nav:end -->
