# DataFrame::label

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::label() -> DataFrameLabelAccessor
```

Return the DataFrame label machinery for inspecting and manipulating row labels.

`label()` is PlussMic's public case-label surface. It centralizes the operations that are equivalent to pandas index machinery while keeping the word “case” available for the full row concept in Pluss data modelling.

Use this surface instead of root-level `caselbl*` methods. The label accessor keeps case-label operations in one place:

- `df.label().values()` returns the current case-label `Thread`.
- `df.label().names()` returns structured label component names.
- `df.label().has()` reports whether structured label metadata is present.
- `df.label().part(name)` or `df.label().part(position)` extracts one structured label component.
- `df.label().set(...)` promotes one or more features, or an explicit `Thread`, into case labels.
- `df.label().reset(...)` materializes labels back into ordinary features.
- `df.label().sort(...)` sorts rows by label values.
- `df.label().align(...)` rebuilds the frame against a target label order.
- `df.label().rename(...)`, `order(...)`, and `drop(...)` edit structured label metadata.
- `df.label().repeated(...)` returns a duplicate-label mask.

## Parameters

`None`

## Returns

`DataFrameLabelAccessor`
    Lightweight label accessor bound to the current frame.

    Methods that inspect labels return `Thread`, `std::vector<std::string>`, `bool`, or duplicate masks as appropriate. Methods that transform labels return a new `DataFrame`; the source frame is not mutated.

## Notes

The label surface is about row labels only. Feature names still live on `df.features()`, `df.rename(...)`, `df.reorder(...)`, and related feature-axis methods.

`label().values()` is the direct equivalent of the old DataFrame case-label getter. Row selection remains on `case_study(...)`.

`label().set(feature, true)` promotes a feature into the case-label axis and drops that feature from ordinary columns. Pass `false` to preserve the source feature as data.

Structured labels are useful for hierarchical case axes: region/account/quarter, date/symbol, campus/term, and similar multi-part row identifiers.

## Examples

Example 1. Inspect the current case-label thread.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0})},
});

auto out = sales.label().values();
```

### Output

```text
out:
0
1
2
```

Example 2. Promote one feature into the label axis.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0})},
});

auto out = sales.label().set("city", true);
```

### Output

```text
================
       | units
----------------
Accra  | 10
Tema   | 7
Kumasi | 12
----------------
[3 cs x 1 feat]
```

Example 3. Build a structured label from multiple features.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"campus", pluss::make_note({"north", "north", "south", "south"})},
    {"term", pluss::make_note({"fall", "spring", "fall", "spring"})},
    {"score", pluss::make_series({80.0, 82.0, 78.0, 88.0})},
});

auto out = scores.label().set({"campus", "term"}, true);
auto names = out.label().names();
```

### Output

```text
names:
campus
term
```

Example 4. Align to an explicit label order.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0})},
}).label().set("city", true);

auto out = sales.label().align(Thread(pluss::make_note({"Tema", "Missing", "Accra"})));
```

### Output

```text
================
        | units
----------------
Tema    | 7
Missing | NA
Accra   | 10
----------------
[3 cs x 1 feat]
```

Example 5. Rename, reorder, and drop structured label parts.

```cpp
using namespace pluss::table;

auto frame = DataFrame({
    {"region", pluss::make_note({"west", "west", "east", "east"})},
    {"account", pluss::make_series({101.0, 102.0, 201.0, 202.0})},
    {"quarter", pluss::make_note({"Q1", "Q2", "Q1", "Q2"})},
    {"sales", pluss::make_series({10.0, 20.0, 30.0, 40.0})},
}).label().set({"region", "account", "quarter"}, true);

auto out = frame.label()
    .rename({{"account", "acct"}})
    .label().order({"region", "quarter", "acct"})
    .label().drop({"quarter"});
```

### Output

```text
out.label().names():
region
acct
```

Example 6. Reset labels back into ordinary features.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0})},
}).label().set("city", true);

auto out = sales.label().reset("city_case");
```

### Output

```text
=======================
  | city_case  units
-----------------------
0 | Accra      10
1 | Tema       7
2 | Kumasi     12
-----------------------
[3 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kurtosis.html">
    <span class="label">Previous</span>
    <span class="title">kurtosis</span>
  </a>
  <a class="next-link" href="land.html">
    <span class="label">Next</span>
    <span class="title">land</span>
  </a>
</div>
