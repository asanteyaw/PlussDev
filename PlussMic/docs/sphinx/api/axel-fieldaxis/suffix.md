# AxelFieldAxisAccessor::suffix

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
AxelFieldAxisAccessor::suffix(std::string_view token) -> Axel
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**token**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto fieldaxis = axel.fieldaxis();

auto out = fieldaxis.suffix("units");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
  <a class="next-link" href="timeindex.html">
    <span class="label">Next</span>
    <span class="title">timeindex</span>
  </a>
</div>
