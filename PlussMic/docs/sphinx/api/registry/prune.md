<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8c186585cd03 -->

# Registry::prune

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::prune() const
```

## What It Does

Returns a new registry with every field whose `Slot` has **zero values**
removed — `Slot::size() == 0`, not "all values are null." A field with three
null values is not empty by this definition; a field built from an explicitly
empty value list is.

```{important} Emptiness here means zero values, not all-null values
This is a different question from [`isnull()`](isnull.md)/[`dropnull()`](dropnull.md),
which look at whether individual *values* are null. `prune()` looks only at each
field's `size()` — a field holding three null values still has `size() == 3` and
survives `prune()` untouched; only a field with `size() == 0` is removed.
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

auto out = rg.prune();
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
- Never mutates the receiver — use [`prune_()`](prune_.md) for the in-place
  form.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::prune_`](prune_.md): the in-place counterpart.
- [`Registry::dropnull`](dropnull.md): the value-level counterpart — drops null
  *values* within fields, rather than zero-length fields entirely.
- [`Registry::drop`](drop.md): removes fields by explicit name, rather than by
  emptiness.
- [`Registry::sizes`](sizes.md): the per-field value counts this method
  filters on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="profiles.html">
    <span class="label">Previous</span>
    <span class="title">profiles</span>
  </a>
  <a class="next-link" href="prune_.html">
    <span class="label">Next</span>
    <span class="title">prune_</span>
  </a>
</div>
