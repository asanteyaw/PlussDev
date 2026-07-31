<!-- pluss-hand-authored -->

# Registry::shape

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
std::pair<std::int64_t, std::int64_t> Registry::shape() const
```

## What It Does

Returns a `(rows, fields)` pair summarizing the registry's overall size in one call
— the first element is the **longest** field's length (found by scanning every
slot's [`size()`](../slot/size.md)), the second is [`nfields()`](nfields.md).
Because it takes the maximum rather than requiring agreement, `shape()` never
throws on a **ragged** registry (one whose fields don't all share the same
length) — unlike `Registry`'s stream-conversion methods, which require every field
to individually be stream-compatible but say nothing about fields matching each
other's length.

## Parameters

`None`

## Returns

`std::pair<std::int64_t, std::int64_t>`
    `(longest field's length, nfields())`.

## Examples

Example 1. A registry whose fields all share the same length.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto [rows, fields] = rg.shape();
std::cout << "(" << rows << ", " << fields << ")";
```

### Output

```text
(3, 2)
```

Example 2. A **ragged** registry (fields of different lengths) reports the longest
field's length as the row count, rather than throwing.

```cpp
auto ragged = pluss::bundle::Registry({
    {"a", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"b", pluss::bundle::Slot({pluss::make_series(10), pluss::make_series(20)})},
});

auto [rows, fields] = ragged.shape();
std::cout << "(" << rows << ", " << fields << ")";
```

### Output

```text
(3, 2)
```

## Notes

- The pair order is `(rows, fields)`, matching typical row-by-column shape
  conventions — not `(fields, rows)`.
- Never throws on a ragged registry — see Example 2. `Registry` does not require
  fields to share a common length the way `Catalog`'s streams often do.
- An explicitly empty registry reports `(0, 0)`.

## See Also

- [`Registry::nfields`](nfields.md) / [`Registry::sizes`](sizes.md): the two kinds
  of value this method combines — field count, and each field's own length.
- [`Catalog::shape`](../catalog/shape.md): the equivalent summary on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_.html">
    <span class="label">Previous</span>
    <span class="title">set_</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
