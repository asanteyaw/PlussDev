<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1ce95b95829f -->

# Registry::size

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
std::int64_t Registry::size() const
```

## What It Does

A pure alias for [`nfields()`](nfields.md) — returns the number of named fields in
the registry. `size()` exists for readability at call sites that expect a generic
"how big is this container" query; `nfields()` exists for call sites where "how
many fields" is the more precise framing. There is no behavioral difference.

```{important} Counts fields, not values
Like `nfields()`, `size()` reports the registry's *width* (how many named fields it
holds), not the length of any individual field. For per-field value counts, see
[`sizes()`](sizes.md).
```

## Parameters

`None`

## Returns

`std::int64_t`
    The number of fields in the registry — identical to `nfields()`.

## Examples

Example 1. `size()` and `nfields()` report the same value.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::cout << rg.size() << " " << rg.nfields();
```

### Output

```text
2 2
```

## Notes

- Identical to `nfields()` in every respect — pick whichever name reads better at
  the call site.
- Not to be confused with [`sizes()`](sizes.md) (plural), which returns per-field
  value counts, not the field count.

## See Also

- [`Registry::nfields`](nfields.md): the method this is a pure alias for.
- [`Registry::sizes`](sizes.md): the plural, per-field counterpart — a very
  different question.
- [`Catalog::size`](../catalog/size.md): the equivalent alias on the kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="sizes.html">
    <span class="label">Next</span>
    <span class="title">sizes</span>
  </a>
</div>
