# Registry::freq

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::freq(pluss::kindle::FreqOptions options = {}) -> Registry
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`Registry`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.freq();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter_values_.html">
    <span class="label">Previous</span>
    <span class="title">filter_values_</span>
  </a>
  <a class="next-link" href="from_kindle.html">
    <span class="label">Next</span>
    <span class="title">from_kindle</span>
  </a>
</div>
