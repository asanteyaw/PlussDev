# Archive::rename

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
Archive::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) -> Archive

Archive::rename(std::string_view from, std::string_view to) -> Archive
```

## What It Does

Changes names or labels only. The underlying payload values, row order, and aligned structure stay the same.

## Parameters

**mapping**
    Inline collection passed directly at the call site.

**from**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

**to**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

## Returns

`Archive`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.rename();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.rename();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nrecords.html">
    <span class="label">Previous</span>
    <span class="title">nrecords</span>
  </a>
  <a class="next-link" href="select.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>
