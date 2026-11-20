<!-- pluss-hand-authored -->

# ShelfLabelAccessor::set

[Back to ShelfLabelAccessor API Reference](../../shelf_label_accessor_api_reference)

## Signature

```cpp
Shelf ShelfLabelAccessor::set(const Shelf& labels) const
```

## What It Does

Replaces the bound shelf's case labels with `labels`, used as-is — a direct
forward to `Shelf::caselbl(labels)`. The shelf's own values are completely
unchanged; only the companion case-label sequence shown alongside them (and
used by [`sort()`](sort.md), [`slice()`](slice.md), [`pos()`](pos.md), and
similar) is replaced.

## Parameters

**labels** (`const Shelf&`)

The replacement case labels. Must be the same length as the bound shelf.

## Returns

`Shelf`
    A new shelf, holding the same values as `*this`, with `labels` as its
    case labels.

## Examples

Example 1. Attach text labels to a numeric shelf.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({100.0, 200.0, 150.0, 90.0}));
auto tagged = sales.label().set(
    Shelf(pluss::make_note({"east", "west", "east", "north"})));
std::cout << tagged;
```

### Output

```text
 east  100
 west  200
 east  150
north  90
ptype: Series
```

The values are exactly `sales`'s own — only the leftmost display column (the
case labels) changed.

## Notes

- The shelf's own values and kind are never touched — only the case-label
  companion sequence changes.
- Identical to calling `Shelf::caselbl(labels)` directly.
- Unlike `DataFrameLabelAccessor::set`, there is no feature-promotion overload
  here — a bare `Shelf` has no named features to promote, so this is the
  only form.

## See Also

- [`ShelfLabelAccessor::reset`](reset.md): the inverse — clears case labels
  back to the default integer range.
- [`ShelfLabelAccessor::range`](range.md): explicitly reset to the default
  range labels.
- [`ShelfLabelAccessor::values`](values.md): read the current case labels
  back out.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reset.html">
    <span class="label">Previous</span>
    <span class="title">reset</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
