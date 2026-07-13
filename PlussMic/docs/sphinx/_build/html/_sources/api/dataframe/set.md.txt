# DataFrame::set

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::set(pluss::RangeView<std::string_view> names) -> LetSlots

DataFrame::set(std::initializer_list<const char*> names) -> LetSlots

DataFrame::set(std::initializer_list<std::string_view> names) -> LetSlots

DataFrame::set(std::string_view name) -> LetSlot
```

## What It Does

Opens an assignment helper so you can write `df.set(...) = ...` and replace or create features declaratively.

## Parameters

**names**
    Collection of feature, slot, or stream names supplied in positional order.

**name**
    Name used to identify the target object, field, feature, or slot for this overload.

## Returns

`LetSlots`
    Value produced by this call.

`LetSlot`
    Value produced by this call.

## Examples

Example 1. Open the multi-feature assignment helper from a reusable feature-name range.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set("profit") = pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}));
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Open the multi-feature assignment helper from inline C-string names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set({"profit", "cost"}) = {pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})), pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0}))};
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Open the multi-feature assignment helper from inline string-view names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set({"profit", "cost"}) = {pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})), pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0}))};
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 4. Open the assignment helper for one feature name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set("profit") = pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}));
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series_features.html">
    <span class="label">Previous</span>
    <span class="title">series_features</span>
  </a>
  <a class="next-link" href="set_axis.html">
    <span class="label">Next</span>
    <span class="title">set_axis</span>
  </a>
</div>
