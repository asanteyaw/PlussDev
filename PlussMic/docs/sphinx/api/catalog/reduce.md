# Catalog::reduce

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::reduce(const std::vector<std::string>& ops, const CatalogReduceOptions& options = {}) -> Catalog

Catalog::reduce(std::initializer_list<std::string_view> ops, const CatalogReduceOptions& options = {}) -> Catalog
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**ops**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`Catalog`
    Value produced by this call.

## Examples

Example 1. Pass several names together from a standard string container.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.reduce();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass several feature names inline.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.reduce();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="query.html">
    <span class="label">Previous</span>
    <span class="title">query</span>
  </a>
  <a class="next-link" href="rename.html">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>
