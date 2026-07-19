<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dc2f0967b7c8 -->

# DataFrame::all

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::all(pluss::RangeView<std::string_view> features = {}, bool skipnull = true) const -> Thread
DataFrame::all(Axis axis, pluss::RangeView<std::string_view> features = {}, bool skipnull = true) const -> Thread
```

## What It Does

Reports whether **every** value is `true` along the requested axis, over a set of
`Bool` `Series` features — the counterpart to [`any()`](any.md), which only requires
one. Every feature it reduces over must already be a `Bool` `Series`; if `features` is
left empty, it defaults to *every* column in the frame, so a frame that mixes boolean
flags with other feature types must pass `features` explicitly to select only the
boolean ones.

The two overloads differ only in whether you supply an axis: the short overload always
reduces `Axis::Case` (down each feature's cases); the longer overload lets you choose
`Axis::Feature` instead (across each row's selected features).

## Parameters

**features** (`pluss::RangeView<std::string_view>`, default `{}`)

Which `Bool` `Series` features to reduce over. An empty range (the default) selects
every column in the frame — which only works if every column actually is a `Bool`
`Series`; otherwise, pass the specific boolean feature names you want.

**axis** (`Axis`, default `Axis::Case` when omitted)

Reduction direction. `Axis::Case` reduces down each feature's cases, producing one
`true`/`false` per feature — "was this flag true on every single row?" `Axis::Feature`
reduces across each row's selected features instead, producing one `true`/`false` per
case — "does this row satisfy every selected flag?", a common eligibility-gate shape.

**skipnull** (`bool`, default `true`)

Controls how a missing value affects the reduction's validity. A missing cell is
always treated as `true` while computing the `true`/`false` value itself — a null can
never *cause* an `all()` result to be `false` on its own, since it's excluded rather
than counted as a failing observation. What `skipnull` changes is whether that
computed result is trusted:

- `true` (the default): the result is reported as-is, regardless of any nulls present.
- `false`: if any cell that participated in a reduction was missing, that reduction's
  result comes back `NULL` instead of the computed `true`/`false`.

## Returns

`Thread`
    One `Bool` entry per participating feature (`Axis::Case`, the default) or per case
    (`Axis::Feature`), holding whether every value was `true`.

## Examples

Example 1. Check whether each flag was true on every case, the default axis.

```cpp
using namespace pluss::table;

auto checks = DataFrame({
    {"order_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"in_stock", Thread(pluss::make_series({true, true, false, true, false, true}, pluss::ScalarType::Bool))},
    {"paid",     Thread(pluss::make_series({true, false, false, true, false, true}, pluss::ScalarType::Bool))},
});

auto always_true = checks.all({"in_stock", "paid"});
std::cout << always_true;
```

### Output

```text
in_stock  false
    paid  false
ptype: Series
```

Both flags have at least one `false` case (`in_stock` at cases `2` and `4`; `paid` at
cases `1`, `2`, and `4`), so both come back `false`. `order_id` was deliberately left
out of `features` — passing it would fail, because `all()` requires every selected
feature to already be a `Bool` `Series`.

Example 2. Use `Axis::Feature` to ask, per row, whether *both* flags are set — a
"ready to ship" eligibility gate.

```cpp
auto ready_to_ship = checks.all(Axis::Feature, {"in_stock", "paid"});
std::cout << ready_to_ship;
```

### Output

```text
0  true
1  false
2  false
3  true
4  false
5  true
ptype: Series
```

Cases `0`, `3`, and `5` are `true` because both `in_stock` and `paid` are `true` for
those orders. Case `1` is `false` because `paid` is `false`; cases `2` and `4` are
`false` because neither flag is `true`.

Example 3. See how `skipnull` affects a reduction that includes a missing value.

```cpp
auto priority_valid = pluss::make_series({true, true, false, true}, pluss::ScalarType::Bool);
auto flags = DataFrame({
    {"is_priority", Thread(pluss::make_series({true, false, false, false}, pluss::ScalarType::Bool), priority_valid)},
});

auto lenient = flags.all();
std::cout << lenient;

auto strict = flags.all({}, false);
std::cout << strict;
```

### Output

```text
is_priority  false
ptype: Series
is_priority  NULL
ptype: Series
```

`is_priority` has an observed `false` value (case `1`), so `all()` computes `false`
either way. With the default `skipnull = true`, that `false` is reported as-is. With
`skipnull = false`, the missing cell at case `2` invalidates the whole result — the
presence of *any* missing value in a `skipnull = false` reduction is enough to make
that feature's result `NULL`, even though the computed answer didn't depend on the
missing cell.

## Notes

- `all()`'s result is labeled by feature name (`Axis::Case`) or by case index
  (`Axis::Feature`), and prints like a small labeled `Series`, not like a row sliced
  from the source frame.
- Every selected feature must already be a `Bool` `Series` — pass `features`
  explicitly whenever the frame contains non-boolean columns.
- A missing value never makes an `all()` result `false` by itself; it can only make
  the result `NULL` (under `skipnull = false`) or be silently excluded (under
  `skipnull = true`).
- An empty selection (a feature with zero cases, or `features` matching nothing) is a
  validation error, not a vacuously-`true` result.

## See Also

- [`DataFrame::any`](any.md): require only one participating value to be `true`
  instead of every one, sharing the same parameter shape.
- [`DataFrame::count`](count.md): counts non-missing cells instead of testing
  truthiness.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align.html">
    <span class="label">Previous</span>
    <span class="title">align</span>
  </a>
  <a class="next-link" href="any.html">
    <span class="label">Next</span>
    <span class="title">any</span>
  </a>
</div>
