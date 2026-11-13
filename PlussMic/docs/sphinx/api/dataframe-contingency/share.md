<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a2b32cdeb6d2 -->

# ContingencyTable::share

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signature

```cpp
ContingencyTable ContingencyTable::share() const
```

## What It Does

Returns the same retained table in global-share space.

Every cell is divided by the grand total across the current retained table, so
the full rendered table sums to `1.0` when the table is non-empty.

## Returns

`ContingencyTable`
    A new retained table whose values are global proportions instead of counts.

## When To Use It

Use `share()` when you want each cell interpreted as "share of the current whole"
rather than "count within the current whole."

## Examples

Example 1. Convert a two-way count table into global shares.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
});

auto out = df.contingency("city", "channel").share().to_frame();
std::cout << out;
```

### Output

```text
=======================
     | city  Card  Cash
-----------------------
0    | A     0.2   0.2
1    | B     0.2   0.4
-----------------------
[2 cs x 3 feat]
```

Example 2. `share()` normalizes the current retained table, not necessarily the
original unsliced one.

```cpp
auto df = DataFrame({
    {"city", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Slate(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
});

auto out = df.contingency({"city", "channel", "weekend"})
             .slice({{"weekend", "yes"}})
             .share()
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

- `share()` normalizes the **current retained table**. If you slice or marginalize
  first, the denominator changes accordingly.
- `share()` is different from `condition(...)`: global share asks "share of all cells";
  conditional share asks "share within these retained parent dimensions."
- Empty or all-zero retained tables render as zeros rather than undefined values.
