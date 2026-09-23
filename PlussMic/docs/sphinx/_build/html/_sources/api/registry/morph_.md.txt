<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 15cb8a4a73a5 -->

# Registry::morph_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry& Registry::morph_(std::string_view key, const std::function<Slot::Value(const Slot::Value&)>& fn)
```

## What It Does

The in-place counterpart to [`morph()`](morph.md) — transforms one named field's
values individually, directly on this registry.

## Parameters

**key** (`std::string_view`)
    The field to transform. Throws if it doesn't exist.

**fn** (`const std::function<Slot::Value(const Slot::Value&)>&`)
    A callback taking each of the field's values in turn and returning its
    replacement.

## Returns

`Registry&`
    A reference to this registry, now holding the transformed field.

## Examples

Example 1. Add 100 to every value in `id`, in place.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

rg.morph_("id", [](const pluss::scribble::Slot::Value& v) -> pluss::scribble::Slot::Value {
  return pluss::make_series(std::get<pluss::Series>(v).to_vec<std::int64_t>()[0] + 100);
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
[101, 102, 103]
```

## Notes

- Same throw conditions as [`morph()`](morph.md).
- Mutates the registry it's called on and returns `*this` by reference.

## See Also

- [`Registry::morph`](morph.md): the non-mutating counterpart.
- [`Registry::map_`](map_.md): the whole-`Slot`, every-field in-place
  counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="morph.html">
    <span class="label">Previous</span>
    <span class="title">morph</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
