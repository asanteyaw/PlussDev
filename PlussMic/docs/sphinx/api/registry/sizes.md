<!-- pluss-hand-authored -->

# Registry::sizes

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
std::unordered_map<std::string, std::int64_t> Registry::sizes() const
```

## What It Does

Returns each field's own value count, keyed by field name — the per-field
counterpart to [`nfields()`](nfields.md)/[`size()`](size.md), which count fields
rather than values. Useful for spotting a **ragged** registry (fields of differing
lengths) at a glance.

## Parameters

`None`

## Returns

`std::unordered_map<std::string, std::int64_t>`
    A map from field name to that field's value count (`Slot::size()`).

## Examples

Example 1. A ragged registry's field lengths, read back sorted for a deterministic
printout.

```cpp
auto ragged = pluss::bundle::Registry({
    {"a", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"b", pluss::bundle::Slot({pluss::make_series(10), pluss::make_series(20)})},
});

auto sizes = ragged.sizes();
std::vector<std::pair<std::string, std::int64_t>> items(sizes.begin(), sizes.end());
std::sort(items.begin(), items.end());
for (const auto& [key, count] : items) std::cout << key << ": " << count << "\n";
```

### Output

```text
a: 3
b: 2
```

## Notes

- Unordered — iterate a sorted copy (as in the example) if you need deterministic
  output.
- Distinct from [`size()`](size.md) (singular), which returns the field count, not
  per-field value counts.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::shape`](shape.md): combines the *longest* field's length with the
  field count in one call, rather than every field's individual length.
- [`Registry::size`](size.md) / [`Registry::nfields`](nfields.md): the field-count
  methods this one is not to be confused with.
- [`Registry::type_profiles`](type_profiles.md): the per-field *type* breakdown,
  a related but distinct summary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
