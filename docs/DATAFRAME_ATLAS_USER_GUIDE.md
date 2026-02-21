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

- A DataFrame is a collection of **features** (columns).
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
auto f = df.feature("price");         // Thread
auto f2 = df.features("price");       // alias -> Thread
auto subset = df.features({"price", "qty"}); // DataFrame subset
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

## 7. Focused Methods (Current)

Primary end-user methods are focused and explicit:

- `filter` / `where` / `query`
- `assign`
- `aggregate` / `agg`
- `groupby`
- `sort_values` / `sort_cases` / `top` / `bottom`
- `isna` / `notna` / `fillna` / `dropna` / `ffill` / `bfill`
- `astype` / `cast_series` / `cast_note` / `cast_timeline` / `cast_timeindex`
- `rolling`
- `join` / `update` / `combine`
- `iloc` / `at` / `iat` / `get` / `sample`
- `repeated_cases` / `unique_cases` / `set_cases` / `reset_cases` / `align_cases`
- `rename_features` / `relabel_features` / `conform_to` / `stitch_on`
- `set_feature_axis` / `reindex_like` / `reindex_cases`

`reshape` methods are planned next.

### 7.1 projection/select

```cpp
auto a = df.features({"price", "qty"});
auto b = df.drop({"debug_col"});
auto c = df.reorder({"qty", "price", "city"});
```

### 7.2 query

`query(...)` is for boolean-expression filtering (pandas-style `df[mask_expr]` replacement).

```cpp
auto m = df.series_value("price").to(pluss::codem::ScalarType::Float64).ge(100.0);
auto q0 = df.filter(m); // mask-based filtering
auto q1 = df.query("price >= 100 and city != 'Unknown'");
auto q2 = df.query("contains(city, 'York')");
auto q3 = df.query("isnull(price)");
```

### 7.3 assign

```cpp
auto out = df.assign({
  {"is_high_price", [](const pluss::table::DataFrame& f) {
     return pluss::table::Thread(f.series_value("price").to(pluss::codem::ScalarType::Float64).ge(100.0));
   }},
});
```

### 7.4 aggregate

```cpp
auto s = df.aggregate({
  {"rows", "price", pluss::table::DataFrame::SummaryOp::Count},
  {"avg_price", "price", pluss::table::DataFrame::SummaryOp::Mean},
  {"n_city", "city", pluss::table::DataFrame::SummaryOp::Nunique},
});
```

`agg(...)` is an alias of `aggregate(...)`.

### 7.5 groupby

```cpp
auto g = df.groupby(
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
```

### 7.6 sort_values

```cpp
auto a1 = df.sort_values({"price"});                  // ascending
auto a2 = df.sort_values({{"price", false, true}});   // descending, nulls last
auto a3 = df.sort_cases(false);                       // descending by case axis
auto a4 = df.top("price", 10);                        // top-10 by feature
auto a5 = df.bottom("price", 10);                     // bottom-10 by feature
```

### 7.7 missing-data methods

```cpp
auto n1 = df.isna();
auto n2 = df.notna({"price"});
auto n3 = df.fillna(0.0);
auto n4 = df.ffill();
auto n5 = df.bfill();
auto n6 = df.dropna();
```

### 7.8 rolling/window

```cpp
auto r = df.rolling({
  {"price_roll_mean_3", "price", pluss::table::ThreadWindowOp::RollingMean, 3, 1},
}, true);

// lower-level window API is also available:
auto w = df.window({
  {"price_roll_sum_5", "price", pluss::table::ThreadWindowOp::RollingSum, 5, 1},
}, true);
```

### 7.9 combine

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

### 7.10 indexing helpers

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
auto conformed = df.conform_to(other_df);
auto reindexed_like = df.reindex_like(other_df);
auto reindexed_cases = df.reindex_cases(other_df.cases());
auto stitched = left_df.stitch_on("id", right_df, "id", "_r");
auto stitched2 = left_df.stitch_on({"id", "bucket"}, right_df, {"id", "bucket"}, "_r");
```

Feature-engineering note:

```cpp
// Chainable constant feature from a reduction:
auto distance_mean_log = df.feature("Distance(mi)").mean_like().log();
auto out = df.with_column("distance_mean_log", distance_mean_log);
```

## 8. Arithmetic And Comparison APIs

DataFrame provides vectorized wrappers that delegate to Thread/Series semantics where valid:

- Arithmetic: `add/sub/mul/div/truediv/floordiv/mod/pow`
- Reverse arithmetic: `radd/rsub/rmul/rdiv/rtruediv/rfloordiv/rmod/rpow`
- Comparison: `eq/ne/lt/le/gt/ge/compare`

Each supports scalar and (where defined) frame-to-frame forms.

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

- Use `feature`/`features` and `cases` terminology consistently in user code.
- Prefer focused methods (`query`, `assign`, `groupby`, `aggregate`, `sort_values`) over manual loops.
- Use `render=false` when composing display-returning methods programmatically.
- Keep heavy numeric/text/datetime operations at Thread-level semantics and let DataFrame orchestrate.

## 12. Current Gaps / Next Additions

Planned next major family:

- `reshape` (melt/pivot-style operations)

As new families/methods are added, update this guide in-place so end users always have one canonical DataFrame/Atlas reference.
