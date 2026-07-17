# DataFrame::factor

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::factor(std::string_view feature, bool ordered = false, bool sort_levels = false) -> CategoryEncoding
```

## What It Does

Factorizes selected features into compact integer codes together with the corresponding observed levels.

## Parameters

**feature**
    Name of the source feature used by the operation.

**ordered**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**sort_levels**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

## Returns

`CategoryEncoding`
    Value produced by this call.

## Examples

Example 1. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.factor();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extend.html">
    <span class="label">Previous</span>
    <span class="title">extend</span>
  </a>
  <a class="next-link" href="feataxis.html">
    <span class="label">Next</span>
    <span class="title">feataxis</span>
  </a>
</div>
