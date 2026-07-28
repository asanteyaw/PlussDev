<!-- pluss-hand-authored -->

# DataFrame::any

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::any(bool skipnull = true) const -> Thread
DataFrame::any(Axis axis, bool skipnull = true) const -> Thread
```

## What It Does

Reports whether **at least one** value is `true` along the requested axis, over
every `Bool` `Series` feature currently on the frame. Every feature it reduces over
must already be a `Bool` `Series` — `any()` never coerces numbers, `Note`,
`Timeline`, or `TimeIndex` columns into truthiness the way some languages implicitly
do. `any()` itself carries no feature selector: it always reduces over the *entire*
current frame, so a frame that mixes boolean flags with other feature types must be
narrowed down first with [`features()`](features.md) — `checks.features({"in_stock",
"paid"}).any()`, not a `features` argument on `any()` itself.

The two overloads differ only in whether you supply an axis: the short overload always
reduces `Axis::Case` (down each feature's cases); the longer overload lets you choose
`Axis::Feature` instead (across each row's selected features).

## Parameters

**axis** (`Axis`, default `Axis::Case` when omitted)

Reduction direction. `Axis::Case` reduces down each feature's cases, producing one
`true`/`false` per feature — "did this flag ever fire across all rows?" `Axis::Feature`
reduces across each row's selected features instead, producing one `true`/`false` per
case — "did this row have at least one flag set?"

**skipnull** (`bool`, default `true`)

Controls how a missing value affects the reduction's validity. A missing cell is
always treated as `false` while computing the `true`/`false` value itself — a null can
never *cause* an `any()` result to be `true` on its own. What `skipnull` changes is
whether that computed result is trusted:

- `true` (the default): the result is reported as-is, regardless of any nulls present.
- `false`: if any cell that participated in a reduction was missing, that reduction's
  result comes back `NULL` instead of the computed `true`/`false` — even though the
  value was still computed the same way underneath.

## Returns

`Thread`
    One `Bool` entry per participating feature (`Axis::Case`, the default) or per case
    (`Axis::Feature`), holding whether at least one value was `true`.

## Examples

Example 1. Check whether each flag ever fires across all cases, the default axis.

```cpp
using namespace pluss::table;

auto checks = DataFrame({
    {"order_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"in_stock", Thread(pluss::make_series({true, true, false, true, false, true}, pluss::ScalarType::Bool))},
    {"paid",     Thread(pluss::make_series({true, false, false, true, false, true}, pluss::ScalarType::Bool))},
});

auto ever_true = checks.features({"in_stock", "paid"}).any();
std::cout << ever_true;
```

### Output

```text
in_stock  true
    paid  true
ptype: Series
```

Both flags are `true` in at least one row, so both come back `true`. `order_id` was
deliberately narrowed out with [`features()`](features.md) first — calling `any()`
directly on `checks` (all three columns) would fail, because `any()` requires every
feature on the current frame to already be a `Bool` `Series`.

Example 2. Use `Axis::Feature` to ask, per row, whether *either* flag is set — a
common "does this case need any attention?" check.

```cpp
auto needs_attention = checks.features({"in_stock", "paid"}).any(Axis::Feature);
std::cout << needs_attention;
```

### Output

```text
0  true
1  true
2  false
3  true
4  false
5  true
ptype: Series
```

Cases `2` and `4` are `false` because neither `in_stock` nor `paid` is `true` for
those orders; every other case has at least one of the two flags set.

Example 3. See how `skipnull` affects a reduction that includes a missing value.

```cpp
auto priority_valid = pluss::make_series({true, true, false, true}, pluss::ScalarType::Bool);
auto flags = DataFrame({
    {"is_priority", Thread(pluss::make_series({true, false, false, false}, pluss::ScalarType::Bool), priority_valid)},
});

auto lenient = flags.any();
std::cout << lenient;

auto strict = flags.any(false);
std::cout << strict;
```

### Output

```text
is_priority  true
ptype: Series
is_priority  NULL
ptype: Series
```

`is_priority` has one `true` value (case `0`), so both calls compute the same
underlying answer. With the default `skipnull = true`, that `true` is reported as-is.
With `skipnull = false`, the missing cell at case `2` invalidates the whole result —
even though the answer would have been `true` either way, the presence of *any*
missing value in a `skipnull = false` reduction is enough to make that feature's
result `NULL`.

## Notes

- `any()`'s result is labeled by feature name (`Axis::Case`) or by case index
  (`Axis::Feature`), and prints like a small labeled `Series`, not like a row sliced
  from the source frame.
- Every feature on the current frame must already be a `Bool` `Series` — narrow with
  [`features()`](features.md) first whenever the frame contains non-boolean columns.
- A missing value never makes an `any()` result `true` by itself; it can only make the
  result `NULL` (under `skipnull = false`) or be silently ignored (under
  `skipnull = true`).

## See Also

- [`DataFrame::all`](all.md): require every participating value to be `true` instead
  of at least one, sharing the same parameter shape.
- [`DataFrame::count`](count.md): counts non-missing cells instead of testing
  truthiness.
- [`DataFrame::features`](features.md): narrow the frame to the boolean feature(s)
  `any()` should reduce over.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="all.html">
    <span class="label">Previous</span>
    <span class="title">all</span>
  </a>
  <a class="next-link" href="assign.html">
    <span class="label">Next</span>
    <span class="title">assign</span>
  </a>
</div>
