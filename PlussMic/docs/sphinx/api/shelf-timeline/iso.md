<!-- pluss-hand-authored -->

# ShelfTimelineAccessor::iso

[Back to ShelfTimelineAccessor API Reference](../../shelf_timeline_accessor_api_reference)

## Signature

```cpp
Shelf ShelfTimelineAccessor::iso() const
```

## What It Does

Replaces the bound shelf with a fixed ISO-8601-with-offset text
representation, as a `Note` `Series`. Equivalent to
[`fmt("%Y-%m-%dT%H:%M:%S.%f%z")`](fmt.md).

## Parameters

`None`

## Returns

`Shelf`
    A new `Note`-kind shelf, same length as the bound shelf, holding each
    row as `"YYYY-MM-DDTHH:MM:SS.fffffffff+HH:MM"`.

## Examples

Example 1. Render every row as ISO-8601 text.

```cpp
using namespace pluss::table;

auto events = Shelf(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().iso();
std::cout << out;
```

### Output

```text
0  2024-01-31T09:15:30.0~
1  2024-07-06T14:00:00.0~
ptype: Note
```

## Notes

- Always includes the full 9-digit nanosecond fraction and a signed `%z`
  offset — there's no shorter variant; use [`fmt()`](fmt.md) with a custom
  pattern for a more compact string.
- The offset is always `+HH:MM`/`-HH:MM` — UTC renders as `"+00:00"`, never a
  bare `"Z"`.
- `.tl()` requires the bound shelf to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying shelf) displays as
  `"NULL"`.

## See Also

- [`ShelfTimelineAccessor::fmt`](fmt.md): the general form this method is a
  fixed pattern of.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnat.html">
    <span class="label">Previous</span>
    <span class="title">isnat</span>
  </a>
  <a class="next-link" href="leap.html">
    <span class="label">Next</span>
    <span class="title">leap</span>
  </a>
</div>
