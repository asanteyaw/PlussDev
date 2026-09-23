<!-- pluss-hand-authored -->

# ThreadLabelAccessor::take

[Back to ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference)

## Signature

```cpp
ThreadLabelAccessor::take(pluss::msharp::RangeView::Int indices) const -> Thread
```

## What It Does

Selects case-label values at the given positions, in the order given —
positions may repeat or appear out of order. Returns the selected **values**
as a bare `Thread`, not the bound thread's own data at those positions.

## Parameters

**indices** (`pluss::msharp::RangeView::Int`)

Positions to select, in the order the results should come back.

## Returns

`Thread`
    The case-label values at those positions, plain default integer index.

## Examples

Example 1. Select positions out of order, with a repeat.

```cpp
using namespace pluss::table;

auto tagged = Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Thread(pluss::make_note({"east", "west", "east", "north"})));

auto out = tagged.label().take({2, 0, 3});
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

- Returns the case-label **values** only — if the matching data rows are
  needed, call `Thread::take` on the bound thread itself with the same
  indices.
- Positions may repeat; there's no requirement that `indices` be sorted or
  unique.

## See Also

- [`ThreadLabelAccessor::slice`](slice.md): a contiguous positional or
  label-bounded range, instead of an arbitrary position list.

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
