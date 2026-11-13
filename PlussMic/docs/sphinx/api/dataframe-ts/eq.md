<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 11624c6e659d -->

# DataFrameTsAccessor::eq

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameTsAccessor::eq(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const
```

## What It Does

Compares every bound feature against `other` for equality, first aligning `other`'s
rows onto this accessor's own time axis according to `mode` — the same alignment
model used by [`add()`](add.md), [`sub()`](sub.md), [`mul()`](mul.md), and
[`div()`](div.md), just with an equality comparison instead of an arithmetic
operator. For each bound feature name, `other` must have a same-named feature; that
feature's values are aligned onto this accessor's axis (producing `NULL` wherever no
match is found), then compared elementwise for equality with the bound feature's own
values.

## Parameters

**other** (`const DataFrame&`)

The right-hand frame. Must contain a feature with the same name as every feature
currently bound on this accessor.

**mode** (`TsAlignMode`, default `Exact`)

How `other`'s rows are matched onto this accessor's time axis:

- **`Exact`**: only rows whose axis tick matches exactly are paired; everything
  else is `NULL`.
- **`Backward`**: each row is paired with `other`'s most recent value at or before
  its own tick.
- **`Forward`**: each row is paired with `other`'s next value at or after its own
  tick.
- **`Nearest`**: each row is paired with whichever of `other`'s values is closest in
  time, before or after.

## Returns

`DataFrame`
    A new frame, same shape and case labels as this accessor's bound features, with
    each feature replaced by a `Bool` column: `true` where the aligned values are
    equal, `false` where they differ, `NULL` where no aligned value exists.

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

auto out = left.ts().eq(right);
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

`2024-01-01` matches exactly on both sides (`100 == 100` → `true`). `2024-01-02`
exists on both sides but the values differ (`120 != 5` → `false`). `2024-01-05` has
no exact match on `right` (which only reaches `2024-01-03`), so with the default
`Exact` mode it's `NULL` rather than `false` — a missing comparison is not the same
as an unequal one.

Example 2. `other` is missing a same-named feature, to see the call throw.

```cpp
auto other_missing = DataFrame(
    {{"units", Slate(pluss::make_series({1.0, 2.0, 3.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"})));

try {
  auto out = left.ts().eq(other_missing);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrameTsAccessor::binary: right frame is missing feature 'sales'
```

## Notes

- The result's case labels are always this accessor's own axis — `other`'s rows
  that don't get matched (under `Exact`) or contribute (under
  `Backward`/`Forward`/`Nearest`) never introduce new rows.
- An unmatched row compares to `NULL`, not `false` — absence of a comparable value
  is distinct from the values being unequal.
- Every bound feature must have a same-named counterpart on `other`, or the call
  throws the same `"DataFrameTsAccessor::binary: right frame is missing feature
  '<name>'"` message shared by every arithmetic and comparison overload in this
  family.

## See Also

- [`DataFrameTsAccessor::ne`](ne.md), [`DataFrameTsAccessor::lt`](lt.md),
  [`DataFrameTsAccessor::le`](le.md), [`DataFrameTsAccessor::gt`](gt.md),
  [`DataFrameTsAccessor::ge`](ge.md): the other time-aligned comparisons sharing this
  same alignment model.
- [`DataFrameTsAccessor::add`](add.md): the arithmetic sibling built on the exact
  same alignment mechanism.
- [`DataFrameTsAccessor::align_to`](align_to.md): the lower-level reindex primitive
  this method's alignment step is built from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="div.html">
    <span class="label">Previous</span>
    <span class="title">div</span>
  </a>
  <a class="next-link" href="ewm.html">
    <span class="label">Next</span>
    <span class="title">ewm</span>
  </a>
</div>
