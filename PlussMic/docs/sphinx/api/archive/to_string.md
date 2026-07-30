# Archive::to_string

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::to_string() -> std::string
```

## What It Does

Serializes or renders the current object into a text form you can inspect, log, or export.

## Parameters

`None`

## Returns

`std::string`
    Rendered string result.

## Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.to_string();
```

### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="to_table.html">
    <span class="label">Next</span>
    <span class="title">to_table</span>
  </a>
</div>
