<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 126503cc35ce -->

# Registry::concat_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry& Registry::concat_(std::string_view key, const Slot& other)
```

## What It Does

The in-place counterpart to [`concat()`](concat.md) — appends `other`'s values
to one named field's `Slot` directly on this registry.

## Parameters

**key** (`std::string_view`)
    The field to append to. Throws if it doesn't exist.

**other** (`const Slot&`)
    The values to append. Must be defined.

## Returns

`Registry&`
    A reference to this registry, now holding the extended field.

## Examples

Example 1. Append one more value onto `id`, in place.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

auto extra = pluss::bundle::Slot({pluss::make_series(4)});
rg.concat_("id", extra);

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
[1, 2, 3, 4]
```

## Notes

- Same throw conditions as [`concat()`](concat.md).
- Mutates the registry it's called on and returns `*this` by reference.

## See Also

- [`Registry::concat`](concat.md): the non-mutating counterpart.
- [`Registry::push_`](push_.md): append a single value in place, rather than an
  entire `Slot`'s worth.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="concat.html">
    <span class="label">Previous</span>
    <span class="title">concat</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
