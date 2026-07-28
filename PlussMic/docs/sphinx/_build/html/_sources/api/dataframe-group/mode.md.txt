<!-- pluss-hand-authored -->

# DataFrameGrouped::mode

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signature

```cpp
DataFrameGrouped::mode() const -> DataFrame
```

## What It Does

Computes grouped mode for each non-key value feature.

This is the grouped “most common value inside each group” terminal. It is especially
useful for categorical/value-coded features such as status, region code, product
family, or segment.

## Example

```cpp
using namespace pluss::table;

auto loans = DataFrame({
    {"region", Thread(pluss::make_note({"North", "North", "South", "South"}))},
    {"status", Thread(pluss::make_note({"good", "good", "bad", "good"}))},
});

auto out = loans.group()
                .by({"region"})
                .mode();
```

## Notes

- Use [`agg()`](agg.md) instead if you want grouped mode beside numeric grouped
  outputs in the same result with explicit names.
- Mode is often the most natural grouped terminal for note/categorical features,
  whereas `mean()`/`std()`/`var()` are more naturally numeric.

## See Also

- [`DataFrameGrouped::median`](median.md)
- [`DataFrameGrouped::agg`](agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="var.html">
    <span class="label">Previous</span>
    <span class="title">var</span>
  </a>
</div>
