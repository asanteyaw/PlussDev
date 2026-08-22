<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 72eb708190e7 -->

# Axel::drop

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::drop(std::span<const std::string_view> fields) const
Axel Axel::drop(std::initializer_list<std::string_view> fields) const
template <pluss::detail::StringViewNameRange NameRange>
Axel Axel::drop(const NameRange& fields) const
```

## What It Does

Removes the named field(s) from every registry frame in the depth
stack, returning a new `Axel`. The complementary operation to
[`fields()`](fields.md), which keeps only the named fields instead of
removing them. Implemented as [`map()`](map.md) applying
`Registry::drop(fields)` per frame.

## Parameters

**fields** (`std::span<const std::string_view>`, `std::initializer_list<std::string_view>`, or any range of string-view-like names)
    One or more field names to drop from every frame.

## Returns

`Axel`
    A new axel with every frame missing `fields`, same depth as the
    original.

## Examples

Example 1. Drop `"city"` from every frame, keeping `"ids"`.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({
        {"ids", pluss::scribble::Slot({pluss::make_series(1)})},
        {"city", pluss::scribble::Slot({pluss::make_note("Accra")})},
    }),
});
auto out = axel.drop({"city"});
std::cout << out.registry(0).nfields();
```

### Output

```text
1
```

## Notes

- Whatever `Registry::drop()` throws for a missing field name in a
  given frame propagates the same way here — every frame the drop is
  applied to must actually contain each named field.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"` (via the underlying `map()` call).

## See Also

- [`Axel::fields`](fields.md): the complementary keep-only operation.
- [`Axel::map`](map.md): the general mechanism `drop()` is built on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="distinct.html">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
