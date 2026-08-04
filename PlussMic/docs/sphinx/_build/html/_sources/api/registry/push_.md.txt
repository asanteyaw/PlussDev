<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7c576a966a43 -->

# Registry::push_

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
Registry& Registry::push_(std::string_view key, const Slot::Value& value)
Registry& Registry::push_(const Slot::Value& value)
```

## What It Does

The in-place counterpart to [`push()`](push.md) — appends one value to a field's
`Slot` directly. The single-key overload appends to just that field; the
value-only overload appends to every field.

## Parameters

**key** (`std::string_view`)
    The field to append to (single-key overload). Throws if it doesn't exist.

**value** (`const Slot::Value&`)
    The value to append.

## Returns

`Registry&`
    A reference to this registry, now holding the appended value(s).

## Examples

Example 1. Append to a single named field in place.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

rg.push_("id", pluss::make_series(4));
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

- Same throw and every-field behavior as [`push()`](push.md) — see that page's
  notes.
- Mutates the registry it's called on and returns `*this` by reference.

## See Also

- [`Registry::push`](push.md): the non-mutating counterpart.
- [`Registry::concat_`](concat_.md): append a whole `Slot`'s worth of values in
  place, rather than a single value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="push.html">
    <span class="label">Previous</span>
    <span class="title">push</span>
  </a>
  <a class="next-link" href="rename.html">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>
