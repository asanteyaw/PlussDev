<!-- pluss-hand-authored -->

# DataFrameTsAccessor::le

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::le(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const -> DataFrame
```

## What It Does

Checks whether every bound feature is less than or equal to `other`, first aligning
`other`'s rows onto this accessor's own time axis according to `mode` — the inclusive
counterpart of [`lt()`](lt.md), sharing the same alignment model. For each bound
feature name, `other` must have a same-named feature; that feature's values are
aligned onto this accessor's axis (producing `NULL` wherever no match is found), then
compared elementwise with `<=` against the bound feature's own values.

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
    each feature replaced by a `Bool` column: `true` where the bound value is less
    than or equal to the aligned `other` value, `false` otherwise, `NULL` where no
    aligned value exists.

## Examples

Example 1. Compare two frames whose time axes don't fully overlap, using the default
`Exact` alignment.

```cpp
using namespace pluss::table;

auto left = DataFrame(
    {{"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));

auto right = DataFrame(
    {{"sales", Thread(pluss::make_series({100.0, 5.0, 3.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"})));

auto out = left.ts().le(right);
std::cout << out;
```

### Output

```text
==================
           | sales
------------------
2024-01-01 | true 
2024-01-02 | false
2024-01-05 | NULL 
------------------
[3 cs x 1 feat]
```

`2024-01-01` is `100 <= 100` → `true` (equal counts here, unlike [`lt()`](lt.md)).
`2024-01-02` is `120 <= 5` → `false`. `2024-01-05` has no exact match on `right`, so
it's `NULL` rather than `false`.

## Notes

- Equal values compare `true` under `le()`, unlike [`lt()`](lt.md)'s strict `<`.
- Every bound feature must have a same-named counterpart on `other`, or the call
  throws `"DataFrameTsAccessor::binary: right frame is missing feature '<name>'"`.

## See Also

- [`DataFrameTsAccessor::lt`](lt.md): the strict counterpart (`<`).
- [`DataFrameTsAccessor::gt`](gt.md), [`DataFrameTsAccessor::ge`](ge.md): the
  opposite-direction comparisons.
- [`DataFrameTsAccessor::eq`](eq.md), [`DataFrameTsAccessor::ne`](ne.md): the
  equality-based comparisons sharing this same alignment model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lag_block.html">
    <span class="label">Previous</span>
    <span class="title">lag_block</span>
  </a>
  <a class="next-link" href="lead.html">
    <span class="label">Next</span>
    <span class="title">lead</span>
  </a>
</div>
