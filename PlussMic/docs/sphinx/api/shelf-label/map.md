<!-- pluss-hand-authored -->

# ShelfLabelAccessor::map

[Back to ShelfLabelAccessor API Reference](../../shelf_label_accessor_api_reference)

## Signature

```cpp
Shelf ShelfLabelAccessor::map(const std::function<ShelfValue(const ShelfValue&)>& fn) const
```

## What It Does

Applies `fn` to every case label individually and reattaches the results as
the bound shelf's new case labels — a direct forward to
`Shelf::caselbl(Shelf::caselbl().map(fn))`. The shelf's own values are
untouched; only the case-label companion sequence changes.

```{note} Always reachable — unlike DataFrameLabelAccessor::map
`DataFrameLabelAccessor::map` throws on a structured (multi-level) frame,
since there's no single flat shelf to map over. A `Shelf`'s case labels
are always a single flat sequence, so `ShelfLabelAccessor::map` never has
that concern.
```

## Parameters

**fn** (`const std::function<ShelfValue(const ShelfValue&)>&`)

A callback receiving one case label at a time (wrapped in a `ShelfValue`)
and returning its replacement.

## Returns

`Shelf`
    A new shelf, holding the same values as `*this`, with case labels
    replaced by `fn`'s output.

## Examples

Example 1. Append a suffix to every case label.

```cpp
using namespace pluss::table;

auto tagged = Shelf(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Shelf(pluss::make_note({"r1", "r2", "r3", "r4"})));
std::cout << tagged;

auto out = tagged.label().map([](const ShelfValue& v) {
  return ShelfValue(Shelf(pluss::make_note({v.to_str() + "!"})));
});
std::cout << out;
```

### Output (before)

```text
r1  100
r2  200
r3  150
r4  90

ptype: Series
```

### Output (after)

```text
r1!  100
r2!  200
r3!  150
r4!  90

ptype: Series
```

Each case label is transformed by appending `"!"`; the shelf's own `sales`-
style values are exactly as they were.

## Notes

- `fn` must return a `ShelfValue` of a kind consistent with the rest of the
  output (a length-1 `Shelf` wrapped in `ShelfValue`, as constructed above).
- Only the case labels change — the bound shelf's own values are untouched.

## See Also

- [`ShelfLabelAccessor::values`](values.md): pull the raw label shelf if a
  transform outside `map(...)` is needed.
- [`ShelfLabelAccessor::set`](set.md): the underlying mechanism this method
  applies its result through.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="monotonic.html">
    <span class="label">Next</span>
    <span class="title">monotonic</span>
  </a>
</div>
