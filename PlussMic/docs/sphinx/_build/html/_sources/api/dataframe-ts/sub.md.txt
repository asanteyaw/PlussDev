<!-- pluss-hand-authored -->

# DataFrameTsAccessor::sub

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::sub(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const -> DataFrame
```

## What It Does

Subtracts `other` from every bound feature, first aligning `other`'s rows onto
this accessor's own time axis according to `mode` — the exact same alignment model
as [`add()`](add.md), with subtraction in place of addition. For each bound feature
name, `other` must have a same-named feature; that feature's values are aligned
onto this accessor's axis (producing `NULL` wherever no match is found), then
subtracted elementwise from the bound feature's own values.

## Parameters

**other** (`const DataFrame&`)

The right-hand frame. Must contain a feature with the same name as every feature
currently bound on this accessor.

**mode** (`TsAlignMode`, default `Exact`)

How `other`'s rows are matched onto this accessor's time axis — see
[`add()`](add.md) for the full description of `Exact`/`Backward`/`Forward`/`Nearest`.

## Returns

`DataFrame`
    A new frame, same shape and case labels as this accessor's bound features, with
    each feature replaced by its difference against `other`'s aligned values.

## Examples

Example 1. Subtract two frames whose time axes don't fully overlap, using the
default `Exact` alignment.

```cpp
using namespace pluss::table;

auto left = DataFrame(
    {{"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));

auto right = DataFrame(
    {{"sales", Thread(pluss::make_series({10.0, 5.0, 3.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"})));

auto out = left.ts().sub(right);
std::cout << out;
```

### Output

```text
==================
           | sales
------------------
2024-01-01 | 90   
2024-01-02 | 115  
2024-01-05 | NULL 
------------------
[3 cs x 1 feat]
```

`2024-01-01` and `2024-01-02` exist on both sides, so they subtract normally
(`100 - 10 = 90`, `120 - 5 = 115`). `2024-01-05` has no exact match on `right`
(which only reaches `2024-01-03`), so with the default `Exact` mode it's `NULL`.

Example 2. The same two frames, using `Backward` instead of `Exact` — `2024-01-05`
now picks up `right`'s most recent prior value instead of `NULL`.

```cpp
auto out_backward = left.ts().sub(right, TsAlignMode::Backward);
std::cout << out_backward;
```

### Output

```text
==================
           | sales
------------------
2024-01-01 | 90   
2024-01-02 | 115  
2024-01-05 | 87   
------------------
[3 cs x 1 feat]
```

`2024-01-01` and `2024-01-02` subtract exactly as before. `2024-01-05` has no
exact match on `right`, but `Backward` carries forward `right`'s most recent
prior value, `2024-01-03`'s `3` — so `90 - 3 = 87` instead of `NULL`.

Example 3. `other` is missing a same-named feature, to see the call throw.

```cpp
auto other_missing = DataFrame(
    {{"units", Thread(pluss::make_series({1.0, 2.0, 3.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"})));

try {
  auto out = left.ts().sub(other_missing);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrameTsAccessor::binary: right frame is missing feature 'sales'
```

`left` has a bound `"sales"` feature, but `other_missing` only has `"units"` —
there's nothing on the right-hand side to align and subtract, so it throws
rather than treating the missing feature as all-`NULL`.

## Notes

- The result's case labels are always this accessor's own axis — see
  [`add()`](add.md) for the full notes on alignment and matching behavior, which
  apply identically here.
- Every bound feature must have a same-named counterpart on `other`, or the call
  throws.

## See Also

- [`DataFrameTsAccessor::add`](add.md), [`DataFrameTsAccessor::mul`](mul.md),
  [`DataFrameTsAccessor::div`](div.md): the other elementwise arithmetic operators
  sharing this same alignment model.
- [`DataFrameTsAccessor::align_to`](align_to.md): the lower-level reindex primitive
  this method's alignment step is built from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="timed.html">
    <span class="label">Next</span>
    <span class="title">timed</span>
  </a>
</div>
