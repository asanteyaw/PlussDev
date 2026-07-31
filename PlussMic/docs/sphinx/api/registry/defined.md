<!-- pluss-hand-authored -->

# Registry::defined

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::defined() const noexcept -> bool
```

## What It Does

Reports whether the registry owns a concrete backing handle at all — the same
undefined/defined distinction `Thread::defined()`, `DataFrame::defined()`, and
`Catalog::defined()` make. A default-constructed `Registry` is **undefined**; almost
every other method throws `"Registry: undefined handle"` if called on one. A
`Registry` built from an explicit (even empty) field list is defined — `defined()`
is about handle existence, not about whether the registry has any fields.

```{important} An empty-but-defined registry is different from an undefined one
`Registry{}` (default-constructed) is **undefined** — `defined()` is `false`, and
calling almost anything else on it throws. `Registry(std::vector<std::pair<std::string, Slot>>{})`
(an explicit, zero-length field list) is **defined but empty** — `defined()` is
`true`, [`empty()`](empty.md) is also `true`, and every introspection method works
normally, reporting zero fields. See Example 3.
```

## Parameters

`None`

## Returns

`bool`
    `true` if the registry owns a backing handle; `false` for a default-constructed
    (moved-from, or otherwise handle-less) registry.

## Examples

Example 1. A registry built from named fields is defined.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::cout << std::boolalpha << rg.defined();
```

### Output

```text
true
```

Example 2. A default-constructed registry is undefined, and most other methods throw
when called on it.

```cpp
pluss::bundle::Registry rg;
std::cout << std::boolalpha << rg.defined() << "\n";

try {
  auto n = rg.nfields();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
false
Registry: undefined handle
```

Example 3. An explicitly empty field list produces a *defined* registry with zero
fields — not an undefined one. Compare with Example 2.

```cpp
pluss::bundle::Registry rg(std::vector<std::pair<std::string, pluss::bundle::Slot>>{});
std::cout << std::boolalpha << rg.defined() << "\n";
std::cout << std::boolalpha << rg.empty() << "\n";
std::cout << rg.nfields();
```

### Output

```text
true
true
0
```

## Notes

- `noexcept`: safe to call on any registry, including an undefined one.
- Undefined is not the same as empty — see the important callout above and Example 3.
- Almost every other `Registry` method throws `"Registry: undefined handle"` when
  called on an undefined registry; check `defined()` first if you're unsure of a
  registry's origin.

## See Also

- [`Registry::empty`](empty.md): whether a *defined* registry has zero fields — a
  different question from whether it's defined at all.
- [`Registry::unique`](unique.md): whether this registry is the sole owner of its
  backing storage, a separate question from whether that storage exists.
- [`Catalog::defined`](../catalog/defined.md): the equivalent check on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="concat_values_.html">
    <span class="label">Previous</span>
    <span class="title">concat_values_</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
