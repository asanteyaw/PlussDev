<!-- pluss-hand-authored -->

# ShelfLabelAccessor::unite

[Back to ShelfLabelAccessor API Reference](../../shelf_label_accessor_api_reference)

## Signature

```cpp
Shelf ShelfLabelAccessor::unite(const Shelf& other) const
```

## What It Does

Treats the bound shelf's case labels and `other` as two sets of values, and
returns their union — deduplicated, this accessor's distinct values first (in
first-occurrence order), then `other`'s distinct values not already seen.

## Parameters

**other** (`const Shelf&`)

The values to union with.

## Returns

`Shelf`
    The distinct values from both sides combined, `display_name` `"unite"`.

## Examples

Example 1. Union case labels with a repeat against another set with one
overlapping value.

```cpp
using namespace pluss::table;

auto tagged = Shelf(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Shelf(pluss::make_note({"east", "west", "east", "north"})));

auto other = Shelf(pluss::make_note({"west", "south"}));
auto out = tagged.label().unite(other);
std::cout << out;
```

### Output

```text
0  east
1  west
2  north
3  south
Name: unite, ptype: Note
```

The case labels' distinct values come first (`"east"`, `"west"`, `"north"`,
skipping the repeated `"east"`), followed by `other`'s values not already
present (`"south"` — `"west"` is skipped since it's already in the result).

## Notes

- Duplicates on either side are collapsed; the result never repeats a value.
- When the two operands share a common representable dtype (same `Series`
  scalar type, both numeric, or the same non-`Series` kind on the same
  platform), the result preserves that dtype and concatenates directly;
  otherwise it falls back to a `Note` of each value's string form.

## See Also

- [`ShelfLabelAccessor::intersect`](intersect.md),
  [`ShelfLabelAccessor::diff`](diff.md),
  [`ShelfLabelAccessor::symdiff`](symdiff.md): the other set-algebra
  operations over case-label values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="values.html">
    <span class="label">Next</span>
    <span class="title">values</span>
  </a>
</div>
