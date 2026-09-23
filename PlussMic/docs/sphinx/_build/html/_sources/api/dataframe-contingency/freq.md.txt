<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4ea09bb6d7ec -->

# ContingencyTable::freq

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signature

```cpp
ContingencyTable::freq() const -> ContingencyTable
```

## What It Does

Returns the same retained table in raw frequency space.

`freq()` does not rebuild the contingency table. It simply clears any active
share/conditioning state and keeps the same retained dimensions, levels, and
default render hints.

## Returns

`ContingencyTable`
    A new retained table whose values are raw counts again.

## When To Use It

Use `freq()` when you have already moved into `share()` or `condition(...)` space and
want to render or export the original count interpretation again without rebuilding the
contingency object from the source frame.

## Examples

Example 1. Convert a global-share table back to raw counts before rendering.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
});

auto out = df.contingency("city", "channel")
             .share()
             .freq()
             .to_frame();
std::cout << out;
```

### Output

```text
=======================
     | city  Card  Cash
-----------------------
0    | A     1     1
1    | B     1     2
-----------------------
[2 cs x 3 feat]
```

## Notes

- `freq()` clears both global-share and conditional-share state.
- It does not change dimensions, levels, default row/column hints, or render options.
- Because the underlying count tensor was already retained, `freq()` is a cheap state
  switch, not a recomputation.

Example 2. `freq()` also clears conditional normalization.

```cpp
auto out = df.contingency("city", "channel")
             .condition({"city"})
             .freq()
             .to_frame();
std::cout << out;
```

### Output

```text
=======================
     | city  Card  Cash
-----------------------
0    | A     1     1
1    | B     1     2
-----------------------
[2 cs x 3 feat]
```
