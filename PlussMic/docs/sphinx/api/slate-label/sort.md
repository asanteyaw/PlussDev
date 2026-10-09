<!-- pluss-hand-authored -->

# SlateLabelAccessor::sort

[Back to SlateLabelAccessor API Reference](../../slate_label_accessor_api_reference)

## Signature

```cpp
SlateLabelAccessor::sort(bool ascending = true, bool nulls_last = true) const -> Slate
```

## What It Does

Reorders the **entire bound slate** — its own values together with their
case labels — by case-label value. This is the one `SlateLabelAccessor`
method that touches the slate's actual data rather than just the case-label
companion sequence: every row moves together as a unit, so a value and the
case label it was originally paired with always stay matched after sorting.

## Parameters

**ascending** (`bool`, default `true`)

Sort ascending if `true`, descending if `false`.

**nulls_last** (`bool`, default `true`)

Where null case labels sort relative to real values.

## Returns

`Slate`
    A new slate with the same rows as `*this`, reordered by case label.

## Examples

Example 1. Sort a case-labeled slate ascending and descending.

```cpp
using namespace pluss::table;

auto tagged = Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Slate(pluss::make_note({"east", "west", "east", "north"})));
std::cout << tagged;

auto ascending = tagged.label().sort();
std::cout << ascending;

auto descending = tagged.label().sort(false);
std::cout << descending;
```

### Output (before)

```text
east   100
west   200
east   150
north  90

ptype: Series
```

### Output (ascending)

```text
east   100
east   150
north  90
west   200

ptype: Series
```

### Output (descending)

```text
west   200
north  90
east   100
east   150

ptype: Series
```

Each value (`100`, `200`, `150`, `90`) moves together with the case label it
started paired with — `"east"` stays next to `100` (its first occurrence)
and `150` (its second) throughout. Ties (the two `"east"` rows) keep their
original relative order either way — the sort is stable.

## Notes

- Sorting is stable: rows with equal case labels retain their original
  relative order.
- Unlike most other `SlateLabelAccessor` methods (which only return a
  transformed view of the case labels themselves), `sort()` reorders the
  bound slate's actual values along with its labels.

## See Also

- [`SlateLabelAccessor::monotonic`](monotonic.md): check sortedness without
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
