<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 63ce27fb8892 -->

# DataFrameLabelAccessor::unite

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
Slate DataFrameLabelAccessor::unite(const Slate& other) const
```

## What It Does

Treats the case labels and `other` as two sets of values, and returns their
union — deduplicated, this accessor's distinct values first (in
first-occurrence order), then `other`'s distinct values not already seen.

## Parameters

**other** (`const Slate&`)

The values to union with.

## Returns

`Slate`
    The distinct values from both sides combined, `display_name` `"unite"`.

## Examples

Example 1. Union case labels with a repeat against another set with one
overlapping value.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.labels().set("region");

auto other = Slate(pluss::make_note({"west", "south"}));
auto out = labeled.labels().unite(other);
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

- [`DataFrameLabelAccessor::intersect`](intersect.md),
  [`DataFrameLabelAccessor::diff`](diff.md),
  [`DataFrameLabelAccessor::symdiff`](symdiff.md): the other set-algebra
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
