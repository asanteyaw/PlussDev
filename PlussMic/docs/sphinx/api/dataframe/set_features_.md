# DataFrame::set_features_

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::set_features_(pluss::RangeView<std::string_view> names, pluss::RangeView<Thread> values) -> DataFrame&

DataFrame::set_features_(std::string_view name, const Thread& value) -> DataFrame&

DataFrame::set_features_(std::string_view name, const Thread& value, const AlignOptions& options) -> DataFrame&
```

## What It Does

Adds or replaces one feature in place.

## Parameters

**names**
    Collection of feature, slot, or stream names supplied in positional order.

**values**
    Value collection supplied to this overload. The ordering must match the corresponding names or positions expected by the method.

**name**
    Name used to identify the target object, field, feature, or slot for this overload.

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`DataFrame&`
    Value produced by this call.

## Examples

Example 1. Add or replace several columns in one call.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set_features_(
    {"profit", "cost"},
    {
        pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})),
        pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0})),
    });
```

### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```
Example 2. Add or replace one column by name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set_features_("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```
Example 3. Add or replace one column by name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set_features_("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_features.html">
    <span class="label">Previous</span>
    <span class="title">set_features</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>
