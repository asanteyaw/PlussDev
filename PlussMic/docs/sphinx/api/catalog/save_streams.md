# Catalog::save_streams

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
void Catalog::save_streams(std::string_view directory, std::string_view store_kind = "parquet")
```

## What It Does

Writes the current object to disk using the corresponding PlussMic saver route.

## Parameters

**directory**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**store_kind**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`void`
    Performs the side effect and returns no value.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.save_streams();
```

### Output

```text
saved: out.*
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_parquet.html">
    <span class="label">Previous</span>
    <span class="title">save_parquet</span>
  </a>
  <a class="next-link" href="save_txt.html">
    <span class="label">Next</span>
    <span class="title">save_txt</span>
  </a>
</div>
