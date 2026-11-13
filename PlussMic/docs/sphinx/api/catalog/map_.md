# Catalog::map_

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog& Catalog::map_(const std::function<Stream(const Stream&)>& fn)
```

## What It Does

Applies the same function to every stream in place.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`Catalog&`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.map_([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });
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
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="match_keys.html">
    <span class="label">Next</span>
    <span class="title">match_keys</span>
  </a>
</div>
