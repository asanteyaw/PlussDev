<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e8d6e5372185 -->

# Axel::fillnull

[Back to Axel API Reference](../../axel_api_reference)

## Signatures

```cpp
Axel Axel::fillnull(double value) const
Axel Axel::fillnull(std::string_view value) const
Axel Axel::fillnull(const char* value) const
Axel Axel::fillnull(const pluss::Timeline& value) const
Axel Axel::fillnull(const pluss::TimeIndex& value) const
```

## What It Does

Returns a new axel with null values replaced by `value`, applying
[`Registry::fillnull(value)`](../registry/fillnull.md) to **every**
frame via [`map()`](map.md) — one overload per fill-value kind
(`double` for numeric fields, `std::string_view`/`const char*` for text
fields, `Timeline`/`TimeIndex` for datetime fields).

```{important} Every field in every frame is filled, and every field's kind must match
Because `fillnull()` applies unconditionally to every field of every
frame via nested `map()` calls, calling `fillnull(double value)` on an
axel where **any** frame has a non-numeric field throws — even if that
field has zero null values. The same
[gotcha documented on `Registry::fillnull`](../registry/fillnull.md)
applies here, just across every frame instead of one registry. There is
no way to scope `fillnull()` to specific fields at the `Axel` level —
for a mixed-kind axel, narrow with [`fields()`](fields.md) first.
```

## Parameters

**value** (`double` / `std::string_view` / `const char*` / `const pluss::Timeline&` / `const pluss::TimeIndex&`)
    The replacement for null values, in the kind matching the overload
    selected.

## Returns

`Axel`
    A new axel with every field's null values, in every frame, replaced
    by `value`.

## Examples

Example 1. On an axel where every field is numeric, `fillnull(double)`
succeeds and fills the null.

```cpp
auto r0 = pluss::scribble::Registry({{"id", pluss::scribble::Slot({pluss::make_series(1), pluss::scribble::Slot::Value{std::monostate{}}, pluss::make_series(3)})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fillnull(99.0);
std::cout << out.registry(0).fields("id").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 3,
  "values": [
    "Series[int64]",
    "Series[float64]",
    "Series[int64]"
  ]
}
```

## Notes

- See the important callout above — this is the same surprising
  behavior documented for `Registry::fillnull()`, extended across every
  frame: a throw can come from a field with no nulls at all, just
  because its *kind* doesn't match the overload chosen.
- Filling a null in an integer `Series` field with a `double` promotes
  that value's numeric entry to `Series[float64]` — see Example 1.
- Never mutates the receiver — always returns a new `Axel`.

## See Also

- [`Axel::dropnull`](dropnull.md): removes null values instead of
  filling them, and never throws over kind mismatches.
- [`Axel::isnull`](isnull.md): identify which values are null before
  deciding how to fill them.
- [`Axel::fields`](fields.md): scope a fill to a single field on a
  mixed-kind axel.
- [`Registry::fillnull`](../registry/fillnull.md): the single-frame
  operation this method broadcasts across the depth stack.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fields.html">
    <span class="label">Previous</span>
    <span class="title">fields</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
