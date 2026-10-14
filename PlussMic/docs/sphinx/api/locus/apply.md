<!-- pluss-hand-authored -->

# Locus::apply, Locus::apply_

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::apply(std::string_view name, const CatalogFn& fn) const -> Locus
Locus& Locus::apply_(std::string_view name, const CatalogFn& fn)
```

```cpp
using CatalogFn = std::function<Catalog(const Catalog&)>;
```

## What It Does

Rewrites one catalog's data by running `fn` on it and storing the result
under the same name -- the general-purpose "transform one catalog" method,
for anything `with(name, newValue)` doesn't already express directly (a
filter, a derived column, a type cast). `fn` receives the *current* catalog
and must return a *defined* replacement; the original `Locus` (and every
other `Locus` sharing its data) is untouched. `apply_` is the in-place form.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | The catalog to transform. |
| `fn` | A callable from the current `Catalog` to its replacement. Must not be empty, and must return a defined `Catalog`. |

## Returns

`apply` returns a new `Locus`. `apply_` returns a reference to the mutated
receiver.

## Errors

Throws when `fn` is empty (a default-constructed `std::function`), or when
`fn` returns an undefined `Catalog`.

## Examples

Example 1. Trimming one catalog's rows without touching the original.

```cpp
const auto linked = make_linked_shop();
const auto trimmed = linked.apply("orders", [](const Catalog& c) { return c.head(2); });
std::cout << "trimmed orders rows=" << trimmed.catalog("orders").maxrows()
          << " original orders rows=" << linked.catalog("orders").maxrows() << "\n";
```

### Output

```text
trimmed orders rows=2 original orders rows=4
```

Example 2. An empty callback and a callback returning an undefined `Catalog`
are both refused.

```cpp
print_error("empty callback", [&] { (void)linked.apply("orders", Locus::CatalogFn{}); });
print_error("undefined result", [&] { (void)linked.apply("orders", [](const Catalog&) { return Catalog(); }); });
```

### Output

```text
empty callback: Locus::apply: callback is empty
undefined result: Locus::apply: callback returned an undefined Catalog
```

## See Also

- [`apply_all`](apply_all.md): the same transform, run over every catalog at
  once.
- [`with`](with.md): replacing a catalog's data directly, without a
  transform function.
