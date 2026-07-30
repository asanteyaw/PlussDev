<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1f364eb9df7b -->

# DataFrame::dropnull

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::dropnull(const DropOptions& options) const -> DataFrame
DataFrame::dropnull(pluss::RangeView<std::string_view> features = {}, bool all = false) const -> DataFrame

template <detail::StringViewNameRange NameRange>
DataFrame::dropnull(const NameRange& features, bool all = false) const -> DataFrame
```

## What It Does

Removes rows according to a missing-data rule — the row-removal counterpart to
[`fillnull()`](fillnull.md) (which replaces missing values instead of discarding the
row) and [`isnull()`](isnull.md)/[`notnull()`](notnull.md) (which report missingness
without acting on it). Use `dropnull()` when incomplete rows — a grade that was never
entered, a measurement the sensor missed, a join key that didn't resolve — should be
excluded from whatever computation comes next, rather than papered over with a filled
value.

Two overloads cover the same underlying policy at different levels of explicitness:

- **Short overload** (`features`, `all`): the common case — say which columns to inspect
  for missingness (or leave it empty to inspect every column) and whether *any* or *all*
  of them being missing should trigger a drop.
- **`DropOptions` overload**: the fully explicit route, spelling out `features`, `axis`,
  `all`, and additionally `threshold` (a minimum count of non-missing values a row must
  have to survive) as named struct fields — reach for this once the short overload's two
  positional arguments stop being expressive enough.

## Parameters

**features** (`pluss::RangeView<std::string_view>`, default empty)

Which features to inspect for missingness. Leave this empty to inspect every feature on
the frame. Pass a subset when only certain columns — grading columns, measurement
columns, join keys — should have a say in whether a row survives; missingness in an
unlisted feature is simply not consulted. Any container of `std::string_view`-convertible
names (`std::vector<std::string>`, `std::set<std::string>`, and similar) works too, via a
generic template overload — not just a braced list or an explicit `RangeView`.

**all** (`bool`, default `false`)

The row-drop rule applied across the selected features:

- `false` (the default): drop a row when *any* selected feature is missing — the strict
  reading, "every inspected value must be present."
- `true`: drop a row only when *all* selected features are missing — the lenient
  reading, "keep the row unless it's missing everywhere we're looking."

**options** (`const DropOptions&`)

A structured policy object for the same operation, adding an explicit `axis` (row versus
feature dropping) and `threshold` (minimum count of non-missing values required to keep a
row) alongside `features` and `all`. Reach for this overload when the null policy needs
to be stated field by field in calling code rather than packed into two positional
arguments.

## Returns

`DataFrame`
    A new frame containing only the rows that survive the requested null policy, in
    their original row order.

## Examples

Example 1. Drop rows when any selected grading column is missing.

```cpp
using namespace pluss::table;

auto missing = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen", "Dede"})},
    {"quiz",    pluss::make_series({10.0, pluss::nan(), 8.0, pluss::nan()})},
    {"final",   pluss::make_series({80.0, 75.0, pluss::nan(), 90.0})},
});

auto out = missing.dropnull({"quiz", "final"}, false);
std::cout << out;
```

### Output

```text
===========================
     | student  quiz  final
---------------------------
0    | Ada      10    80   
---------------------------
[1 cs x 3 feat]
```

Only `Ada` (case `0`) has both `quiz` and `final` present. `Bola` is missing `quiz`,
`Chen` is missing `final`, and `Dede` is missing `quiz` — under `all = false`, any one
missing value among the selected features is enough to drop the row, so all three are
excluded and only the fully-complete row remains.

Example 2. Spell the same rule through `DropOptions`.

```cpp
using namespace pluss::table;

auto missing = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen", "Dede"})},
    {"quiz",    pluss::make_series({10.0, pluss::nan(), 8.0, pluss::nan()})},
    {"final",   pluss::make_series({80.0, 75.0, pluss::nan(), 90.0})},
});

DataFrame::DropOptions options;
options.features = {"quiz", "final"};
options.axis = Axis::Case;
options.all = false;

auto out = missing.dropnull(options);
std::cout << out;
```

### Output

```text
===========================
     | student  quiz  final
---------------------------
0    | Ada      10    80   
---------------------------
[1 cs x 3 feat]
```

Identical result to Example 1 — `DropOptions` states the same `any-missing-among-{quiz,
final}` rule explicitly as struct fields instead of positional arguments. This is the
overload to reach for once a call site also needs `threshold` (say, "keep a row if at
least 2 of 3 inspected columns are present" rather than an all-or-nothing rule), which
has no positional-argument equivalent in the short overload.

Example 3. `all = true`: only drop a row when *every* selected feature is
missing, keeping a row that's missing just one of them.

```cpp
using namespace pluss::table;

auto missing2 = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen", "Dede"})},
    {"quiz",    pluss::make_series({10.0, pluss::nan(), pluss::nan(), 5.0})},
    {"final",   pluss::make_series({80.0, 75.0, pluss::nan(), pluss::nan()})},
});

auto kept_any = missing2.dropnull({"quiz", "final"}, false);
std::cout << kept_any;

auto kept_all = missing2.dropnull({"quiz", "final"}, true);
std::cout << kept_all;
```

### Output (all = false, the default)

```text
===========================
     | student  quiz  final
---------------------------
0    | Ada      10    80   
---------------------------
[1 cs x 3 feat]
```

### Output (all = true)

```text
===========================
     | student  quiz  final
---------------------------
0    | Ada      10    80   
1    | Bola     NULL  75   
3    | Dede     5     NULL 
---------------------------
[3 cs x 3 feat]
```

`missing2` has one row (`Chen`, case `2`) missing both `quiz` and `final`, and
two rows each missing exactly one of them (`Bola` missing `quiz`, `Dede`
missing `final`). With the default `all = false`, any one missing value is
enough to drop a row, so only `Ada` (fully complete) survives. With `all =
true`, only `Chen` — missing *both* selected features — is dropped; `Bola` and
`Dede` survive despite each still having one `NULL` cell, since they aren't
missing *everywhere* the rule is checking. Note the surviving case labels
(`0, 1, 3`): `dropnull()` never renumbers rows, so the gap left by dropping
case `2` stays a gap.

## Notes

- The short overload is the common "drop rows with missing values in these columns"
  route; `DropOptions` is the more explicit route when the null policy — including
  `threshold` — needs to be written out field by field.
- The output preserves the original row order of every surviving case; `dropnull()`
  never reorders rows, it only removes some.
- `student` was never inspected in either example (it wasn't listed in `features`), so a
  missing `student` value would not by itself cause a row to drop — only `quiz` and
  `final` participate in the decision.
- To replace missing values instead of discarding their rows, see
  [`fillnull()`](fillnull.md); to inspect missingness without acting on it at all, see
  [`isnull()`](isnull.md) and [`notnull()`](notnull.md).

## See Also

- [`DataFrame::fillnull`](fillnull.md): replace missing values instead of dropping rows.
- [`DataFrame::isnull`](isnull.md), [`DataFrame::notnull`](notnull.md): inspect
  missingness first, without removing anything.
- [`DataFrame::drop`](drop.md): remove whole features by name, the schema-level
  counterpart to this row-level operation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop_duplicates.html">
    <span class="label">Previous</span>
    <span class="title">drop_duplicates</span>
  </a>
  <a class="next-link" href="dtypes.html">
    <span class="label">Next</span>
    <span class="title">dtypes</span>
  </a>
</div>
