# DataFrame::nfeats

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::nfeats() -> std::int64_t
```

## What It Does

Returns the number of features in the current frame.

## Parameters

`None`

## Returns

`std::int64_t`
    Integer count, size, or positional result.

## Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.nfeats();
```

### Output

```text
3
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ne.html">
    <span class="label">Previous</span>
    <span class="title">ne</span>
  </a>
  <a class="next-link" href="nfeatures.html">
    <span class="label">Next</span>
    <span class="title">nfeatures</span>
  </a>
</div>
