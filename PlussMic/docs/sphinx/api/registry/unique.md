<!-- pluss-hand-authored -->

# Registry::unique

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
bool Registry::unique() const noexcept
```

## What It Does

Reports whether the current registry is the **sole owner** of its backing storage —
a reference-count check on the internal handle (`defined() && refcount == 1`), not a
check about whether the registry's *data* is free of duplicate values. `Registry`
shares storage cheaply on copy; `unique()` tells you whether that sharing is
currently happening.

```{important} This is not about duplicate fields or values
Despite the name, `unique()` has nothing to do with [`distinct()`](distinct.md) or
duplicate detection on the registry's contents. It answers a single question: "is
any other `Registry` handle currently pointing at the same backing storage as this
one?" Two independently built registries with identical field contents are each
`unique()` on their own — copying is what breaks uniqueness, not data collisions.
See Example 2.
```

## Parameters

`None`

## Returns

`bool`
    `true` if the registry is defined and no other `Registry` handle currently
    shares its storage; `false` if it's undefined, or if a copy is currently
    sharing storage with it.

## Examples

Example 1. A freshly built registry with no copies is unique.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

std::cout << std::boolalpha << rg.unique();
```

### Output

```text
true
```

Example 2. Copying a registry makes **both** the original and the copy report
`unique() == false`, since they now share the same backing storage.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

auto copy = rg;
std::cout << std::boolalpha << rg.unique() << "\n";
std::cout << std::boolalpha << copy.unique();
```

### Output

```text
false
false
```

Example 3. An undefined registry is never unique.

```cpp
pluss::bundle::Registry rg;
std::cout << std::boolalpha << rg.unique();
```

### Output

```text
false
```

## Notes

- `noexcept`: safe to call on any registry, including an undefined one — an
  undefined registry always returns `false`.
- Purely a reference-count check on the internal handle — unrelated to whether the
  registry's field contents contain duplicate values. See the important callout
  above.
- Copying a `Registry` (copy constructor or copy-assignment) is what breaks
  uniqueness; every purely functional transform method returns freshly built
  storage instead.

## See Also

- [`Registry::defined`](defined.md): whether the registry has *any* backing storage
  at all, independent of sharing.
- [`Registry::distinct`](distinct.md): the actual duplicate-value-detection
  operation, not to be confused with this method's storage-sharing check.
- [`Catalog::unique`](../catalog/unique.md): the equivalent check on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="type_profiles.html">
    <span class="label">Previous</span>
    <span class="title">type_profiles</span>
  </a>
</div>
