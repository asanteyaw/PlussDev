# DataFrame::top

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::top(std::string_view feature, std::int64_t n) -> DataFrame
```

## What It Does

Returns the rows with the largest values in the selected feature.

## Parameters

**feature**
    Name of the source feature used by the operation.

**n**
    Number of rows, entries, or depth items to keep in the preview.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.top("sales", 2);
```

### Output

```text
id  sales  city
2   120.0  Tema
1   100.0  Accra
```

## Practical Behavior

- `top(feature, n)` sorts descending by the selected feature, keeps nulls last, then returns the first `n` rows.
- The output rows stay aligned across every feature; this is not a single-column extraction.
- Use `bottom(...)` when you want the smallest rows instead of the largest ones.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="transform.html">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>
