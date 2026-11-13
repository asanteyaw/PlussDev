<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9a8e6ed62634 -->

# DataFrameTsAccessor::ne

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameTsAccessor::ne(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const
```

## What It Does

Compares every bound feature against `other` for inequality, first aligning `other`'s
rows onto this accessor's own time axis according to `mode` — the exact negation of
[`eq()`](eq.md), sharing the same alignment model as
[`add()`](add.md)/[`sub()`](sub.md)/[`mul()`](mul.md)/[`div()`](div.md). For each
bound feature name, `other` must have a same-named feature; that feature's values are
aligned onto this accessor's axis (producing `NULL` wherever no match is found), then
compared elementwise for inequality with the bound feature's own values.

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
    each feature replaced by a `Bool` column: `true` where the aligned values differ,
    `false` where they're equal, `NULL` where no aligned value exists.

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

auto out = left.ts().ne(right);
std::cout << out;
```

### Output

```text
==================
           | sales
------------------
2024-01-01 | false
2024-01-02 | true 
2024-01-05 | NULL 
------------------
[3 cs x 1 feat]
```

`2024-01-01` matches exactly on both sides (`100 == 100` → `false`, they are *not*
unequal). `2024-01-02` differs (`120 != 5` → `true`). `2024-01-05` has no exact
match on `right`, so it's `NULL` rather than `true` — an unmatched row is neither
equal nor unequal.

## Notes

- `ne()` is the exact negation of [`eq()`](eq.md) wherever both sides have an
  aligned value; an unmatched row is `NULL` under both, never flipped to the
  opposite boolean.
- Every bound feature must have a same-named counterpart on `other`, or the call
  throws `"DataFrameTsAccessor::binary: right frame is missing feature '<name>'"`.

## See Also

- [`DataFrameTsAccessor::eq`](eq.md): the equality counterpart this method negates.
- [`DataFrameTsAccessor::lt`](lt.md), [`DataFrameTsAccessor::le`](le.md),
  [`DataFrameTsAccessor::gt`](gt.md), [`DataFrameTsAccessor::ge`](ge.md): the other
  time-aligned comparisons sharing this same alignment model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mul.html">
    <span class="label">Previous</span>
    <span class="title">mul</span>
  </a>
  <a class="next-link" href="resample.html">
    <span class="label">Next</span>
    <span class="title">resample</span>
  </a>
</div>
