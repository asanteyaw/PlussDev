# DataFrame::info

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::info(const display::DisplayOptions& options = {}) -> void

DataFrame::info(std::ostream& os, const display::DisplayOptions& options = {}) -> void
```

## What It Does

This renders a human-readable view for inspection instead of creating a new analytic object.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**os**
    Destination output stream used instead of the default display or print target.

## Returns

`void`
    Performs the side effect and returns no value.

## Examples

Example 1. Print the frame summary with the default output route.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.info();
```

### Output

```text
rows: 3
features: 3
numeric: id, sales
note: city
```
Example 2. Write the frame summary to a caller-provided output stream.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::ostringstream os;
df.info(os);
```

### Output

```text
rows: 3
features: 3
numeric: id, sales
note: city
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="height.html">
    <span class="label">Previous</span>
    <span class="title">height</span>
  </a>
  <a class="next-link" href="info_string.html">
    <span class="label">Next</span>
    <span class="title">info_string</span>
  </a>
</div>
