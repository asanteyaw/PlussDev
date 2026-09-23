<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f5f95771ecf1 -->

# DataFrame::shownull

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::shownull(Axis axis = Axis::Case, bool all = false) const -> DataFrame
```

## What It Does

Reveals the rows or features that participate in missingness, instead of returning a
boolean mask like [`isnull()`](isnull.md) or dropping non-missing structure like
[`dropnull()`](dropnull.md). This is the inspection counterpart to
`dropnull(...)`: keep the *incomplete* part of the frame so you can see what is
actually missing.

The default call, `shownull()`, works case-wise and keeps every row containing **at
least one** null cell anywhere on the frame. That is the most useful default for
"show me the problematic rows" work, and it matches the way [`info()`](info.md)'s
`Null Case Rows` count is computed.

The key mental model is:

- `isnull()` answers: "which individual cells are missing?"
- `dropnull()` answers: "which rows/features should I discard?"
- `shownull()` answers: "show me only the rows/features that are involved in missingness."

So `shownull()` is the right stop when you are still diagnosing data quality and want
to *look at the actual offending subframe* before deciding whether to drop, fill, or
ignore it.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Which side of the frame to reveal:

- `Axis::Case`: keep rows that satisfy the requested null rule.
- `Axis::Feature`: keep features that satisfy the requested null rule.

**all** (`bool`, default `false`)

How strict the null rule is across the inspected cells:

- `false` (default): keep a row/feature when it contains **any** null.
- `true`: keep a row/feature only when **all** inspected cells along that axis are null.

## Returns

`DataFrame`
    A smaller frame containing only the rows or only the features that match the
    requested missingness rule, with order and labels preserved.

## When To Use It

Use `shownull()` when you are still in the inspection stage and want the missing part
of the frame isolated for you.

Typical use cases:

- after a load or merge, to see exactly which rows became incomplete
- before imputation, to understand whether the missingness is localized or widespread
- before dropping data, to verify that the rows or features you are about to remove
  are genuinely low-quality
- when `info()` reports null-heavy structure and you want the concrete subframe behind
  that summary

The important workflow distinction is:

- `info()` tells you **how much** missingness you have
- `shownull()` shows you **where** it lives
- `fillnull()` or `dropnull()` decides **what to do about it**

## Examples

Example 1. Default behavior: show rows with any null anywhere on the frame.

```cpp
using namespace pluss::table;

const auto nan = std::numeric_limits<double>::quiet_NaN();

auto readings = DataFrame({
    {"a", pluss::make_series(std::vector<double>{1.0, nan, 3.0, nan})},
    {"b", pluss::make_series(std::vector<double>{10.0, 11.0, nan, nan})},
    {"c", pluss::make_series(std::vector<double>{100.0, 101.0, 102.0, nan})},
});

auto out = readings.shownull();
std::cout << out;
```

### Output

```text
=======================
     | a     b     c   
-----------------------
1    | NULL  11    101 
2    | 3     NULL  102 
3    | NULL  NULL  NULL
-----------------------
[3 cs x 3 feat]
```

Case `0` is omitted because it is fully observed. Cases `1`, `2`, and `3` remain,
in their original order, because each contains at least one `NULL`.

Example 2. Feature-wise reveal: keep columns with any null, then only the fully-null one.

```cpp
auto features = DataFrame({
    {"clean",     pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0})},
    {"some_null", pluss::make_series(std::vector<double>{5.0, nan, 7.0, 8.0})},
    {"all_null",  pluss::make_series(std::vector<double>{nan, nan, nan, nan})},
});

auto any_null_feature = features.shownull(Axis::Feature);
std::cout << any_null_feature;
```

### Output

```text
=====================
     | some_null  all_null
---------------------
0    | 5          NULL
1    | NULL       NULL
2    | 7          NULL
3    | 8          NULL
---------------------
[4 cs x 2 feat]
```

```cpp
auto fully_null_feature = features.shownull(Axis::Feature, true);
std::cout << fully_null_feature;
```

### Output

```text
==============
     | all_null
--------------
0    | NULL
1    | NULL
2    | NULL
3    | NULL
--------------
[4 cs x 1 feat]
```

With `Axis::Feature`, `all = false` means "keep any column that has at least one
missing cell." Switching to `all = true` tightens that to "keep only columns that are
missing everywhere."

Example 3. Contrast `shownull()` with `dropnull()` on the same frame.

```cpp
auto missing_rows = readings.shownull();
auto clean_rows = readings.dropnull();
```

`missing_rows` contains the rows that need attention; `clean_rows` contains the rows
already safe to proceed with. They are complementary views of the same missingness
rule:

- `shownull(Axis::Case, false)` keeps rows with at least one null.
- `dropnull({}, false)` keeps rows with no nulls at all.

That pairing is often the cleanest audit workflow:

1. `shownull()` to inspect what is broken.
2. `fillnull()` if the gaps are recoverable.
3. `dropnull()` only if those rows or features truly should be excluded.

Example 4. Use `Axis::Case, true` to isolate fully-empty rows only.

```cpp
auto sparse = DataFrame({
    {"x", pluss::make_series(std::vector<double>{nan, 1.0, nan, nan})},
    {"y", pluss::make_series(std::vector<double>{nan, 2.0, nan, 5.0})},
});

auto fully_null_rows = sparse.shownull(Axis::Case, true);
std::cout << fully_null_rows;
```

### Output

```text
============
     | x     y
------------
0    | NULL  NULL
2    | NULL  NULL
------------
[2 cs x 2 feat]
```

This is stricter than the default. Case `3` is excluded because it has one observed
value (`y = 5`) even though it is still partially incomplete.

Example 5. Feature-wise reveal is useful after wide joins or feature engineering.

```cpp
auto wide = DataFrame({
    {"customer_id", pluss::make_series(std::vector<std::int64_t>{1, 2, 3})},
    {"age",         pluss::make_series(std::vector<double>{34.0, nan, 29.0})},
    {"income",      pluss::make_series(std::vector<double>{nan, nan, nan})},
    {"score",       pluss::make_series(std::vector<double>{710.0, 688.0, 701.0})},
});

auto null_features = wide.shownull(Axis::Feature);
std::cout << null_features;
```

### Output

```text
==================
     | age   income
------------------
0    | 34    NULL
1    | NULL  NULL
2    | 29    NULL
------------------
[3 cs x 2 feat]
```

Here `customer_id` and `score` disappear because they are fully observed. That makes
`shownull(Axis::Feature)` a compact schema-audit tool on wide frames.

## Notes

- `shownull()` preserves the surviving row order, feature order, and case labels.
- The default `Axis::Case` + `all = false` behavior is the missingness-reveal mirror
  of `dropnull(..., all = false)`, which removes those same rows instead of showing
  them.
- `Axis::Case, all = true` is intentionally strict: a row survives only if every
  feature in that row is null. This is mainly useful for spotting fully-empty records
  after load/merge operations.
- `Axis::Feature, all = true` is the schema-level equivalent: it helps locate dead
  columns that never received a value at all.
- If you need the full cellwise mask rather than the revealed subframe, use
  [`isnull()`](isnull.md).

## See Also

- [`DataFrame::isnull`](isnull.md): full boolean missingness mask.
- [`DataFrame::dropnull`](dropnull.md): remove the incomplete rows/features instead of
  showing them.
- [`DataFrame::info`](info.md): structural missingness summary before drilling into the
  offending rows or features.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.md">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="shuffle.md">
    <span class="label">Next</span>
    <span class="title">shuffle</span>
  </a>
</div>
