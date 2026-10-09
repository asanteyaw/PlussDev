<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3d8adffab0e0 -->

# DataFrameTsAccessor::lt

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::lt(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const -> DataFrame
```

## What It Does

Checks whether every bound feature is strictly less than `other`, first aligning
`other`'s rows onto this accessor's own time axis according to `mode` — the same
alignment model shared by [`eq()`](eq.md) and the arithmetic operators. For each
bound feature name, `other` must have a same-named feature; that feature's values are
aligned onto this accessor's axis (producing `NULL` wherever no match is found), then
compared elementwise with `<` against the bound feature's own values.

## Parameters

**other** (`const DataFrame&`)

The right-hand frame. Must contain a feature with the same name as every feature
currently bound on this accessor.

**mode** (`TsAlignMode`, default `Exact`)

How `other`'s rows are matched onto this accessor's time axis — see [`eq()`](eq.md)
for the full description of `Exact`/`Backward`/`Forward`/`Nearest`.

## Returns

`DataFrame`
    A new frame, same shape and case labels as this accessor's bound features, with
    each feature replaced by a `Bool` column: `true` where the bound value is
    strictly less than the aligned `other` value, `false` otherwise, `NULL` where no
    aligned value exists.

## Examples

Example 1. Compare two frames whose time axes don't fully overlap, using the default
`Exact` alignment.

```cpp
using namespace pluss::table;

auto left = DataFrame(
    {{"sales", Slate(pluss::make_series({100.0, 120.0, 90.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));

auto right = DataFrame(
    {{"sales", Slate(pluss::make_series({100.0, 5.0, 3.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"})));

auto out = left.ts().lt(right);
std::cout << out;
```

### Output

```text
==================
           | sales
------------------
2024-01-01 | false
2024-01-02 | false
2024-01-05 | NULL 
------------------
[3 cs x 1 feat]
```

`2024-01-01` is `100 < 100` → `false` (equal is not strictly less). `2024-01-02` is
`120 < 5` → `false`. `2024-01-05` has no exact match on `right`, so it's `NULL`
rather than `false` — an unmatched row is neither less than nor not-less-than.

## Notes

- Equal values compare `false` under `lt()`, the same as any strict-inequality
  operator — use [`le()`](le.md) if the boundary should count.
- Every bound feature must have a same-named counterpart on `other`, or the call
  throws `"DataFrameTsAccessor::binary: right frame is missing feature '<name>'"`.

## See Also

- [`DataFrameTsAccessor::le`](le.md): the inclusive counterpart (`<=`).
- [`DataFrameTsAccessor::gt`](gt.md), [`DataFrameTsAccessor::ge`](ge.md): the
  opposite-direction comparisons.
- [`DataFrameTsAccessor::eq`](eq.md), [`DataFrameTsAccessor::ne`](ne.md): the
  equality-based comparisons sharing this same alignment model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log_returns.html">
    <span class="label">Previous</span>
    <span class="title">log_returns</span>
  </a>
  <a class="next-link" href="mul.html">
    <span class="label">Next</span>
    <span class="title">mul</span>
  </a>
</div>
