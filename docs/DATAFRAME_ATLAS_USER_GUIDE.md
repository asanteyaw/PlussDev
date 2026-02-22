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
auto numeric_df = df.series_features();
auto text_df = df.note_features();
auto datetime_df = df.timeline_features();
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
- Derivation/aggregation/grouping: `assign`, `transform`, `aggregate`, `group_by`, `pivot`, `pivot_table`, `count_by`, `mean_by`, `sum`, `mean`, `min`, `max`, `std`, `var`, `count`, `nunique`, `tally`, `describe`, `transpose`
- Dimensional bridge: `unsqueeze` (DataFrame -> depth-1 Atlas), `squeeze` (Atlas -> DataFrame when depth==1)
- Sorting/ranking: `sort_values`, `sort_cases`, `top`, `bottom`
- Missing data: `isna`, `isnull`, `notna`, `notnull`, `fillna`, `fillna_`, `dropna`, `dropna_all`, `ffill`, `bfill`, `count_null_rows`
- Type operations: `series`, `note`, `timeline`, `cast_series`, `cast_note`, `cast_timeline`, `cast_timeindex`, `cast`, `astype`
- Window operations: `window`, `rolling`
- Combine/join: `join`, `update`, `combine`, `stitch_on`
- Arithmetic/comparison: `add`, `sub`, `subtract`, `mul`, `multiply`, `div`, `divide`, `truediv`, `floordiv`, `mod`, `pow`, `radd`, `rsub`, `rmul`, `rdiv`, `rtruediv`, `rfloordiv`, `rmod`, `rpow`, `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `compare`
- Structural/IO: `info`, `info_string`, `to_string`, `copy`, `equals`, `transpose`, `save_csv`, `save_excel`, `save_as`

### 7.6 squeeze / unsqueeze

```cpp
auto a1 = df.unsqueeze();     // depth = 1 Atlas
auto df2 = a1.squeeze();      // requires depth == 1
```

With optional depth label:

```cpp
auto label = pluss::table::Thread(pluss::make_note({"batch_0"}, {1}));
auto a2 = df.unsqueeze(label);
```

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

### 7.4 assign / transform

```cpp
auto out = df.assign({
  {"is_high_price", [](const pluss::table::DataFrame& f) {
     return pluss::table::Thread(f.series_value("price").to(pluss::codem::ScalarType::Float64).ge(100.0));
   }},
});
```

```cpp
auto t0 = df.transform([](const pluss::table::Thread& f) {
  if (f.kind() == pluss::table::ThreadKind::Series) return f + 1.0;
  return f;
});

auto t1 = df.transform("price", [](const pluss::table::Thread& f) { return f.log(); });
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

## 13. Kindle Model (Stream -> Catalog -> Archive)

`pluss::kindle` is the unaligned keyed-stream model:

- `Stream`: one typed 1D payload (`Series` / `Note` / `Timeline` / `TimeIndex`) with optional case labels and validity mask.
- `Catalog`: named collection of `Stream` with varying lengths allowed.
- `Archive`: stack of `Catalog`.

Conversion:

```cpp
auto cat = df.kindelize();      // DataFrame -> Catalog
auto arc = atlas.kindelize();   // Atlas -> Archive

auto back_df = cat.tabelize();      // Catalog -> DataFrame
auto back_atlas = arc.tabelize();   // Archive -> Atlas (frame-by-frame tabelize)
auto tbl = cat.to_table();          // explicit table rendering path (returns DataFrame)
auto atl = arc.to_table();          // explicit table rendering path (returns Atlas)
```

`Archive::tabelize(policy)` applies the same `TabelizePolicy` to each `Catalog`, then builds a strict `Atlas`. If resulting frames do not satisfy Atlas invariants (schema/shape consistency), construction throws.

### Stream APIs (streamable ops live here)

Core:

```cpp
auto n = s.size();
auto k = s.kind();
auto t = s.to_thread();
```

Predicates/transforms:

```cpp
auto nulls = s.isnull();
auto nonnull = s.notnull();
auto s2 = s.fillna(0.0).clip(0.0, 100.0).log();
```

Row-space:

```cpp
auto h = s.head(5);
auto w = s.slice(10, 20);
auto kept = s.filter(mask_series);
auto clean = s.dropna();
```

Typed cast/reduction:

```cpp
auto s_num = s.cast(pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Float64);
auto mu = s.mean();  // scalar Stream (Series payload, numel == 1)
```

Arithmetic/comparison (delegates to Thread semantics):

```cpp
auto z = s + 1.0;
auto m = s >= 0.0;
```

### Catalog APIs (delegating/vectorized over streams)

```cpp
auto numeric = cat.series();
auto text = cat.note();
auto time = cat.timeline();

auto h = cat.head(5);
auto w = cat.slice(10, 20);

auto cat2 = cat.fillna(0.0);                    // default: Series streams
auto cat3 = cat.fillna(0.0, {"price", "qty"});  // explicit keys
auto cat4 = cat.dropna();                        // per-stream dropna
```

### Key/value construction syntax

Supported:

```cpp
using namespace pluss::kindle::literals;
pluss::kindle::Catalog c({
  "price"_k = price_stream,
  "city"_k = city_stream,
});
```

and standard pair form:

```cpp
pluss::kindle::Catalog c({
  {"price", price_stream},
  {"city", city_stream},
});
```

Not possible in C++:

```cpp
// "price" = price_stream
```

because string literals are not assignable lvalues.

### Kindle rendering

`Stream`, `Catalog`, and `Archive` render through `pluss::kindle::display` and print JSON-shaped output by default:

```cpp
std::cout << s << "\n";
std::cout << cat << "\n";
std::cout << arc << "\n";
```

`Catalog` output is pretty JSON with:

- top-level type/defined/nstreams
- key preview
- stream objects (`kind`, `size`, `dtype`, `cases`, `mask`, `preview`, optional `meta`)
- terminal-aware head/tail key truncation with `"..."` omission markers

`Archive` output is JSON summary with:

- depth and depth-label presence
- head/tail catalog summaries (index, optional label, nstreams, keys preview)
- omission marker when depth is large

To render with the table engine explicitly:

```cpp
cat.head(10).to_table(); // DataFrame table renderer
arc.to_table();          // Atlas table renderer
```

`Catalog::to_table()` does not require equal stream lengths. It uses `TabelizePolicy` alignment:
- `inner` (default): intersection of case domains
- `outer`: union of case domains
- `left` / `right`: anchor-based alignment
- `strict`: equal-domain enforcement

### Catalog methods (current)

Row/indexing:

```cpp
auto c1 = cat.head(10).tail(5);
auto c2 = cat.slice(100, 200);
auto c3 = cat.iloc(100, 200);
auto c4 = cat.filter(mask_series);
auto c5 = cat.masked_select(mask_series);
```

Field/structure:

```cpp
auto c_sel = cat.select({"price", "city"});
auto c_drop = cat.drop({"zipcode"});
auto c_ren = cat.rename({{"old_name", "new_name"}});
```

Sort/group/merge:

```cpp
auto c_s = cat.sort("price", false);
auto c_m = cat.sort_by({"state", "city"}, {true, true});
auto groups = cat.group_by("state"); // unordered_map<string, Catalog>
auto joined = cat.merge(other, "id", "inner");
```

Aggregations:

```cpp
auto s = cat.sum("price");
auto m = cat.mean("price");
auto n = cat.count();
```

Apply/text:

```cpp
auto c_ap = cat.apply("price", [](const pluss::kindle::Stream& s) { return s.fillna(0.0); });
auto c_tx = cat.text_preprocess("description");
auto c_kw = cat.extract_keywords("description", 20);
```

I/O and delegation policy:

```cpp
auto cat_csv = pluss::kindle::Catalog::load_csv("data.csv");
auto cat_xlsx = pluss::kindle::Catalog::load_excel("data.xlsx");
cat_csv.save_csv("out.csv");
```

`Catalog::load`, `Catalog::load_csv`, and `Catalog::load_excel` read through loader batch readers and construct `Catalog` directly (no DataFrame conversion step).

## 14. Bundle Model (Slot -> Registry -> Aisle)

`pluss::bundle` is the document-native model:

- `Slot`: 1D heterogeneous values (can mix `Series` / `Note` / `Timeline` / `TimeIndex` and scalar values).
- `Registry`: named collection of `Slot` with varying lengths.
- `Aisle`: stack of `Registry` with optional labels.

This differs from `kindle`:

- `kindle::Stream` is typed/homogeneous per stream.
- `bundle::Slot` is heterogeneous by design.

Example:

```cpp
using pluss::bundle::Slot;
using pluss::bundle::Registry;
using pluss::bundle::Aisle;

Slot items({
  std::int64_t{42},
  std::string{"hello"},
  pluss::make_series({1.0, 2.0, 3.0}),
});

Registry r({
  {"items", items},
  {"meta", Slot(std::vector<Slot::Value>{std::string{"doc-1"}, true})},
});

Aisle a({r});
```

Bridge policy:

- `Registry::kindelize()` exists but throws if a slot is not stream-compatible.
- `Aisle::kindelize()` delegates through each registry.
- No general `bundelize()` from table/signal: heterogeneous slots are not reliably rectangularizable without lossy coercion.

## 15. Plotting with PlussPl

`PlussPl` is a standalone plotting library under `pluss::plotism`.

`PlussPl` now has explicit 2D/3D modules:

- `Figure2D` (`line`, `scatter`, `bar`, `hist`, subplot layout, axis/legend/style)
- `Figure3D` (`line3`, `scatter3`, `surface`)

`PlussMic` provides DataFrame/Atlas wrappers on top of `plotism` (`df.plot()` style), separated from table core logic.

```cpp
#include <pluss/pluss.h>

// Direct PlussPl usage (no DataFrame required)
std::vector<double> x{0.0, 1.0, 2.0, 3.0};
std::vector<double> y{1.0, 2.0, 1.5, 3.5};
auto raw = pluss::plotism::line(x, y, "wave").with_title("Raw plotism line");
raw.show();
raw.save_svg("wave.svg"); // rendered graph file

std::vector<double> z{0.2, 0.4, 0.1, 0.8};
auto raw3 = pluss::plotism::line3(x, y, z, "wave3d").with_title("Raw 3D line");
raw3.save_svg("wave3d.svg");

auto df = pluss::table::DataFrame::load_csv("data.csv");

// PlussMic wrappers (pandas-like)
auto f1 = df.plot().line("price");
auto f2 = df.plot().scatter("x", "y");
auto f3 = df.plot().bar("count", "label");
auto f4 = df.plot().hist("price", 20);

f1.show();
std::string spec = f1.to_json(); // JSON plot spec for downstream renderers
f1.save_svg("price.svg");
```

Atlas slice plotting:

```cpp
pluss::table::Atlas atlas({df1, df2});
auto f = atlas.plot(0).line("price");
f.show();
```

Notes:

- Plotting accepts `Series`, `Timeline`/`TimeIndex`, and numeric-like `Note` values.
- `save_svg(...)` writes actual rendered graph files now.
- `save_png(...)` is intentionally backend-gated and throws unless a raster backend is wired in your build.
- `to_json()` remains the stable interchange surface for future GUI/web backends.
