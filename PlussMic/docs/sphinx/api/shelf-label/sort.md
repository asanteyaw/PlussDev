<!-- pluss-hand-authored -->

# ShelfLabelAccessor::sort

[Back to ShelfLabelAccessor API Reference](../../shelf_label_accessor_api_reference)

## Signature

```cpp
Shelf ShelfLabelAccessor::sort(bool ascending = true, bool nulls_last = true) const
```

## What It Does

Reorders the **entire bound shelf** — its own values together with their
case labels — by case-label value. This is the one `ShelfLabelAccessor`
method that touches the shelf's actual data rather than just the case-label
companion sequence: every row moves together as a unit, so a value and the
case label it was originally paired with always stay matched after sorting.

## Parameters

**ascending** (`bool`, default `true`)

Sort ascending if `true`, descending if `false`.

**nulls_last** (`bool`, default `true`)

Where null case labels sort relative to real values.

## Returns

`Shelf`
    A new shelf with the same rows as `*this`, reordered by case label.

## Examples

Example 1. Sort a case-labeled shelf ascending and descending.

```cpp
using namespace pluss::table;

auto tagged = Shelf(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Shelf(pluss::make_note({"east", "west", "east", "north"})));
std::cout << tagged;

auto ascending = tagged.label().sort();
std::cout << ascending;

auto descending = tagged.label().sort(false);
std::cout << descending;
```

### Output (before)

```text
 east  100
 west  200
 east  150
north  90
ptype: Series
```

### Output (ascending)

```text
 east  100
 east  150
north  90
 west  200
ptype: Series
```

### Output (descending)

```text
 west  200
north  90
 east  100
 east  150
ptype: Series
```

Each value (`100`, `200`, `150`, `90`) moves together with the case label it
started paired with — `"east"` stays next to `100` (its first occurrence)
and `150` (its second) throughout. Ties (the two `"east"` rows) keep their
original relative order either way — the sort is stable.

## Notes

- Sorting is stable: rows with equal case labels retain their original
  relative order.
- Unlike most other `ShelfLabelAccessor` methods (which only return a
  transformed view of the case labels themselves), `sort()` reorders the
  bound shelf's actual values along with its labels.

## See Also

- [`ShelfLabelAccessor::monotonic`](monotonic.md): check sortedness without
  reordering.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="symdiff.html">
    <span class="label">Next</span>
    <span class="title">symdiff</span>
  </a>
</div>
