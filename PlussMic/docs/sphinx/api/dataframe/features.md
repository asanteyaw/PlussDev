# DataFrame::features

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::features() -> const std::vector<std::string>&

DataFrame::features(const NameRange& names) -> DataFrame

DataFrame::features(const NameRange& names) -> DataFrame

DataFrame::features(pluss::RangeView<std::string_view> names) -> DataFrame

DataFrame::features(pluss::RangeView<std::string_view> names) -> DataFrame

DataFrame::features(StackArray<std::string_view> names) -> DataFrame

DataFrame::features(StackArray<std::string_view> names) -> DataFrame

DataFrame::features(std::initializer_list<const char*> names) -> DataFrame

DataFrame::features(std::initializer_list<const char*> names) -> DataFrame

DataFrame::features(std::initializer_list<std::string_view> names) -> DataFrame

DataFrame::features(std::initializer_list<std::string_view> names) -> DataFrame

DataFrame::features(std::string_view name) -> Thread

DataFrame::features(std::string_view name) -> Thread
```

## What It Does

Materializes the current members as a collection you can inspect or reuse elsewhere.

## Parameters

**names**
    Collection of feature, slot, or stream names supplied in positional order.

**name**
    Name used to identify the target object, field, feature, or slot for this overload.

## Returns

`const std::vector<std::string>&`
    Borrowed list of names.

`DataFrame`
    Value produced by this call.

`Thread`
    Value produced by this call.

## Examples

Example 1. Materialize every feature thread in left-to-right order.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features();
```

### Output

```text
[Thread(id), Thread(sales), Thread(city)]
```
Example 2. Project a named feature subset from a reusable name-range container.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features();
```

### Output

```text
[Thread(id), Thread(sales), Thread(city)]
```
Example 3. Project a named feature subset from a reusable name-range container.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features();
```

### Output

```text
[Thread(id), Thread(sales), Thread(city)]
```
Example 4. Project a named feature subset from a lightweight string view.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

### Output

```text
Thread([100.0, 120.0, 90.0])
```
Example 5. Project a named feature subset from a lightweight string view.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

### Output

```text
Thread([100.0, 120.0, 90.0])
```
Example 6. Project a named feature subset from a fixed stack-allocated name list.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

### Output

```text
Thread([100.0, 120.0, 90.0])
```
Example 7. Project a named feature subset from a fixed stack-allocated name list.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

### Output

```text
Thread([100.0, 120.0, 90.0])
```
Example 8. Project a named feature subset from inline C-string feature names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features({"id", "sales"});
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 9. Project a named feature subset from inline C-string feature names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features({"id", "sales"});
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 10. Project a named feature subset from inline string-view feature names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features({"id", "sales"});
```

### Output

```text
Thread([100.0, 120.0, 90.0])
```
Example 11. Project a named feature subset from inline string-view feature names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features({"id", "sales"});
```

### Output

```text
Thread([100.0, 120.0, 90.0])
```
Example 12. Extract one feature thread by name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

### Output

```text
Thread([100.0, 120.0, 90.0])
```
Example 13. Extract one feature thread by name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

### Output

```text
Thread([100.0, 120.0, 90.0])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="feataxis.html">
    <span class="label">Previous</span>
    <span class="title">feataxis</span>
  </a>
  <a class="next-link" href="ffill.html">
    <span class="label">Next</span>
    <span class="title">ffill</span>
  </a>
</div>
