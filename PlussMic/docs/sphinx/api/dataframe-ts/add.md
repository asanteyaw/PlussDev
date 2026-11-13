<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b47738d75683 -->

# DataFrameTsAccessor::add

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameTsAccessor::add(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const
```

## What It Does

Adds `other` to every bound feature, first aligning `other`'s rows onto this
accessor's own time axis according to `mode`. For each bound feature name, `other`
must have a same-named feature; that feature's values are aligned onto this
accessor's axis (producing `NULL` wherever no match is found), then added
elementwise to the bound feature's own values.

## Parameters

**other** (`const DataFrame&`)

The right-hand frame. Must contain a feature with the same name as every feature
currently bound on this accessor.

**mode** (`TsAlignMode`, default `Exact`)

How `other`'s rows are matched onto this accessor's time axis:

- **`Exact`**: only rows whose axis tick matches exactly are paired; everything
  else is `NULL`.
- **`Backward`**: each row is paired with `other`'s most recent value at or before
  its own tick (a "carry the last known value forward" match).
- **`Forward`**: each row is paired with `other`'s next value at or after its own
  tick.
- **`Nearest`**: each row is paired with whichever of `other`'s values is closest in
  time, before or after.

## Returns

`DataFrame`
    A new frame, same shape and case labels as this accessor's bound features, with
    each feature replaced by its sum with `other`'s aligned values.

## Examples

Example 1. Add two frames whose time axes don't fully overlap, using the default
`Exact` alignment.

```cpp
using namespace pluss::table;

auto left = DataFrame(
    {{"sales", Slate(pluss::make_series({100.0, 120.0, 90.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));
std::cout << left;

auto right = DataFrame(
    {{"sales", Slate(pluss::make_series({10.0, 5.0, 3.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"})));
std::cout << right;
```

### Output

```text
==================
           | sales
------------------
2024-01-01 | 100  
2024-01-02 | 120  
2024-01-05 | 90   
------------------
[3 cs x 1 feat]
==================
           | sales
------------------
2024-01-01 | 10   
2024-01-02 | 5    
2024-01-03 | 3    
------------------
[3 cs x 1 feat]
```

```cpp
auto out = left.ts().add(right);
std::cout << out;
```

### Output

```text
==================
           | sales
------------------
2024-01-01 | 110  
2024-01-02 | 125  
2024-01-05 | NULL 
------------------
[3 cs x 1 feat]
```

`2024-01-01` and `2024-01-02` exist on both sides, so they add normally
(`100 + 10 = 110`, `120 + 5 = 125`). `2024-01-05` has no exact match on `right`
(which only reaches `2024-01-03`), so with the default `Exact` mode it's `NULL` —
`right`'s own `2024-01-03` value is simply never considered, since `Exact`
performs no time-proximity matching at all.

Example 2. The same two frames, using `Backward` instead of `Exact` — `2024-01-05`
now picks up `right`'s most recent prior value instead of `NULL`.

```cpp
auto out_backward = left.ts().add(right, TsAlignMode::Backward);
std::cout << out_backward;
```

### Output

```text
==================
           | sales
------------------
2024-01-01 | 110  
2024-01-02 | 125  
2024-01-05 | 93   
------------------
[3 cs x 1 feat]
```

`2024-01-01` and `2024-01-02` add exactly as before. `2024-01-05` has no exact
match on `right`, but `Backward` carries forward `right`'s most recent prior
value, `2024-01-03`'s `3` — so `90 + 3 = 93` instead of `NULL`.

Example 3. `other` is missing a same-named feature, to see the call throw.

```cpp
using namespace pluss::table;

auto other_missing = DataFrame(
    {{"units", Slate(pluss::make_series({1.0, 2.0, 3.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z"})));

try {
  auto out = left.ts().add(other_missing);
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
there's nothing on the right-hand side to align and add, so it throws rather
than treating the missing feature as all-`NULL`.

## Notes

- The result's case labels are always this accessor's own axis — `other`'s rows
  that don't get matched (under `Exact`) or contribute (under
  `Backward`/`Forward`/`Nearest`) never introduce new rows.
- Every bound feature must have a same-named counterpart on `other`, or the call
  throws.
- `Exact` and `Backward` are shown fully worked above; `Forward`/`Nearest` follow
  the exact same asof-style matching documented on [`align_to()`](align_to.md).

## See Also

- [`DataFrameTsAccessor::sub`](sub.md), [`DataFrameTsAccessor::mul`](mul.md),
  [`DataFrameTsAccessor::div`](div.md): the other elementwise arithmetic operators
  sharing this same alignment model.
- [`DataFrameTsAccessor::align_to`](align_to.md): the lower-level reindex primitive
  this method's alignment step is built from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="align_to.html">
    <span class="label">Next</span>
    <span class="title">align_to</span>
  </a>
</div>
