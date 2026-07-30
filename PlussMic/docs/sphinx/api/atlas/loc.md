# Atlas::loc

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
Atlas::loc(const pluss::Timeline& label) -> DataFrame

Atlas::loc(std::int64_t label) -> DataFrame

Atlas::loc(std::string_view label) -> DataFrame
```

## What It Does

Fetches one value, member, or slice from the current object without changing the source object.

## Parameters

**label**
    Label or labels used by this overload.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto labeled = pluss::table::Atlas({f0, f1}, pluss::table::Thread(pluss::make_note({"train", "test"})));
auto out = labeled.loc("test");
```

### Output

```text
sales
10.0
20.0
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto labeled = pluss::table::Atlas({f0, f1}, pluss::table::Thread(pluss::make_note({"train", "test"})));
auto out = labeled.loc("test");
```

### Output

```text
sales
10.0
20.0
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto labeled = pluss::table::Atlas({f0, f1}, pluss::table::Thread(pluss::make_note({"train", "test"})));
auto out = labeled.loc("test");
```

### Output

```text
sales
10.0
20.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="label.html">
    <span class="label">Previous</span>
    <span class="title">label</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
