<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e06cc0a68ea6 -->

# AtlasLabelAccessor::slice

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::slice(std::string_view start, std::string_view stop) const -> Thread
```

## What It Does

Returns the atlas's depth labels lying between `start` and `stop`, inclusive of both
ends — a label-based (rather than positional) slice, matching the label-inclusive
convention used across the rest of PlussMic's `.loc`-style APIs.

## Parameters

**start** (`std::string_view`)
    The first depth label to include.

**stop** (`std::string_view`)
    The last depth label to include (inclusive, unlike a positional slice's exclusive
    stop).

## Returns

`Thread`
    The depth labels from `start` through `stop`, inclusive, named `"slice"`.

## Examples

Example 1. Slice a text-labeled atlas's depth labels between two bounds.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 90.0}))},
});
auto q3 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({95.0, 130.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2, q3}, pluss::table::Thread(pluss::make_note({"jan", "feb", "mar"})));

std::cout << atlas.label().slice("feb", "mar");
```

### Output

```text
0  feb
1  mar
Name: slice, ptype: Note
```

## Notes

- Inclusive of both `start` and `stop`, unlike a positional slice
  (e.g. [`Atlas::slice_frames`](../atlas/slice_frames.md)), whose `stop` is exclusive.
- Only a `std::string_view` overload exists — there is no `double`-based slice for
  numeric depth labels, unlike [`pos()`](pos.md)'s three overloads.
- Returns the label values, not the frames themselves; combine with
  [`align()`](align.md) if you need the atlas subset to those depth positions.

## See Also

- [`Atlas::slice_frames`](../atlas/slice_frames.md): the positional (exclusive-stop)
  equivalent, operating on frame index rather than depth label.
- [`AtlasLabelAccessor::align`](align.md): subset the bound atlas's frames to a specific
  label set, such as the result of this slice.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set.html">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="sort.html">
    <span class="label">Next</span>
    <span class="title">sort</span>
  </a>
</div>
