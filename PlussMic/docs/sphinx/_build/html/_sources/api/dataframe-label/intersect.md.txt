<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a6e6b7ffd2f0 -->

# DataFrameLabelAccessor::intersect

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::intersect(const Thread& other) const -> Thread
```

## What It Does

Treats the case labels and `other` as two sets of values, and returns the
labels present in **both** — deduplicated, in the case labels' own
first-occurrence order.

## Parameters

**other** (`const Thread&`)

The values to intersect with.

## Returns

`Thread`
    The distinct case-label values also found in `other`, in first-occurrence
    order, `display_name` `"intersect"`.

## Examples

Example 1. Intersect case labels with a repeat against a target set.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Thread(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.label().set("region");

auto other = Thread(pluss::make_note({"west", "south", "north"}));
auto out = labeled.label().intersect(other);
std::cout << out;
```

### Output

```text
0  west
1  north
Name: intersect, ptype: Note
```

`"east"` doesn't appear in `other`, so it's excluded even though it's the case
labels' most frequent value; `"west"` and `"north"` are shared, in the order
they first appear on the left side.

## Notes

- Symmetric in *content* but not in *order*: `a.intersect(b)` and
  `b.intersect(a)` (thought of as sets) contain the same values, ordered by
  whichever side's first-occurrence order you called it from.
- Duplicates on either side are collapsed; the result never repeats a value.

## See Also

- [`DataFrameLabelAccessor::unite`](unite.md),
  [`DataFrameLabelAccessor::diff`](diff.md),
  [`DataFrameLabelAccessor::symdiff`](symdiff.md): the other set-algebra
  operations over case-label values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="isnull.html">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>
