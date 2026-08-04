# Catalog::grep

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::grep(std::string_view field, std::string_view pattern) -> Catalog
```

## What It Does

Keeps only the rows where one named text stream matches the pattern.

## Parameters

**field**
    Name of the target stream in the catalog.

**pattern**
    Pattern string used for matching or extraction.

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

auto out = ct.grep("text", "rain");
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
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="group.html">
    <span class="label">Next</span>
    <span class="title">group</span>
  </a>
</div>
