# Catalog::load_streams

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::load_streams(std::string_view directory) -> Catalog
```

## What It Does

Loads the object from disk using the corresponding PlussMic loader route.

## Parameters

**directory**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Catalog`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto out = pluss::kindle::Catalog::load_streams("path/to/data.csv");
```

### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_parquet.html">
    <span class="label">Previous</span>
    <span class="title">load_parquet</span>
  </a>
  <a class="next-link" href="load_txt.html">
    <span class="label">Next</span>
    <span class="title">load_txt</span>
  </a>
</div>
