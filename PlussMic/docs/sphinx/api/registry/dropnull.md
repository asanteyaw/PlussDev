<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0ae71ee86720 -->

# Registry::dropnull

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::dropnull() const
```

## What It Does

Returns a new registry with null values removed from **every** field
independently, applying [`Slot::dropnull()`](../slot/dropnull.md) to each field
via [`map()`](map.md).

```{important} Can produce a ragged registry
Because each field's nulls are dropped independently, fields that started with
different null counts end up with different lengths — a registry that was
aligned before `dropnull()` can come out ragged. Follow up with
[`nstate()`](nstate.md) if you need to assert (or [`shape()`](shape.md) if you
need to inspect) alignment afterward.
```

## Parameters

`None`

## Returns

`Registry`
    A new registry with each field's null values removed.

## Examples

Example 1. A field with one null value and a field with none — after
`dropnull()`, the two fields have different lengths.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::bundle::Slot::Value{std::monostate{}}, pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.dropnull();
std::cout << "id size: " << out.fields("id").size() << "\n";
std::cout << "city size: " << out.fields("city").size();
```

### Output

```text
id size: 2
city size: 3
```

## Notes

- See the important callout above — a previously-aligned registry can become
  ragged after this call.
- Never throws on a mismatched field kind — unlike [`fillnull()`](fillnull.md),
  which does.
- Never mutates the receiver — always returns a new `Registry`.

## See Also

- [`Registry::isnull`](isnull.md): identify null values without removing them.
- [`Registry::fillnull`](fillnull.md): replace null values instead of dropping
  them, keeping every field's length intact.
- [`Registry::nstate`](nstate.md) / [`Registry::shape`](shape.md): check field
  alignment after a `dropnull()` call.
- [`Catalog::dropnull`](../catalog/dropnull.md): the equivalent operation on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
