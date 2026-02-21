# DataFrame + Atlas User Guide (PlussMic)

This guide documents how to use `pluss::table::DataFrame` and `pluss::table::Atlas` as an end user.

It tracks the current implementation in `PlussMic/src/table/` and is intended to be updated as new focused methods ship.

## 1. Include And Namespaces

```cpp
#include <pluss/pluss.h>   // unified umbrella (includes mic when available)
// or:
#include <pluss/mic.h>     // PlussMic umbrella only
```

Main table types:

- `pluss::table::Thread` (1D)
- `pluss::table::DataFrame` (2D)
- `pluss::table::Atlas` (3D strict cube)

## 2. Core Mental Model

- A DataFrame is a collection of **features**.
- A DataFrame has a **case axis** (rows), conceptually equivalent to pandas index.
- A feature payload is one pluss type via `Thread`:
  - `Series` (numeric/bool)
  - `Note` (text)
  - `Timeline` or `TimeIndex` (datetime/time index)

The design direction is:

- `Atlas -> DataFrame -> Thread -> (Series/Note/Timeline/TimeIndex)`

## 3. Loading Data

### CSV

Recommended entrypoint:

```cpp
using pluss::table::DataFrame;

auto df = DataFrame::load_csv("../../data/messy_data.csv");
```

Useful overloads:

```cpp
auto df = DataFrame::load_csv(
    "../../data/bank-full.csv",
    /*header=*/"infer",      // or "none"
    /*case feature=*/"none", // name of feature to use as case axis, or "none"
    /*engine=*/"auto",       // auto | serial | parallel
    /*ncases=*/std::nullopt  // optional row cap
);
```

Advanced config overloads are also available through `Config` / `ReadCsvOptions`.

Default CSV behavior (simple `load_csv(path)`):

- Header inferred from first row
- Separator auto-detected
- Engine auto-selected (`parallel` for larger files based on `parallel_min_bytes`)
- Case axis defaults to `0..n-1` if no case feature is supplied
- Type inference maps features to `Series` / `Note` / `Timeline` where possible

### Excel

```cpp
auto df = DataFrame::load_excel("../../data/report.xlsx");               // default first sheet
auto df_prices = DataFrame::load_excel("../../data/report.xlsx", "Prices");
auto df_first = DataFrame::load_excel("../../data/report.xlsx", 0);
```

Load multiple sheets:

```cpp
auto loaded = DataFrame::load_excel("../../data/report.xlsx", nullptr, true); // workbook mode
auto names = loaded.sheet_names();
auto prices = loaded.sheets("Prices");
auto first = loaded.sheet_by_index(0);
```

Notes:

- Excel support requires build-time dependencies (e.g., libzip path configured).
- If Excel support is not enabled, load calls throw with a clear message.

### Unified Loader

You can still use centralized dispatch:

```cpp
auto df = DataFrame::load("csv", "../../data/messy_data.csv");
auto loaded = pluss::loader::load("excel", "../../data/report.xlsx", {{"sheet name", "Prices"}});
```

### Constructing In Memory (No CSV/Excel)

You can build `Thread`, `DataFrame`, and `Atlas` directly from your own raw lvalues/containers.

Build `Thread` values from pluss-native payloads:

```cpp
using pluss::table::Thread;

// numeric lvalue container -> Series -> Thread
std::vector<double> price_raw{12.5, 8.0, 19.2, 7.7};
auto price_s = pluss::make_series(price_raw, {static_cast<std::int64_t>(price_raw.size())});
Thread price_t(price_s);

// text lvalue container -> Note -> Thread
std::vector<std::string> city_raw{"Austin", "Dallas", "Houston", "Plano"};
Thread city_t(pluss::make_note(city_raw));

// datetime lvalue container -> Timeline -> Thread
std::vector<std::string> ts_raw{
    "2026-01-01 08:00:00",
    "2026-01-01 09:00:00",
    "2026-01-01 10:00:00",
    "2026-01-01 11:00:00",
};
Thread start_t(pluss::make_timeline(ts_raw, {static_cast<std::int64_t>(ts_raw.size())}));

// optional null mask (same length as payload)
auto valid = pluss::make_series({true, true, false, true});
Thread price_with_nulls(price_s, valid);
```

Build a `DataFrame` from named features:

```cpp
using pluss::table::DataFrame;

DataFrame df({
    {"price", price_t},
    {"city", city_t},
    {"start_time", start_t},
});
// default cases axis: 0..n-1
```

Build a `DataFrame` with explicit cases labels:

```cpp
Thread case_labels(pluss::make_note(std::vector<std::string>{"c0", "c1", "c2", "c3"}));

DataFrame df_labeled({
    {"price", price_t},
    {"city", city_t},
    {"start_time", start_t},
}, case_labels);
```

Build an `Atlas` from multiple DataFrames:

```cpp
using pluss::table::Atlas;

auto df_t0 = df;
auto df_t1 = df.set_features("price", df.features("price") + 1.0);
auto df_t2 = df_t1.set_features("price", df_t1.features("price") + 1.0);

Atlas at(
    std::vector<DataFrame>{df_t0, df_t1, df_t2},
    Thread(pluss::make_note(std::vector<std::string>{"t0", "t1", "t2"}))
);
```

Rules still apply during manual construction:

- All features in a `DataFrame` must be 1D and case-aligned (same length).
- `cases` length must equal `nrows`.
- All frames in an `Atlas` must share schema and shape.

Rvalue construction is also supported:

```cpp
using pluss::table::Thread;
using pluss::table::DataFrame;
using pluss::table::Atlas;

// Thread from rvalue payloads
Thread t_num(pluss::make_series({1.0, 2.0, 3.0}));
Thread t_txt(pluss::content({"a", "b", "c"}));
Thread t_dt(pluss::moment({"2026-01-01 08:00:00", "2026-01-01 09:00:00", "2026-01-01 10:00:00"}));

// DataFrame from rvalue feature list
DataFrame df_r({
    {"x", Thread(pluss::make_series({1, 2, 3}))},
    {"label", Thread(pluss::content({"u", "v", "w"}))},
});

// Atlas from rvalue frame stack
Atlas at_r(
    std::vector<DataFrame>{
        df_r,
        df_r.set_features("x", df_r.features("x") + 10.0)
    }
);
```

## 4. Display And Rendering

Display methods are terminal-aware and responsive:

- `df.head(n)` (default `n=5`)
- `df.tail(n)` (default `n=5`)
- `df.view(start, stop)`
- `df.describe()`
- `df.info()`
- `std::cout << df`

Behavior:

- These use the display engine (auto width-aware truncation/summarization).
- `render=true` is default in APIs that support it.
- If you need a DataFrame result without immediate display side effects, pass `render=false`.

Example:

```cpp
df.head();           // renders
df.info();           // renders
auto t = df.head(5, false); // no immediate render
std::cout << t << "\n";
```

## 5. Feature/Case Access

Shape and labels:

```cpp
auto ncs = df.nrows();      // number of cases
auto nfeat = df.ncols();    // number of features
auto names = df.feature_names();
```

Access features:

```cpp
auto f = df.features("price");                // Thread
auto subset = df.features({"price", "qty"});  // DataFrame subset
```

Access case axis:

```cpp
auto case_axis = df.cases();           // Thread
auto one_case_df = df.cases(10);       // single case as DataFrame
auto case_slice = df.cases({5, 10});   // [5,10) as DataFrame
auto cell1 = df.cases(3, 2);           // row+feature-index cell (string view materialized to string)
auto cell2 = df.cases(3, "price");     // row+feature-name cell
```

Extract underlying pluss payload type:

```cpp
auto v = df.features("price").value(); // ThreadValue
// ThreadValue can be converted to Series/Note/Timeline/TimeIndex depending on actual kind.
```

Typed extraction helpers:

```cpp
auto s = df.series_value("price");
auto t = df.note_value("city");
auto dt = df.timeline_value("Updated Date");
```

Set/create features:

```cpp
// functional (returns a new DataFrame handle)
auto df2 = df.set_features("price_plus_1", df.features("price") + 1.0);

// in-place (mutates this DataFrame handle)
df.set_features_("price", df.features("price").clip(0.0, 1000.0));

// multi-feature assignment
auto df3 = df.set_features({"a_new", "b_new"}, {df.features("a"), df.features("b")});

// assignment sugar (in-place)
df.set("price_plus_2") = df.features("price") + 2.0;
df.set({"a_copy", "b_copy"}) = df.features({"a", "b"});
// rhs must be case-compatible with target DataFrame (same number of cases)
```

## 6. Type-Oriented Projections

Project DataFrame by high-level pluss type:

```cpp
auto numeric_df = df.series();
auto text_df = df.note();
auto datetime_df = df.timeline();
```

Cast features:

```cpp
auto c1 = df.astype({"price", "qty"}, pluss::codem::ScalarType::Float64);
auto c2 = df.cast_timeline({"Start_Time", "End_Time"});
auto c3 = df.cast_note({"Category"});
```

General cast:

```cpp
auto c = df.cast({
  {"Start_Time", pluss::table::ThreadCastTarget::Timeline, std::nullopt},
  {"price", pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Float64},
});
```

## 7. DataFrame Method Catalog (Current)

The API uses focused canonical names:

- **core naming**: `features`, `cases`, `sort_cases`, `tally`, `set_features`, `set`, `group_by`

Complete method index (current public surface, grouped):

- Access: `nrows`, `ncols`, `nfeatures`, `feature_names`, `features`, `cases`, `cases_series`, `iloc`, `at`, `iat`, `get`, `series_value`, `note_value`, `timeline_value`, `timeindex_value`
- Feature/case mutation: `set_features`, `set_features_`, `set`, `with_column`, `with_column_`, `with_cases`, `with_cases_`, `insert`, `drop`, `reorder`, `rename`, `rename_`, `rename_features`, `relabel_features`, `set_axis`, `set_feature_axis`, `set_cases`, `reset_cases`, `align_cases`, `reindex_cases`, `conform_to`, `reindex_like`
- Selection/filtering: `head`, `tail`, `view`, `sample`, `slice`, `take`, `where`, `filter`, `query`
- Derivation/aggregation/grouping: `assign`, `aggregate`, `group_by`, `pivot`, `pivot_table`, `count_by`, `mean_by`, `sum`, `mean`, `min`, `max`, `std`, `var`, `count`, `nunique`, `tally`, `describe`
- Sorting/ranking: `sort_values`, `sort_cases`, `top`, `bottom`
- Missing data: `isna`, `isnull`, `notna`, `notnull`, `fillna`, `fillna_`, `dropna`, `dropna_all`, `ffill`, `bfill`, `count_null_rows`
- Type operations: `series`, `note`, `timeline`, `cast_series`, `cast_note`, `cast_timeline`, `cast_timeindex`, `cast`, `astype`
- Window operations: `window`, `rolling`
- Combine/join: `join`, `update`, `combine`, `stitch_on`
- Arithmetic/comparison: `add`, `sub`, `subtract`, `mul`, `multiply`, `div`, `divide`, `truediv`, `floordiv`, `mod`, `pow`, `radd`, `rsub`, `rmul`, `rdiv`, `rtruediv`, `rfloordiv`, `rmod`, `rpow`, `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `compare`
- Structural/IO: `info`, `info_string`, `to_string`, `copy`, `equals`, `save_csv`, `save_excel`, `save_as`

### 7.1 Selection and indexing

- `head`, `tail`, `view`, `slice`, `take`, `sample`
- `iloc`, `at`, `iat`, `get`
- `features("name")`, `features({"a","b"})`
- `cases()`, `cases(i)`, `cases({i,k})`, `cases(i, j_or_name)`

### 7.2 projection/select

```cpp
auto a = df.features({"price", "qty"});
auto b = df.drop({"debug_col"});
auto c = df.reorder({"qty", "price", "city"});
```

### 7.3 query

`query(...)` is for boolean-expression filtering (pandas-style `df[mask_expr]` replacement).

```cpp
auto m = df.series_value("price").to(pluss::codem::ScalarType::Float64).ge(100.0);
auto q0 = df.filter(m); // mask-based filtering
auto q1 = df.query("price >= 100 and city != 'Unknown'");
auto q2 = df.query("contains(city, 'York')");
auto q3 = df.query("isnull(price)");
```

### 7.4 assign

```cpp
auto out = df.assign({
  {"is_high_price", [](const pluss::table::DataFrame& f) {
     return pluss::table::Thread(f.series_value("price").to(pluss::codem::ScalarType::Float64).ge(100.0));
   }},
});
```

### 7.5 aggregate + summary

```cpp
auto s = df.aggregate({
  {"rows", "price", pluss::table::DataFrame::SummaryOp::Count},
  {"avg_price", "price", pluss::table::DataFrame::SummaryOp::Mean},
  {"n_city", "city", pluss::table::DataFrame::SummaryOp::Nunique},
});
```

Fast summary helpers:

```cpp
auto s1 = df.sum();
auto s2 = df.mean();
auto s3 = df.min();
auto s4 = df.max();
auto s5 = df.std();
auto s6 = df.var();
auto s7 = df.count();
auto s8 = df.nunique();
auto s9 = df.tally("city");
auto s10 = df.describe();
```

### 7.6 group_by

```cpp
auto g = df.group_by(
  {"city"},
  {
    {"rows", "", pluss::table::DataFrame::GroupOp::Count},
    {"avg_price", "price", pluss::table::DataFrame::GroupOp::Mean},
    {"min_price", "price", pluss::table::DataFrame::GroupOp::Min},
  }
);
```

Convenience:

```cpp
auto c = df.count_by("city", /*top_n=*/10);
auto m = df.mean_by("city", "price", /*top_n=*/10, /*min_count=*/5);
auto p = df.pivot("state", "severity", "distance");
auto pt = df.pivot_table("state", "severity", "distance", pluss::table::DataFrame::GroupOp::Mean);
```

### 7.7 sorting

```cpp
auto a1 = df.sort_values({"price"});                  // ascending
auto a2 = df.sort_values({{"price", false, true}});   // descending, nulls last
auto a3 = df.sort_cases(false);                       // descending by case axis
auto a4 = df.top("price", 10);                        // top-10 by feature
auto a5 = df.bottom("price", 10);                     // bottom-10 by feature
```

### 7.8 missing-data methods

```cpp
auto n1 = df.isna();
auto n2 = df.notna({"price"});
auto n3 = df.fillna(0.0);
auto n4 = df.ffill();
auto n5 = df.bfill();
auto n6 = df.dropna();
auto n7 = df.dropna_all({"price", "qty"});
```

### 7.9 rolling/window

```cpp
auto r = df.rolling({
  {"price_roll_mean_3", "price", pluss::table::ThreadWindowOp::RollingMean, 3, 1},
}, true);

// lower-level window API is also available:
auto w = df.window({
  {"price_roll_sum_5", "price", pluss::table::ThreadWindowOp::RollingSum, 5, 1},
}, true);
```

### 7.10 combine

`combine` supports two primary operations:

- `Features`: append right-frame features (resolve collisions with suffix)
- `Update`: update overlapping features in-place semantics (functional result), optionally overwrite

```cpp
pluss::table::DataFrame::CombineSpec spec1;
spec1.op = pluss::table::DataFrame::CombineOp::Features;
spec1.collision_suffix = "_rhs";
auto both = left.combine(right, spec1);

pluss::table::DataFrame::CombineSpec spec2;
spec2.op = pluss::table::DataFrame::CombineOp::Update;
spec2.overwrite = true;
auto updated = left.combine(right, spec2);
```

Both frames must have identical case count.

### 7.11 indexing helpers

```cpp
auto row0 = df.iloc(0);
auto rows = df.iloc(10, 20);
auto v1 = df.iat(3, 2);
auto v2 = df.at(3, "price");
auto maybe_price = df.get("price");
auto boot = df.sample(100, 42);
auto dup = df.repeated_cases({"city"});
auto dedup = df.unique_cases({"city"});
auto cased = df.set_cases("city", true);
auto reset = cased.reset_cases("city_case");
auto aligned = reset.align_cases(reset.cases());
auto renamed = df.rename_features({{"price", "price_usd"}});
auto relabeled = df.relabel_features({"feat_a", "feat_b", "feat_c"});
auto relabeled2 = df.set_feature_axis({"feat_a", "feat_b", "feat_c"});
auto relabeled3 = df.set_axis({"feat_a", "feat_b", "feat_c"}); // alias
auto conformed = df.conform_to(other_df);
auto reindexed_like = df.reindex_like(other_df);
auto reindexed_cases = df.reindex_cases(other_df.cases());
auto stitched = left_df.stitch_on("id", right_df, "id", "_r");
auto stitched2 = left_df.stitch_on({"id", "bucket"}, right_df, {"id", "bucket"}, "_r");
```

## 8. Arithmetic And Comparison APIs

DataFrame provides vectorized wrappers that delegate to Thread/Series semantics where valid:

- Arithmetic: `add/sub/mul/div/truediv/floordiv/mod/pow`
- Reverse arithmetic: `radd/rsub/rmul/rdiv/rtruediv/rfloordiv/rmod/rpow`
- Comparison: `eq/ne/lt/le/gt/ge/compare`

Each supports scalar and (where defined) frame-to-frame forms.

Example:

```cpp
auto bumped = df.set_features("price", df.features("price") + 1.0);
auto mask_df = df.features({"price", "qty"}).gt(0.0);
```

## 9. Save APIs

### CSV

```cpp
df.save_csv("out.csv"); // defaults: add_cases=false, header=true, sep=','

df.save_csv(
    "out.csv",
    /*add_cases=*/false,
    /*header=*/true,
    /*sep=*/',',
    /*na_rep=*/"NA");
```

### Excel

```cpp
df.save_excel("out.xlsx"); // default sheet "Sheet1"
df.save_excel("out.xlsx", "Prices", /*add_cases=*/false, /*header=*/true);
```

### Unified save

```cpp
df.save_as("csv", "out.csv", {{"add_cases", false}});
df.save_as("excel", "out.xlsx", {{"sheet name", "Prices"}});
```

## 10. Atlas (3D strict cube)

`Atlas` is a stack of DataFrames with strict schema and size invariants:

- Same feature names/order across frames
- Same feature kinds/dtypes across frames
- Same number of cases across frames

Build:

```cpp
using pluss::table::Atlas;
using pluss::table::Thread;

std::vector<pluss::table::DataFrame> frames = {df_t0, df_t1, df_t2};
auto labels = Thread(pluss::Note({"2024-01", "2024-02", "2024-03"}));
Atlas at(std::move(frames), labels);
```

Access:

```cpp
auto d = at.depth();
auto f0 = at.frame(0);
auto f1 = at.iloc(1);
auto feb = at.loc("2024-02");
auto slice = at.slice_frames(0, 2);
auto first = at.head(2);
auto last = at.tail(2);
auto sorted = at.sort_depth(false);
auto relabeled_depth = at.relabel_depth(pluss::table::Thread(pluss::Note({"A", "B", "C"})));
auto aligned_depth = at.align_depth(pluss::table::Thread(pluss::Note({"2024-03", "2024-01"})));
```

Transform/reduce:

```cpp
auto shifted = at.map_frames([](const pluss::table::DataFrame& f) {
  return f.fillna(0.0);
});

auto combined = at.aggregate_over_frames([](const std::vector<pluss::table::DataFrame>& frames) {
  return frames.front(); // replace with custom reducer
});
```

Render:

```cpp
std::cout << at << "\n";
```

## 11. Practical Guidance

- Use `features` and `cases` terminology consistently in user code.
- Prefer focused methods (`query`, `assign`, `group_by`, `aggregate`, `sort_values`, `sort_cases`) over manual loops.
- Use `render=false` when composing display-returning methods programmatically.
- Keep heavy numeric/text/datetime operations at Thread-level semantics and let DataFrame orchestrate.

## 12. Current Gaps / Next Additions

Planned next major family:

- `reshape` (melt/pivot-style operations)

As new families/methods are added, update this guide in-place so end users always have one canonical DataFrame/Atlas reference.
