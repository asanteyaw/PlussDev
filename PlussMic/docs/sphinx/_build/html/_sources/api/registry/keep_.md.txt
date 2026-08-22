<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4c7bfac2ecab -->

# Registry::keep_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry& Registry::keep_(std::string_view key, const std::function<bool(const Slot::Value&)>& predicate)
```

## What It Does

The in-place counterpart to [`keep()`](keep.md) — filters one named field's
`Slot` down to only the values for which `predicate` returns `true`, directly on
this registry.

## Parameters

**key** (`std::string_view`)
    The field to filter. Throws if it doesn't exist.

**predicate** (`const std::function<bool(const Slot::Value&)>&`)
    A callback taking each of the field's values in turn; values for which it
    returns `false` are dropped.

## Returns

`Registry&`
    A reference to this registry, now holding the filtered field.

## Examples

Example 1. Keep only the odd values in `id`, in place.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

rg.keep_("id", [](const pluss::scribble::Slot::Value& v) {
  return std::get<pluss::Series>(v).to_vec<std::int64_t>()[0] % 2 == 1;
});

auto ids = rg.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[1, 3]
```

## Notes

- Same throw conditions as [`keep()`](keep.md).
- Mutates the registry it's called on and returns `*this` by reference.

## See Also

- [`Registry::keep`](keep.md): the non-mutating counterpart.
- [`Registry::morph_`](morph_.md): transform a field's values in place, rather
  than filtering them out.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keep.html">
    <span class="label">Previous</span>
    <span class="title">keep</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
