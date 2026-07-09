# Part V: DataFrame Workflows

This part covers the daily DataFrame operations: inspect, select, filter, sort,
assign, transform, summarize, combine, and save.

## Inspect a frame

```cpp
df.head();
df.tail();
df.view(10, 20);
df.info();
df.describe();
```

Use shape and schema methods when writing logic:

```cpp
if (!df.defined() || df.ncases() == 0) {
  throw std::runtime_error("empty input");
}

for (const auto& name : df.feature_names()) {
  std::cout << name << '\n';
}
```

## Select features

```cpp
auto one = df.features("balance");
auto some = df.features({"age", "job", "balance"});
auto numeric = df.series_features();
auto text = df.note_features();
auto temporal = df.timeline_features();
```

### Verified example: numeric feature selection and reductions

```cpp
auto sales = sales_frame();
auto numeric = sales.series_features();

std::cout << numeric.to_string() << '\n';
std::cout << "units mean=" << sales.features("units").mean().item<double>() << '\n';
std::cout << "price max=" << sales.features("price").max().item<double>() << '\n';
```

Output:

```text
===================
     | units  price
-------------------
0    | 10     3
1    | 7      4
2    | 12     2.5
3    | 5      5
-------------------
[4 cs x 2 feat]
units mean=8.5
price max=5
```

`series_features()` removed the text features and kept only numeric `Series`
features. The mean of `units` is `(10 + 7 + 12 + 5) / 4 = 8.5`; the maximum
price is `5`.

Pattern-based selection:

```cpp
auto costs = df.select_like("Cost", false);
auto monthly = df.select_regex(".*_month$");
```

Schema management:

```cpp
auto renamed = df.rename("old", "new");
auto renamed_many = df.rename({{"age", "age_years"}, {"job", "role"}});
auto relabeled = df.rename({"a", "b", "c"});
auto prefixed = df.add_prefix("raw_");
auto suffixed = df.add_suffix("_clean");
auto reordered = df.reorder({"id", "date", "value"});
auto dropped = df.drop({"temporary_feature"});
```

`rename` changes one feature name, applies several renames, or replaces the
entire feature axis depending on the overload. Full-axis renaming requires the
number of new names to match the current feature count. `reorder` also requires
the full feature set; it is not a selector. Use `features({...})` or `drop({...})`
when you want to keep only part of the frame.

Practical rule:

- use `rename("old", "new")` for one feature
- use `rename({{"old1", "new1"}, {"old2", "new2"}})` for many features

## Select cases

```cpp
auto first = df.case_study(0);
auto first_ten = df.case_study(0, 10);
auto middle = df.slice(100, 200);
auto sampled = df.sample(100, 7);
auto taken = df.take({0, 2, 5, 9});
```

`head`, `tail`, and `view` are preview-oriented and return `DataFrameView`.
`slice`, `take`, and `sample` return `DataFrame`.

`slice(start, stop)` uses half-open row positions: it keeps `start` and stops
before `stop`. Negative slice bounds are normalized against the frame size by
the implementation. `take(indices)` uses the exact row positions you provide
and keeps them in that order. Both preserve the selected rows' original case
labels.

## Filter cases

Use query expressions for readable filters:

```cpp
auto older = df.query("age >= 45");
auto known = df.query("job != 'unknown'");
auto band = df.query("age between 45 and 50");
auto selected = df.query("job in ('management', 'technician')");
auto combined = df.query("age >= 45 and balance > 0");
```

Use `filter` when you already have a boolean `Series` or boolean `Thread`:

```cpp
auto mask = df.features("balance").gt(0.0).as_series();
auto positive = df.filter(mask);
```

For note-backed columns, direct string-literal comparison is supported:

```cpp
auto laurels = df.filter(df.features("First Name") == "Laurel");
```

That keeps every row whose `First Name` equals `"Laurel"`.

### Verified example: assign then query

```cpp
auto enriched = sales.assign({{
    "revenue",
    [](const DataFrame& frame) {
      return frame.features("units") * frame.features("price");
    },
}});

auto high_value = enriched.query("revenue >= 30");
std::cout << high_value.to_string() << '\n';
```

Output:

```text
============================================
     | city    region  units  price  revenue
--------------------------------------------
0    | Accra   south   10     3      30
2    | Kumasi  middle  12     2.5    30
--------------------------------------------
[2 cs x 5 feat]
```

`assign` evaluates the lambda against the evolving output frame and adds the
new `revenue` feature. The query keeps rows where `revenue >= 30`, so Accra and
Kumasi remain. Tema has revenue `28`, and Tamale has revenue `25`, so both are
filtered out.

## Clean text columns

For text-specific transforms, drop to `Note` with `.as_note()`:

```cpp
gradebook.set("OrgDefinedId") =
    gradebook.features("OrgDefinedId").as_note().strip().lstrip("#");
```

This is the common text-cleaning pattern for IDs that arrive with leading
markers.

Use `strip()` for whitespace cleanup, `lstrip(chars)` for leading markers, and
`rstrip(chars)` for trailing markers.

## Sort and rank

```cpp
auto by_age = df.sort_values({"age"});
auto by_age_desc = df.sort_values({"age"}, false);

auto by_multiple = df.sort_values({
    {"age", true, true},
    {"duration", false, true},
});

auto top = df.top("balance", 10);
auto bottom = df.bottom("balance", 10);
```

`sort_values({"age"})` sorts ascending by default and puts null values last by
default. Use `ArrangeSpec` when a sort needs per-feature direction or null
placement. `top(feature, n)` is shorthand for descending sort plus the first
`n` rows. `bottom(feature, n)` is shorthand for ascending sort plus the first
`n` rows.

### Verified example: shaping rows, schema, and case labels

```cpp
auto sales = sales_frame();

auto sorted = sales.sort_values({DataFrame::ArrangeSpec{"price", false, true}})
                  .features({"city", "price"});
std::cout << sorted.to_string() << '\n';

std::cout << sales.slice(1, 3).to_string() << '\n';
std::cout << sales.take(std::vector<std::int64_t>{3, 0}).to_string() << '\n';

auto schema = sales.rename("units", "quantity")
                 .drop({"region"})
                 .reorder({"city", "price", "quantity"});
std::cout << schema.to_string() << '\n';

auto city_cases = sales.set_cases("city", true);
std::cout << city_cases.features({"region", "units"}).to_string() << '\n';

auto reset = city_cases.reset_cases("city_case").features({"city_case", "units"});
std::cout << reset.to_string() << '\n';
```

Output:

```text
====================
     | city    price
--------------------
3    | Tamale  5
1    | Tema    4
0    | Accra   3
2    | Kumasi  2.5
--------------------
[4 cs x 2 feat]
===================================
     | city    region  units  price
-----------------------------------
1    | Tema    south   7      4
2    | Kumasi  middle  12     2.5
-----------------------------------
[2 cs x 4 feat]
===================================
     | city    region  units  price
-----------------------------------
3    | Tamale  north   5      5
0    | Accra   south   10     3
-----------------------------------
[2 cs x 4 feat]
==============================
     | city    price  quantity
------------------------------
0    | Accra   3      10
1    | Tema    4      7
2    | Kumasi  2.5    12
3    | Tamale  5      5
------------------------------
[4 cs x 3 feat]
======================
       | region  units
----------------------
Accra  | south   10
Tema   | south   7
Kumasi | middle  12
Tamale | north   5
----------------------
[4 cs x 2 feat]
=======================
     | city_case  units
-----------------------
0    | Accra      10
1    | Tema       7
2    | Kumasi     12
3    | Tamale     5
-----------------------
[4 cs x 2 feat]
```

The first table is sorted by `price` descending. The case labels still show
the original row labels, so the first row displays case `3` because Tamale was
the fourth row in the input.

The second table is `slice(1, 3)`. It keeps row positions 1 and 2 and stops
before position 3. The third table is `take({3, 0})`; it keeps exactly Tamale
then Accra because that is the order of the indices.

The fourth table demonstrates feature-axis work. `rename("units", "quantity")`
changes the feature name, `drop({"region"})` removes the region feature, and
`reorder({"city", "price", "quantity"})` arranges the full remaining feature
set in the requested order.

The fifth table shows `set_cases("city", true)`. City values become case labels,
and the original `city` feature is removed because `drop_feature` is `true`.
The sixth table shows `reset_cases("city_case")`, which turns the case axis
back into a normal feature named `city_case` and restores default numeric case
labels.

## Derive features

Use `assign` for one or more derived features:

```cpp
auto enriched = df.assign({{
    "balance_per_second",
    [](const pluss::table::DataFrame& frame) {
      return frame.features("balance") / frame.features("duration");
    },
}});
```

Use `set_features` when the new feature is already computed:

```cpp
auto doubled = df.set_features("balance_x2", df.features("balance") * 2.0);
```

Use mutable assignment only when mutation is intentional:

```cpp
df.set_features_("balance_x2", df.features("balance") * 2.0);
```

## Transform features

Transform all features:

```cpp
auto transformed = df.transform([](const pluss::table::Thread& feature) {
  if (feature.kind() == pluss::table::ThreadKind::Series) {
    return feature + 1.0;
  }
  return feature;
});
```

Transform selected features:

```cpp
auto adjusted = df.transform({
    {"age", [](const auto& feature) { return feature + 1.0; }},
    {"balance", [](const auto& feature) { return feature.clip(-1000.0, 1000.0); }},
});
```

## Missing data

```cpp
auto nulls = df.isnull();
auto present = df.notnull();
auto filled_numeric = df.fillna(0.0, {"balance", "duration"});
auto filled_text = df.fillna("unknown", {"job"});
auto forward = df.ffill();
auto backward = df.bfill();
auto complete = df.dropna();
auto complete_selected = df.dropna({"balance", "duration"}, true);
```

Count null rows:

```cpp
auto null_rows = df.isnull().any(pluss::table::Axis::Case).sum();
```

### Verified example: missing data audit

```cpp
const std::vector<std::vector<std::string>> records = {
    {"id", "city", "units", "price"},
    {"A001", "Accra", "10", "3.0"},
    {"A002", "Tema", "", "4.0"},
    {"A003", "", "12", ""},
};

pluss::loader::ReadCsvOptions options;
options.case_col = "id";
auto df = pluss::loader::load_csv_records(records, options);

std::cout << df.to_string() << '\n';
std::cout << "null rows=" << df.count_null_rows() << '\n';
std::cout << df.isna().to_string() << '\n';
std::cout << df.fillna(0.0, {"units", "price"})
               .fillna("unknown", {"city"})
               .to_string() << '\n';
```

Output:

```text
==========================
     | city   units  price
--------------------------
A001 | Accra  10     3
A002 | Tema   NA     4
A003 | NA     12     NA
--------------------------
[3 cs x 3 feat]
null rows=2
==========================
     | city   units  price
--------------------------
A001 | false  false  false
A002 | false  true   false
A003 | true   false  true
--------------------------
[3 cs x 3 feat]
============================
     | city     units  price
----------------------------
A001 | Accra    10     3
A002 | Tema     0      4
A003 | unknown  12     0
----------------------------
[3 cs x 3 feat]
```

The first table shows missing cells as `NA`. `count_null_rows()` reports `2`
because rows `A002` and `A003` have at least one null value. The `isna()` table
is a boolean mask with `true` exactly where the displayed input had `NA`.

The filled table keeps all existing non-null values. It replaces missing
numeric cells in `units` and `price` with `0`, and replaces the missing `city`
cell with `unknown`.

## Arithmetic and comparisons

DataFrame arithmetic delegates to numeric features:

```cpp
auto numeric = df.features({"age", "balance", "duration"});

numeric.add(1.0);
numeric.subtract(1.0);
numeric.multiply(2.0);
numeric.divide(2.0);
numeric.pow(2.0);
```

Reverse operations:

```cpp
numeric.rsub(100.0);
numeric.rpow(2.0);
```

Comparisons:

```cpp
auto ge_zero = numeric.ge(0.0);
auto same = numeric.eq(numeric);
auto cmp = numeric.compare(numeric.add(1.0));
```

## Aggregation

Simple reductions:

```cpp
df.series_features().mean(true);
df.series_features().std(true);
df.series_features().corr(true);
```

Named summaries:

```cpp
df.aggregate({
    {"cases", "age", DataFrame::SummaryOp::Count},
    {"age_mean", "age", DataFrame::SummaryOp::Mean},
    {"job_cardinality", "job", DataFrame::SummaryOp::Nunique},
}, true);
```

### Verified example: aggregate and group

```cpp
auto sales = sales_frame().assign({{
    "revenue",
    [](const DataFrame& frame) {
      return frame.features("units") * frame.features("price");
    },
}});

auto summary = sales.aggregate({
    {"total_units", "units", DataFrame::SummaryOp::Sum},
    {"mean_price", "price", DataFrame::SummaryOp::Mean},
    {"max_revenue", "revenue", DataFrame::SummaryOp::Max},
}, false);
std::cout << summary.to_string() << '\n';

auto grouped = sales.group_by(
    {"region"},
    {
        {"units_sum", "units", DataFrame::GroupOp::Sum},
        {"revenue_mean", "revenue", DataFrame::GroupOp::Mean},
    },
    true,
    false);
std::cout << grouped.to_string() << '\n';
```

Output:

```text
==============================================
        | total_units  mean_price  max_revenue
----------------------------------------------
summary | 34           3.625       30
----------------------------------------------
[1 cs x 3 feat]
======================================
     | region  units_sum  revenue_mean
--------------------------------------
0    | south   17         29
1    | middle  12         30
2    | north   5          25
--------------------------------------
[3 cs x 3 feat]
```

The aggregate result has one case named `summary`. The grouped result has one
row per region. The south group combines Accra and Tema, so `units_sum` is
`10 + 7 = 17`, and mean revenue is `(30 + 28) / 2 = 29`.

Grouped summaries:

```cpp
df.group_by(
    {"job"},
    {
        {"count", "age", DataFrame::GroupOp::Count},
        {"mean_balance", "balance", DataFrame::GroupOp::Mean},
        {"max_duration", "duration", DataFrame::GroupOp::Max},
    },
    true,
    true);
```

Convenience group summaries:

```cpp
df.count_by("job", 10, true, true);
df.mean_by("job", "balance", 10, 1, true, true);
```

## Pivoting

```cpp
df.pivot("date", "symbol", "close", true, true);
df.pivot_table("date", "symbol", "volume", DataFrame::GroupOp::Mean, true, true);
```

Use `pivot` when the case-feature and feature-feature pairs identify one value.
Use `pivot_table` when duplicate pairs need aggregation.

## Combining frames

```cpp
auto joined = left.join(right, "_right");
auto updated = left.update(right, true);
auto combined = left.combine(right);

auto stitched = left.unify("id", right, "id", "common", "_right");
```

Use `join` when two frames already have compatible row counts and you want to
append features. Use `unify` when rows must be matched by key values. Use
`update` when the right frame should patch values into the left frame. Use
`combine` when you need the configurable primitive behind `join` and `update`.

The names are precise:

- `join` is feature-wise combination. It requires the same case count and
  appends right-hand features by row position. Duplicate feature names receive
  the collision suffix.
- `update` is schema-preserving patching. It only updates overlapping features
  and ignores right-hand features that do not exist on the left.
- `unify` is key-based matching. It creates rows for matching key values
  and appends right-hand payload features.
- `combine` is the configurable primitive underneath `join` and `update`.

### Verified example: join, update, and unify

```cpp
DataFrame left({
    {"sku", pluss::make_note({"A", "B", "C"})},
    {"qty", pluss::make_series({1.0, 2.0, 3.0})},
});
DataFrame right({
    {"sku", pluss::make_note({"B", "C", "D"})},
    {"qty", pluss::make_series({20.0, 30.0, 40.0})},
    {"cost", pluss::make_series({5.0, 8.0, 13.0})},
});
DataFrame patch({
    {"qty", pluss::make_series({10.0, 20.0, 30.0})},
    {"status", pluss::make_note({"new", "open", "closed"})},
});

std::cout << left.join(patch, "_patch").to_string() << '\n';
std::cout << left.update(patch, true).to_string() << '\n';
std::cout << left.unify("sku", right, "sku", "common", "_right").to_string() << '\n';
```

Output:

```text
====================================
     | sku   qty   qty_patch  status
------------------------------------
0    | A     1     10         new
1    | B     2     20         open
2    | C     3     30         closed
------------------------------------
[3 cs x 4 feat]
=================
     | sku   qty
-----------------
0    | A     10
1    | B     20
2    | C     30
-----------------
[3 cs x 2 feat]
==================================
     | sku   qty   qty_right  cost
----------------------------------
0    | B     2     20         5
1    | C     3     30         8
----------------------------------
[2 cs x 4 feat]
```

The first result is `join`. The left frame has `sku` and `qty`; the patch frame
has `qty` and `status`. Because `qty` exists on both sides, PlussMic keeps the
left `qty` and appends the right feature as `qty_patch`. `status` is appended
without renaming.

The second result is `update`. Only overlapping features are considered, so
`qty` is replaced by the patch values and `status` is ignored. This is useful
when the right frame is a correction frame rather than a source of new columns.

The third result is `stitch_on("sku", right, "sku", "_right")`. This is the
key-based operation. Only `B` and `C` appear because those keys exist in both
frames. The right key feature is not duplicated because both key names are
`sku`. The right `qty` collides with the left `qty`, so it becomes
`qty_right`; `cost` is appended as a new payload feature.

### Prefer `unify(...)` for key-based merge workflows

Same-key left merge:

```cpp
auto df_ready = gradebook.unify(
    "OrgDefinedId",
    finals.features({"OrgDefinedId", "Final Points Grade"}),
    "",
    "left"
);
```

Different-key left merge:

```cpp
auto df_ready = gradebook.unify(
    "OrgDefinedId",
    finals.features({"Username", "Final Points Grade"}),
    "Username",
    "left"
);
```

Cardinality validation:

```cpp
auto df_ready = gradebook.unify(
    "OrgDefinedId",
    finals.features({"Username", "Final Points Grade"}),
    "Username",
    "left",
    "_right",
    "many_to_one"
);
```

`validate` values:

- `"many_to_many"`: default
- `"one_to_one"`
- `"one_to_many"`
- `"many_to_one"`

Important behavior:

- matching is by key value, not row order
- `how="left"` preserves left-row order
- duplicate right-side matches expand rows unless `validate` forbids them
- `right_suffix` is only used when a non-key right-hand feature name collides with a left-hand feature name

## Display and titles

```cpp
auto named = df.title("Clean sales");
named.show();

pluss::table::display::DisplayOptions options;
named.show(options);
```

Preview methods return display-aware `DataFrameView` objects:

```cpp
auto preview = df.head(20);
preview.show();
```

See [DataFrame Display](../dataframe_display.md) for terminal and popup behavior.
