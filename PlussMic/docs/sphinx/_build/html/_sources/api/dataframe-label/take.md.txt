<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c66fe40e8fcd -->

# DataFrameLabelAccessor::take

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::take(pluss::msharp::RangeView::Int indices) const -> Slate
```

## What It Does

Selects case-label values at the given positions, in the order given —
positions may repeat or appear out of order. Returns the selected **values**
as a bare `Slate`, not the corresponding `DataFrame` rows.

## Parameters

**indices** (`pluss::msharp::RangeView::Int`)

Positions to select, in the order you want them back.

## Returns

`Slate`
    The case-label values at those positions, plain default integer index.

## Examples

Example 1. Select positions out of order, including a repeat-free selection.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.labels().set("region");

auto out = labeled.labels().take({2, 0, 3});
std::cout << out;
```

### Output

```text
0  east
1  east
2  north
ptype: Note
```

Position `2` (`"east"`) comes first, then position `0` (also `"east"`), then
position `3` (`"north"`) — position `1` (`"west"`) was never requested, so it
doesn't appear.

## Notes

- Returns the case-label **values** only — if you need the matching
  `DataFrame` rows, use [`DataFrame::take`](../dataframe/take.md) on the frame
  itself with the same indices.
- Positions may repeat; there's no requirement that `indices` be sorted or
  unique.

## See Also

- [`DataFrameLabelAccessor::slice`](slice.md): a contiguous positional or
  label-bounded range, instead of an arbitrary position list.
- [`DataFrame::take`](../dataframe/take.md): the frame-level counterpart that
  selects whole rows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="symdiff.html">
    <span class="label">Previous</span>
    <span class="title">symdiff</span>
  </a>
  <a class="next-link" href="unite.html">
    <span class="label">Next</span>
    <span class="title">unite</span>
  </a>
</div>
