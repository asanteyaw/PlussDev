# Catalog::clean

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::clean(std::string_view field) -> Catalog
```

## What It Does

Cleans one named text stream and returns the updated catalog.

## Parameters

**field**
    Name of the target stream in the catalog.

## Returns

`Catalog`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.clean("text");
```

### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cast.html">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="clear_join_cache.html">
    <span class="label">Next</span>
    <span class="title">clear_join_cache</span>
  </a>
</div>
