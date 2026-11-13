<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1e87b63c48da -->

# DataFrameLabelAccessor::diff

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
Slate DataFrameLabelAccessor::diff(const Slate& other) const
```

## What It Does

Treats the case labels and `other` as two sets of values, and returns the
labels that appear in this accessor's case labels but **not** in `other` (a
set difference: "mine minus yours") — deduplicated, in first-occurrence order.
This is not the numeric row-to-row difference some other `diff` methods in the
library compute; it's a set operation over label *values*.

## Parameters

**other** (`const Slate&`)

The values to subtract. Can be any `Slate`, not necessarily another frame's
case labels.

## Returns

`Slate`
    The distinct case-label values with no match in `other`, in first-occurrence
    order, `display_name` `"diff"`.

## Examples

Example 1. Subtract a set of values from a case-label slate with a repeat.

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "west", "east", "north"}))},
     {"sales",  Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))}});
auto labeled = orders.labels().set("region");

auto other = Slate(pluss::make_note({"west", "north"}));
auto out = labeled.labels().diff(other);
std::cout << out;
```

### Output

```text
0  east
Name: diff, ptype: Note
```

The case labels hold three distinct values (`"east"`, `"west"`, `"north"`,
ignoring the repeated `"east"`); removing everything found in `other`
(`"west"`, `"north"`) leaves just `"east"`.

## Notes

- Order is `left minus right`: swap the operands and you generally get a
  different (and possibly non-empty where this was empty) result — unlike
  [`symdiff()`](symdiff.md), which is symmetric.
- Duplicates within either operand are collapsed; the result never repeats a
  value.
- When the two operands share a common representable dtype (same `Series`
  scalar type, both numeric, or the same non-`Series` kind on the same
  platform), the result preserves that dtype; otherwise it falls back to a
  `Note` of each value's string form.

## See Also

- [`DataFrameLabelAccessor::symdiff`](symdiff.md): values in exactly one of the
  two sets (symmetric difference).
- [`DataFrameLabelAccessor::unite`](unite.md),
  [`DataFrameLabelAccessor::intersect`](intersect.md): the other set-algebra
  operations over case-label values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="depth.html">
    <span class="label">Previous</span>
    <span class="title">depth</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
