<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d9dfbce0d97e -->

# DataFrameLabelAccessor::symdiff

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::symdiff(const Slate& other) const -> Slate
```

## What It Does

Treats the case labels and `other` as two sets of values, and returns the
labels present in **exactly one** of the two (the symmetric difference) —
deduplicated, in this accessor's first-occurrence order followed by `other`'s.

## Parameters

**other** (`const Slate&`)

The values to compare against.

## Returns

`Slate`
    The distinct values found in exactly one of the two sides, `display_name`
    `"symdiff"`.

## Examples

Example 1. Symmetric difference between case labels with a repeat and another
set.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region");

auto other = Slate(pluss::make_note({"west", "south"}));
auto out = labeled.label().symdiff(other);
std::cout << out;
```

### Output

```text
0  east
1  north
2  south
Name: symdiff, ptype: Note
```

`"west"` is shared by both sides, so it's excluded; `"east"` and `"north"` are
only on the left, `"south"` only on the right — all three make it into the
result, left-side values first.

## Notes

- Unlike [`diff()`](diff.md), `symdiff()` is symmetric: swapping the operands
  produces the same set of values (order may differ).
- Duplicates on either side are collapsed; the result never repeats a value.

## See Also

- [`DataFrameLabelAccessor::diff`](diff.md): the one-sided ("mine minus yours")
  set difference.
- [`DataFrameLabelAccessor::unite`](unite.md),
  [`DataFrameLabelAccessor::intersect`](intersect.md): the other set-algebra
  operations over case-label values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="swap.html">
    <span class="label">Previous</span>
    <span class="title">swap</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
