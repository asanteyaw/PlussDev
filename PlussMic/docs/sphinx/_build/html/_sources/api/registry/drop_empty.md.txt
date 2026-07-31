<!-- pluss-hand-authored -->

# Registry::drop_empty

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::drop_empty() const
```

## What It Does

Returns a new registry with every field whose `Slot` has **zero values** removed —
`Slot::size() == 0`, not `Slot::empty()` in the sense of "all-null" or "no valid
values". A field with three null values is not empty by this definition; a field
built from an explicitly empty value list is.

```{important} Emptiness here means zero values, not all-null values
This is a different question from [`isnull()`](isnull.md)/[`dropnull()`](dropnull.md),
which look at whether individual *values* are null. `drop_empty()` looks only at
each field's `size()` — a field holding three null values still has `size() == 3`
and survives `drop_empty()` untouched; only a field with `size() == 0` is removed.
```

## Parameters

`None`

## Returns

`Registry`
    A new registry with every zero-length field removed; every other field is
    preserved in its original order.

## Examples

Example 1. A field built from an explicitly empty value list is dropped; every
other field survives.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"empty_field", pluss::bundle::Slot(std::vector<pluss::bundle::Slot::Value>{})},
});

auto out = rg.drop_empty();
for (const auto& key : out.fields()) std::cout << key << " ";
```

### Output

```text
id
```

## Notes

- Checks `Slot::size() == 0`, not value-nullness — see the important callout
  above.
- Preserves the original order of every surviving field.
- Never mutates the receiver — use [`drop_empty_()`](drop_empty_.md) for the
  in-place form.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::drop_empty_`](drop_empty_.md): the in-place counterpart.
- [`Registry::dropnull`](dropnull.md): the value-level counterpart — drops null
  *values* within fields, rather than zero-length fields entirely.
- [`Registry::drop`](drop.md): removes fields by explicit name, rather than by
  emptiness.
- [`Registry::sizes`](sizes.md): the per-field value counts this method filters
  on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="drop_empty_.html">
    <span class="label">Next</span>
    <span class="title">drop_empty_</span>
  </a>
</div>
