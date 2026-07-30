# Atlas::features

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
Atlas::features(const NameRange& features) -> template <detail::StringViewNameRange NameRange> Atlas

Atlas::features(pluss::RangeView<std::string_view> features) -> Atlas

Atlas::features(std::initializer_list<std::string_view> features) -> Atlas

Atlas::features(std::string_view feature) -> Atlas
```

## What It Does

Materializes the current members as a collection you can inspect or reuse elsewhere.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**feature**
    Name of the source feature used by the operation.

## Returns

`template <detail::StringViewNameRange NameRange> Atlas`
    Value produced by this call.

`Atlas`
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

auto out = atlas.features();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Work with a selected feature subset.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.features();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Pass several feature names inline.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.features();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 4. Work with one named feature.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.features();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="flow.html">
    <span class="label">Next</span>
    <span class="title">flow</span>
  </a>
</div>
