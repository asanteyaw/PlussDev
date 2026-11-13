# Catalog::combine

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::combine(std::string_view lhs, std::string_view rhs, std::string_view op, std::string_view out_key = {}, const RaggedPolicy& policy = RaggedPolicy::defaults(OpClass::Elementwise))
```

## What It Does

Use this when you need to merge compatible objects or extend the current one with another input.

## Parameters

**lhs**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**rhs**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**op**
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

**out_key**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**policy**
    Policy object that changes the structural behavior of this overload.

## Returns

`Catalog`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.combine();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cloc.html">
    <span class="label">Previous</span>
    <span class="title">cloc</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
