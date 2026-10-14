<!-- pluss-hand-authored -->

# Locus::default_policy, Locus::policy, Locus::with_policy, Locus::with_policy_

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
static Locus::default_policy() -> TabelizePolicy
Locus::policy() const -> const TabelizePolicy&
Locus::with_policy(const TabelizePolicy& policy) const -> Locus
Locus& Locus::with_policy_(const TabelizePolicy& policy)
```

## What It Does

`policy()` reads the `TabelizePolicy` used whenever a catalog needs
aligning into a rectangular table -- for [`nexus()`](nexus.md), reports, and
SQL. `default_policy()` is the value a freshly-constructed `Locus` starts
with: `Align::Outer` with null fill, so a ragged catalog's missing records
are never silently dropped the way `Catalog`'s own default
(`Align::Inner`) would drop them. `with_policy` replaces it with a new
`Locus`; `with_policy_` replaces it in place.

## Parameters

| Parameter | Meaning |
|---|---|
| `policy` | The complete replacement `TabelizePolicy`. |

## Returns

`default_policy` and `policy` return a `TabelizePolicy` (by value and by
reference, respectively). `with_policy` returns a new `Locus`.
`with_policy_` returns a reference to the mutated receiver.

## Examples

Example 1. The default policy aligns `Outer`, and matches what a fresh
`Locus` actually uses.

```cpp
const auto linked = make_linked_shop();
std::cout << "default_policy().align == policy().align: "
          << (Locus::default_policy().align == linked.policy().align) << "\n";
```

### Output

```text
default_policy().align == policy().align: true
```

Example 2. `with_policy` returns a new `Locus`; the original keeps its own
policy.

```cpp
pluss::kindle::TabelizePolicy strict;
strict.align = pluss::kindle::Align::Strict;
const auto strict_locus = linked.with_policy(strict);
std::cout << "with_policy new align=Strict: " << (strict_locus.policy().align == pluss::kindle::Align::Strict) << "\n";
std::cout << "original align unchanged: " << (linked.policy().align == pluss::kindle::Align::Outer) << "\n";
```

### Output

```text
with_policy new align=Strict: true
original align unchanged: true
```

`Align::Strict` makes ragged input an error instead of padding it -- a
different policy for whichever call actually needs that strictness.

## See Also

- [`nexus`](nexus.md): the aligned view this policy governs.
- [`alignment_cache_stats`](alignment_cache_stats.md): how alignment results
  under a given policy are cached and reused.
