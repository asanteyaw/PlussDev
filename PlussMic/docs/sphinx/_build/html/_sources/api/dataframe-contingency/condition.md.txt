<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4e1052070763 -->

# ContingencyTable::condition

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signatures

```cpp
ContingencyTable::condition(std::string_view dim) const -> ContingencyTable
ContingencyTable::condition(std::initializer_list<std::string_view> dims) const -> ContingencyTable
ContingencyTable::condition(pluss::msharp::RangeView::Of<std::string_view> dims) const -> ContingencyTable
```

## What It Does

Turns the retained count table into a conditional-share table with respect to
the supplied dimensions.

More precisely: the supplied dimensions are the dimensions you want the result to sum
to `1` within. Every other retained dimension is summed out of the denominator during
normalization, but remains available for rendering.

That makes `condition(...)` the targeted normalization surface:

- `share()` -> divide by the grand total of the current retained table
- `condition({"city"})` -> divide within each `city`
- `condition({"city", "weekend"})` -> divide within each `(city, weekend)` cell family

## Parameters

**dim / dims**

One or more retained dimension names already present on the `ContingencyTable`.

## Returns

`ContingencyTable`
    A new retained table with the same dimensions and levels, but with conditional
    shares materialized at render/export time instead of raw counts.

## When To Use It

Use `condition(...)` when the question is not "what share of the whole table is this?"
but rather "what share within this parent slice is this?"

Typical interpretations:

- `condition({"city"})` -> channel mix within each city
- `condition({"channel"})` -> city composition within each channel
- `condition({"city", "weekend"})` -> channel mix within each city/weekend slice

## Examples

Example 1. Condition on the row dimension.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
});

auto out = df.contingency("city", "channel")
             .condition({"city"})
             .to_frame();
std::cout << out;
```

### Output

```text
=============================
     | city  Card      Cash
-----------------------------
0    | A     0.5       0.5
1    | B     0.333333  0.666667
-----------------------------
[2 cs x 3 feat]
```

Example 2. Condition on the column dimension instead.

```cpp
auto out = df.contingency("city", "channel")
             .condition({"channel"})
             .to_frame();
std::cout << out;
```

### Output

```text
=============================
     | city  Card  Cash
-----------------------------
0    | A     0.5   0.333333
1    | B     0.5   0.666667
-----------------------------
[2 cs x 3 feat]
```

Example 3. Condition after slicing to normalize within the remaining subset
rather than the original whole table.

```cpp
auto df = DataFrame({
    {"city", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Slate(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
});

auto out = df.contingency({"city", "channel", "weekend"})
             .slice({{"weekend", "yes"}})
             .condition({"city"})
             .to_frame({"city"}, {"channel"});
std::cout << out;
```

### Output

```text
=======================
     | city  Card  Cash
-----------------------
0    | A     0     0
1    | B     0.5   0.5
-----------------------
[2 cs x 3 feat]
```

## Notes

- `condition(...)` does not remove dimensions. It only changes how values are
  normalized.
- Conditioning is applied to the **current retained table**. If you slice or
  marginalize first, the conditional shares are computed on that transformed object,
  not on the original unsliced contingency.
- Duplicate dimension names are rejected.
- Unknown dimension names are rejected.
- Zero-denominator slices render as `0` rather than producing `NaN` cells. That is why
  city `A` above shows `0, 0` in the sliced weekend-only example instead of undefined
  values.
