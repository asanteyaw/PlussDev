# DataFrame::feature_ref

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::feature_ref(std::string_view name) -> ColumnRef
```

## What It Does

Returns a lightweight reference object for one feature so you can inspect metadata or switch into column-oriented operations without materializing a new frame.

## Parameters

**name**
    Name used to identify the target object, field, feature, or slot for this overload.

## Returns

`ColumnRef`
    Value produced by this call.

## Examples

Example 1. Open a reusable column reference without copying the feature into a new frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.feature_ref("sales");
```

### Output

```text
ColumnRef(name="sales")
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="factor.html">
    <span class="label">Previous</span>
    <span class="title">factor</span>
  </a>
  <a class="next-link" href="features.html">
    <span class="label">Next</span>
    <span class="title">features</span>
  </a>
</div>
