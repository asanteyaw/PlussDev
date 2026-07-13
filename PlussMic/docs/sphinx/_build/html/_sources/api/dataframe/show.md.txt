# DataFrame::show

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::show(const display::DisplayOptions& options = {}) -> void

DataFrame::show(std::ostream& os, const display::DisplayOptions& options = {}) -> void
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

Example 1. Render the frame using the default display route.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.show();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Render the frame to a caller-provided output stream.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::ostringstream os;
df.show(os);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html">
    <span class="label">Previous</span>
    <span class="title">shift</span>
  </a>
  <a class="next-link" href="shuffle.html">
    <span class="label">Next</span>
    <span class="title">shuffle</span>
  </a>
</div>
