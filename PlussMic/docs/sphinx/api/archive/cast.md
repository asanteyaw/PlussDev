# Archive::cast

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::cast(pluss::table::ThreadCastTarget target, const pluss::table::ThreadCastOptions& options = {}) -> Archive
```

## What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

## Parameters

**target**
    Target payload family chosen by this cast overload.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`Archive`
    Value produced by this call.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.cast();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="catalog.html">
    <span class="label">Next</span>
    <span class="title">catalog</span>
  </a>
</div>
