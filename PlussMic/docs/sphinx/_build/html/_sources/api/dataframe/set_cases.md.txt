# DataFrame::set_cases

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::set_cases(const SetCasesOptions& options) -> DataFrame

DataFrame::set_cases(pluss::RangeView<std::string_view> feature_names, bool drop_features = false, std::string_view separator = "\x1F") -> DataFrame

DataFrame::set_cases(std::initializer_list<std::string_view> feature_names, bool drop_features = false, std::string_view separator = "\x1F") -> DataFrame

DataFrame::set_cases(std::string_view feature_name, bool drop_feature = false) -> DataFrame
```

## What It Does

Promotes one feature to become the case axis. With the default `drop_feature=true`, that feature stops being a normal column and becomes the row labels instead.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**feature_names**
    Feature name(s) supplied explicitly for subset, ordering, or axis-replacement operations, depending on the method.

**drop_features**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**separator**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**feature_name**
    Name of the target feature to read, rewrite, sort by, or promote to cases.

**drop_feature**
    When `true`, the feature promoted to cases is removed from the ordinary feature columns.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_cases("city");
```

### Output

```text
df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi

out.caselbl():
["Accra", "Tema", "Kumasi"]

out.head():
id  sales
1   100.0
2   120.0
3    90.0
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_cases("city");
```

### Output

```text
df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi

out.caselbl():
["Accra", "Tema", "Kumasi"]

out.head():
id  sales
1   100.0
2   120.0
3    90.0
```
Example 3. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_cases("city");
```

### Output

```text
df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi

out.caselbl():
["Accra", "Tema", "Kumasi"]

out.head():
id  sales
1   100.0
2   120.0
3    90.0
```
Example 4. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_cases("city");
```

### Output

```text
df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi

out.caselbl():
["Accra", "Tema", "Kumasi"]

out.head():
id  sales
1   100.0
2   120.0
3    90.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_axis.html">
    <span class="label">Previous</span>
    <span class="title">set_axis</span>
  </a>
  <a class="next-link" href="set_features.html">
    <span class="label">Next</span>
    <span class="title">set_features</span>
  </a>
</div>
