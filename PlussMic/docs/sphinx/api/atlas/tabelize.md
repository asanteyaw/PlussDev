# Atlas::tabelize

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
Atlas::tabelize(const pluss::kindle::Archive& archive) -> Atlas

Atlas::tabelize(const pluss::kindle::Archive& archive, const pluss::kindle::TabelizePolicy& policy) -> Atlas
```

## What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

## Parameters

**archive**
    Archive payload supplied to this overload.

**policy**
    Policy object that changes the structural behavior of this overload.

## Returns

`Atlas`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;
auto c0 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0}))});
auto c1 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0}))});
auto archive = pluss::kindle::Archive({c0, c1});
auto out = pluss::table::Atlas::tabelize(archive);
```

### Output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;
auto c0 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0}))});
auto c1 = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0}))});
auto archive = pluss::kindle::Archive({c0, c1});
auto out = pluss::table::Atlas::tabelize(archive);
```

### Output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="squeeze.html">
    <span class="label">Previous</span>
    <span class="title">squeeze</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
