# DataFrame::title

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::title() -> std::string

DataFrame::title(std::string title) -> DataFrame
```

## What It Does

Gets the current frame title or returns a titled copy, depending on the overload.

## Parameters

**title**
    Human-readable title stamp to attach to the returned frame.

## Returns

`std::string`
    Rendered string result.

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Return a titled copy of the current frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto titled = df.title("sales_report");
auto out = titled.title();
```

### Output

```text
sales_report
```
Example 2. Return a titled copy of the current frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.title("sales_report");
```

### Output

```text
out.title():
sales_report

out.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

## Practical Behavior

- `title()` with no argument reads the current frame title.
- `title("...")` returns a titled copy and leaves the source frame unchanged.
- `title_("...")` mutates the current frame title in place.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timeline_features.html">
    <span class="label">Previous</span>
    <span class="title">timeline_features</span>
  </a>
  <a class="next-link" href="title_.html">
    <span class="label">Next</span>
    <span class="title">title_</span>
  </a>
</div>
