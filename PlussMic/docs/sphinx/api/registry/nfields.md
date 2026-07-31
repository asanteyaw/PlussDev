<!-- pluss-hand-authored -->

# Registry::nfields

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
std::int64_t Registry::nfields() const
```

## What It Does

Returns the number of named fields (slots) in the registry — the registry's
*width*, not the length of any individual field. This is a question about how many
slots the registry holds, not about how many values are inside any one of them.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of fields in the registry.

## Examples

Example 1. A registry with two fields.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::cout << rg.nfields();
```

### Output

```text
2
```

## Notes

- Counts fields, not values — for per-field value counts, see [`sizes()`](sizes.md);
  for the longest field's length paired with the field count, see
  [`shape()`](shape.md).
- [`size()`](size.md) and [`empty()`](empty.md) are both defined in terms of this
  method (`size()` is a pure alias; `empty()` checks `nfields() == 0`).
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::size`](size.md): a pure alias for this method.
- [`Registry::shape`](shape.md): pairs this count with the longest field's length.
- [`Registry::sizes`](sizes.md): the per-field value counts this method does not
  report.
- [`Catalog::nkeys`](../catalog/nkeys.md): the equivalent count on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="notnull.html">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>
