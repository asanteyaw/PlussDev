<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0055432d4f89 -->

# DataFrameTsAccessor::mul

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameTsAccessor::mul(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const
```

## What It Does

Multiplies every bound feature by `other`, first aligning `other`'s rows onto this
accessor's own time axis according to `mode` — the exact same alignment model as
[`add()`](add.md), with multiplication in place of addition. For each bound feature
name, `other` must have a same-named feature; that feature's values are aligned
onto this accessor's axis (producing `NULL` wherever no match is found), then
multiplied elementwise with the bound feature's own values.

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
    each feature replaced by its product with `other`'s aligned values.

## Examples

Example 1. Multiply two frames whose time axes don't fully overlap, using the
default `Exact` alignment.

```cpp
using namespace pluss::table;

auto left = DataFrame(
    {{"sales", Slate(pluss::make_series({100.0, 120.0, 90.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));

auto right = DataFrame(
    {{"sales", Slate(pluss::make_series({10.0, 5.0, 3.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"})));

auto out = left.ts().mul(right);
std::cout << out;
```

### Output

```text
==================
           | sales
------------------
2024-01-01 | 1000 
2024-01-02 | 600  
2024-01-05 | NULL 
------------------
[3 cs x 1 feat]
```

`2024-01-01` and `2024-01-02` exist on both sides, so they multiply normally
(`100 * 10 = 1000`, `120 * 5 = 600`). `2024-01-05` has no exact match on `right`
(which only reaches `2024-01-03`), so with the default `Exact` mode it's `NULL`.

Example 2. The same two frames, using `Backward` instead of `Exact` — `2024-01-05`
now picks up `right`'s most recent prior value instead of `NULL`.

```cpp
auto out_backward = left.ts().mul(right, TsAlignMode::Backward);
std::cout << out_backward;
```

### Output

```text
==================
           | sales
------------------
2024-01-01 | 1000 
2024-01-02 | 600  
2024-01-05 | 270  
------------------
[3 cs x 1 feat]
```

`2024-01-01` and `2024-01-02` multiply exactly as before. `2024-01-05` has no
exact match on `right`, but `Backward` carries forward `right`'s most recent
prior value, `2024-01-03`'s `3` — so `90 * 3 = 270` instead of `NULL`.

Example 3. `other` is missing a same-named feature, to see the call throw.

```cpp
auto other_missing = DataFrame(
    {{"units", Slate(pluss::make_series({1.0, 2.0, 3.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"})));

try {
  auto out = left.ts().mul(other_missing);
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
there's nothing on the right-hand side to align and multiply, so it throws
rather than treating the missing feature as all-`NULL`.

## Notes

- The result's case labels are always this accessor's own axis — see
  [`add()`](add.md) for the full notes on alignment and matching behavior, which
  apply identically here.
- Every bound feature must have a same-named counterpart on `other`, or the call
  throws.

## See Also

- [`DataFrameTsAccessor::add`](add.md), [`DataFrameTsAccessor::sub`](sub.md),
  [`DataFrameTsAccessor::div`](div.md): the other elementwise arithmetic operators
  sharing this same alignment model.
- [`DataFrameTsAccessor::align_to`](align_to.md): the lower-level reindex primitive
  this method's alignment step is built from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lt.html">
    <span class="label">Previous</span>
    <span class="title">lt</span>
  </a>
  <a class="next-link" href="ne.html">
    <span class="label">Next</span>
    <span class="title">ne</span>
  </a>
</div>
