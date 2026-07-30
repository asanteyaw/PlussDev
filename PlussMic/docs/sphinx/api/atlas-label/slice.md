# AtlasLabelAccessor::slice

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::slice(std::string_view start, std::string_view stop) -> Thread
```

## What It Does

Returns a smaller preview cut from the current object so you can inspect rows or depth items without carrying the full payload forward.

## Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Inspect one contiguous positional slice from the current object.

```cpp
auto q1 = pluss::table::DataFrame({"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))});
auto q2 = pluss::table::DataFrame({"sales", pluss::table::Thread(pluss::make_series({110.0, 90.0}))});
auto atlas = pluss::table::Atlas({q1, q2}, pluss::table::Thread(pluss::make_note({"2024Q1", "2024Q2"})));
auto labels = atlas.label();

auto out = labels.slice("units", "units");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set.html">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="sort.html">
    <span class="label">Next</span>
    <span class="title">sort</span>
  </a>
</div>
