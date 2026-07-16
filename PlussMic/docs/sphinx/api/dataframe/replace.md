# DataFrame::replace

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::replace(const char* from, const char* to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(const char* from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(const pluss::Note& from, const pluss::Note& to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(const pluss::Note& from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(const pluss::Series& from, const pluss::Series& to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(const pluss::Series& from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(const pluss::TimeIndex& from, const pluss::TimeIndex& to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(const pluss::TimeIndex& from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(const pluss::Timeline& from, const pluss::Timeline& to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(const pluss::Timeline& from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(double from, double to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(double from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(pluss::null_t from, const char* to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(pluss::null_t from, const pluss::Note& to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(pluss::null_t from, const pluss::Series& to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(pluss::null_t from, const pluss::TimeIndex& to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(pluss::null_t from, const pluss::Timeline& to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(pluss::null_t from, double to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(pluss::null_t from, std::string_view to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(std::string_view from, pluss::null_t to, pluss::RangeView<std::string_view> features = {}) -> DataFrame

DataFrame::replace(std::string_view from, std::string_view to, pluss::RangeView<std::string_view> features = {}) -> DataFrame
```

## What It Does

Replaces matched scalar or pluss-typed values across selected compatible features and returns the updated frame.

## Parameters

**from**
    Value to search for inside the selected compatible features. Its type decides which feature kinds participate.

**to**
    Replacement value written anywhere `from` matches. Use `pluss::null` when you want matched cells to become missing.

**features**
    Optional feature subset. Leave empty to apply the replacement across every compatible feature of the relevant kind.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace("Tema", "Tamale", {"city"});
```

### Output

```text
midterm_1  midterm_2  city
0.0        8.0        Accra
12.0       0.0        Tamale
0.0        15.0       Tamale
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 3. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 4. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 5. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 6. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 7. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 8. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 9. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 10. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 11. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 12. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 13. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 14. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 15. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 16. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 17. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 18. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 19. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 20. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

### Output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 21. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace("Tema", "Tamale", {"city"});
```

### Output

```text
midterm_1  midterm_2  city
0.0        8.0        Accra
12.0       0.0        Tamale
0.0        15.0       Tamale
```

## Practical Behavior

- `replace(from, to, features)` dispatches by value type, so numeric replacements touch numeric series features while string replacements touch note features.
- If `features` is empty, every compatible feature of the relevant kind participates automatically.
- Use `pluss::null` as `to` when you want matched cells to become missing.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reorder.html">
    <span class="label">Previous</span>
    <span class="title">reorder</span>
  </a>
  <a class="next-link" href="reshuffle.html">
    <span class="label">Next</span>
    <span class="title">reshuffle</span>
  </a>
</div>
