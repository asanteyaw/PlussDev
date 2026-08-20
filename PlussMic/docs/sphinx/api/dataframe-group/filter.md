<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9732d3cd8128 -->

# DataFrameGrouped::filter

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## Signatures

```cpp
DataFrameGrouped::filter(const pluss::Series& keep) const -> DataFrame
DataFrameGrouped::filter(const Thread& keep) const -> DataFrame
```

## What It Does

Keeps or drops **whole groups** from the original source frame.

This is not row-wise filtering. `keep` is interpreted at the **group level**:
one boolean per group, in grouped order. If a group's slot is `true`, every row in
that group survives. If it is `false`, every row in that group is removed.

## Parameters

**keep** (`Series` or `Thread`)

A boolean grouped mask with one element per group.

## Returns

`DataFrame`
    A filtered version of the original frame, preserving original row order among
    surviving rows.

## Examples

Example 1. Keep only regions whose grouped total is above a threshold.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"region", Thread(pluss::make_note({"North", "North", "South", "South"}))},
    {"amount", Thread(pluss::make_series({100.0, 250.0, 80.0, 120.0}))},
});

auto totals = sales.group()
                  .by({"region"})
                  .agg({
                      {"amount_sum", "amount", DataFrame::GroupOp::Sum},
                  });

auto keep = totals.features("amount_sum").gt(200.0);
auto out = sales.group().by({"region"}).filter(keep);
std::cout << out;
```

### Output

```text
=====================
     | region  amount
---------------------
0    | North   100   
1    | North   250   
---------------------
[2 cs x 2 feat]
```

`totals` puts `"North"` (sum `350`) ahead of `"South"` (sum `200`), so `keep` is
`[true, false]` in that same grouped order. `North`'s sum is strictly greater than
`200`, so both of its rows survive; `South`'s sum is exactly `200`, which fails a
strict `gt(200.0)`, so both `South` rows are dropped entirely. The important contract
is that `keep` was produced in the same grouped order as the handle being filtered.

## Notes

- `filter(...)` expects **group count**, not row count.
- The safest pattern is: compute grouped summary from the same grouped handle shape,
  derive a boolean mask from that summary, then pass it to `filter(...)`.
- Returned rows keep their original order and original case labels from the source
  frame — filtering never renumbers or reindexes survivors.

## See Also

- [`DataFrame::filter`](../dataframe/filter.md): row-wise filtering, not group-wise.
- [`DataFrameGrouped::agg`](agg.md): often used immediately before deriving `keep`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transform.html">
    <span class="label">Previous</span>
    <span class="title">transform</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
