# Axel::has

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::has(std::string_view field) -> bool
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**field**
    Name of the target stream in the catalog.

## Returns

`bool`
    Boolean status or membership result.

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

auto out = axel.has();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_kindle.html">
    <span class="label">Previous</span>
    <span class="title">from_kindle</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
