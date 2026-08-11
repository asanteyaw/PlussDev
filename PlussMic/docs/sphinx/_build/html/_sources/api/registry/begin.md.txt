<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a0d3777268d8 -->

# Registry::begin

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::const_iterator Registry::begin() const
```

## What It Does

Returns an iterator over the registry's **field names** — `begin()`/[`end()`](end.md)
are what make `Registry` usable in a range-`for` loop and with standard
algorithms. `const_iterator` is `std::vector<std::string>::const_iterator`.

```{important} Iterates field names, not values or Slots
A range-`for` over a `Registry` yields `const std::string&` field names, one per
field — the same sequence [`fields()`](fields.md)'s no-arg overload returns. It
does **not** yield `Slot`s, and it does not iterate row values within any field.
```

## Parameters

`None`

## Returns

`Registry::const_iterator`
    An iterator to the first field name; equal to `end()` for an empty
    registry.

## Examples

Example 1. Iterate a registry's field names with a range-`for` loop.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

for (const auto& name : rg) std::cout << name << " ";
```

### Output

```text
id city
```

## Notes

- See the important callout above — this iterates field *names*, matching
  [`fields()`](fields.md)'s no-arg overload, not field values.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one (via the underlying `keys()` accessor).

## See Also

- [`Registry::end`](end.md): the matching end iterator.
- [`Registry::fields`](fields.md): the no-arg overload returns the same field
  names as a `std::vector<std::string>`, without needing manual iteration.
- [`Catalog::begin`](../catalog/begin.md): the equivalent iterator on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../registry_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Registry API</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
