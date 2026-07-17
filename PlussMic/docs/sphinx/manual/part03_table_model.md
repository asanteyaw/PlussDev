# Part III: The Table Model

This part explains how `Thread`, `DataFrame`, `DataFrameView`, and `Atlas`
relate. Understanding these types makes the rest of PlussMic predictable.

## Thread

A `Thread` is one typed column. It can hold one of four payload kinds:

```cpp
pluss::table::Thread numeric(pluss::make_series({1.0, 2.0, 3.0}));
pluss::table::Thread text(pluss::make_note({"red", "green", "blue"}));
pluss::table::Thread dates(pluss::make_timeline({
    "2024-01-01T00:00:00Z",
    "2024-01-02T00:00:00Z",
    "2024-01-03T00:00:00Z",
}));
```

Inspect a thread:

```cpp
thread.defined();
thread.size();
thread.kind();
thread.dtype();
thread.info();
```

Access the payload only after you know the kind:

```cpp
if (thread.kind() == pluss::table::ThreadKind::Series) {
  auto values = thread.as_series();
}
```

## Missingness and validity

`Thread` can carry a validity mask. Loader-created columns use this to represent
missing values from input files.

```cpp
auto missing = thread.isnull();
auto present = thread.notnull();
auto filled = thread.fillnull(0.0);
auto complete = thread.dropnull();
```

Numeric threads also support `isnan` and `isfinite`.

## DataFrame

A `DataFrame` is a set of named `Thread` features with an optional case axis.

```cpp
auto df = pluss::table::DataFrame({
    {"region", pluss::make_note({"north", "south"})},
    {"sales", pluss::make_series({10.0, 12.5})},
});
```

Shape and schema:

```cpp
df.defined();
df.ncases();
df.nfeatures();
df.features();
df.info();
```

### Verified example: building a small frame

The following code creates two text features and two numeric features:

```cpp
using pluss::table::DataFrame;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

std::cout << sales.to_string() << '\n';
std::cout << "ncases=" << sales.ncases()
          << ", nfeatures=" << sales.nfeatures() << '\n';
std::cout << "features:";
for (const auto& name : sales.features()) {
  std::cout << ' ' << name;
}
std::cout << '\n';
```

Output:

```text
===================================
     | city    region  units  price
-----------------------------------
0    | Accra   south   10     3
1    | Tema    south   7      4
2    | Kumasi  middle  12     2.5
3    | Tamale  north   5      5
-----------------------------------
[4 cs x 4 feat]
ncases=4, nfeatures=4
features: city region units price
```

This table is the exact string returned by `sales.to_string()` in the current
build. `head()` also renders a table because it returns a renderable
`DataFrameView`, but its implementation is not used to produce this output.
The display has four cases and four features. The default case axis is the
integer axis shown at the left. `features()` preserves insertion order:
`city`, `region`, `units`, then `price`.

Feature selection:

```cpp
auto sales = df.features("sales");                 // Thread
auto smaller = df.features({"region", "sales"});   // DataFrame
auto numeric = df.feataxis().series();
auto text = df.feataxis().note();
```

`features("name")` returns a `Thread` with display labels from the frame's case
axis and a display name matching the feature. `features({"a", "b"})` returns a
new `DataFrame` with the same case axis and the selected feature order.

## Cases

The case axis is the row label/index axis. It may be default integer labels or
loaded from a feature.

```cpp
auto cases = df.label().values();
auto first_case = df.case_study(0);
auto first_ten = df.case_study(0, 10);

auto indexed = df.label().set("id", true);
auto reset = indexed.label().reset("id");
```

`label().set("id", true)` uses `id` as the case axis and drops it from features.
Pass `false` to keep the feature as well.

## DataFrameView

Some methods return `DataFrameView`:

```cpp
df.head();
df.tail();
df.describe();
df.aggregate({{"mean_sales", "sales", DataFrame::SummaryOp::Mean}});
```

`DataFrameView` is still a `DataFrame`, but it also knows how to auto-render a
preview in terminal or popup contexts. If you need a materialized `DataFrame`,
pass `render=false` where the method supports it:

```cpp
pluss::table::DataFrame first_ten = df.head(10, false);
```

## Atlas

An `Atlas` stacks related frames:

```cpp
auto january = df.query("month == 1");
auto february = df.query("month == 2");

pluss::table::Atlas atlas({january, february});
```

Use an `Atlas` for:

- repeated experiments;
- scenario outputs;
- time-sliced frames;
- multi-sheet workbook-like data;
- 3D plotting and depth aggregation.

## Conversion model

PlussMic can convert between layers:

```cpp
auto catalog = df.kindelize();
auto roundtrip_df = catalog.tabelize();

auto archive = atlas.kindelize();
auto roundtrip_atlas = pluss::table::Atlas::tabelize(archive);

auto registry = pluss::bundle::Registry::from_kindle(catalog);
auto back_catalog = registry.to_kindle();
```

Use these conversions when you need a different data model for a specific part
of your workflow.
