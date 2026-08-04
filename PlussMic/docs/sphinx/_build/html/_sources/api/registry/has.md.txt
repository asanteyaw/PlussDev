<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 63e705043173 -->

# Registry::has

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
bool Registry::has(std::string_view key) const
```

## What It Does

Checks whether a named field exists in the registry, without throwing regardless of
the answer. This is the safe way to test for a key's presence before calling
[`fields(key)`](fields.md) (which throws on a missing key) — or use
[`get()`](get.md) directly if you also want the `Slot` itself when present.

## Parameters

**key** (`std::string_view`)
    The field name to check.

## Returns

`bool`
    `true` if a field with that exact name exists; `false` otherwise (including for
    a misspelled or differently-cased name — key lookup is exact and
    case-sensitive).

## Examples

Example 1. An existing key returns `true`; a missing one returns `false` — no
throw.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::cout << std::boolalpha << rg.has("city") << " " << rg.has("zzz");
```

### Output

```text
true false
```

Example 2. Key lookup is exact and case-sensitive — a differently-cased name is
treated as missing.

```cpp
auto rg = pluss::bundle::Registry({
    {"City", pluss::bundle::Slot({pluss::make_note("Accra")})},
});

std::cout << std::boolalpha << rg.has("city") << " " << rg.has("City");
```

### Output

```text
false true
```

## Notes

- Never throws — `has()` is the safe presence check, unlike [`fields(key)`](fields.md),
  which throws on a missing field.
- Key comparison is exact and case-sensitive.

## See Also

- [`Registry::fields`](fields.md): fetch the field's `Slot` directly (single-key
  overload), throwing if the key is missing.
- [`Registry::get`](get.md): fetch the field's `Slot` as `std::optional<Slot>`,
  combining the presence check and the fetch into one call.
- [`Registry::has_all`](has_all.md) / [`Registry::has_any`](has_any.md): the
  multi-key versions of this check.
- [`Catalog::has`](../catalog/has.md): the equivalent check on the kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="has_all.html">
    <span class="label">Next</span>
    <span class="title">has_all</span>
  </a>
</div>
