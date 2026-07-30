# Archive::catalogs

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::catalogs() -> const std::vector<Catalog>&
```

## What It Does

Materializes the current members as a collection you can inspect or reuse elsewhere.

## Parameters

`None`

## Returns

`const std::vector<Catalog>&`
    Borrowed collection of catalogs.

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

auto out = archive.catalogs();
```

### Output

```text
[Catalog(train), Catalog(test)]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="catalog.html">
    <span class="label">Previous</span>
    <span class="title">catalog</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
