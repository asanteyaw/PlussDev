# DataFrame::put

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::put(std::string_view feature, const pluss::Series& bool_mask, const char* value) -> DataFrame

DataFrame::put(std::string_view feature, const pluss::Series& bool_mask, const Thread& value) -> DataFrame

DataFrame::put(std::string_view feature, const pluss::Series& bool_mask, double value) -> DataFrame

DataFrame::put(std::string_view feature, const pluss::Series& bool_mask, pluss::null_t value) -> DataFrame

DataFrame::put(std::string_view feature, const pluss::Series& bool_mask, std::string_view value) -> DataFrame

DataFrame::put(std::string_view feature, const Thread& bool_mask, const char* value) -> DataFrame

DataFrame::put(std::string_view feature, const Thread& bool_mask, const char* value, const AlignOptions& options) -> DataFrame

DataFrame::put(std::string_view feature, const Thread& bool_mask, const Thread& value) -> DataFrame

DataFrame::put(std::string_view feature, const Thread& bool_mask, const Thread& value, const AlignOptions& options) -> DataFrame

DataFrame::put(std::string_view feature, const Thread& bool_mask, double value) -> DataFrame

DataFrame::put(std::string_view feature, const Thread& bool_mask, double value, const AlignOptions& options) -> DataFrame

DataFrame::put(std::string_view feature, const Thread& bool_mask, pluss::null_t value) -> DataFrame

DataFrame::put(std::string_view feature, const Thread& bool_mask, pluss::null_t value, const AlignOptions& options) -> DataFrame

DataFrame::put(std::string_view feature, const Thread& bool_mask, std::string_view value) -> DataFrame

DataFrame::put(std::string_view feature, const Thread& bool_mask, std::string_view value, const AlignOptions& options) -> DataFrame
```

## What It Does

Writes a replacement value only where a boolean mask is true and returns the updated frame.

## Parameters

**feature**
    Feature name to rewrite.

**bool_mask**
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

**value**
    Replacement scalar, string, null, or thread written only where the mask is `true`.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Rewrite selected rows using a boolean Series mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
auto out = df.put("city", mask, "Tamale");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 2. Rewrite selected rows using a boolean Series mask and another full Thread as the replacement values.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
auto repl = pluss::table::Thread(pluss::make_series({500.0, 999.0, 500.0}));
auto out = df.put("sales", mask, repl);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 3. Rewrite selected rows using a boolean Series mask and a scalar numeric replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
auto out = df.put("sales", mask, 999.0);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 4. Rewrite selected rows using a boolean Series mask and `pluss::null` to mark the matched cells as missing.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
auto out = df.put("sales", mask, pluss::null);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 5. Rewrite selected rows using a boolean Series mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
auto out = df.put("city", mask, std::string_view("Tamale"));
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 6. Rewrite selected rows using a boolean Thread mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("city", mask, "Tamale");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 7. Rewrite selected rows using a boolean Thread mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("city", mask, "Tamale");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 8. Rewrite selected rows using a boolean Thread mask and another full Thread as the replacement values.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto repl = pluss::table::Thread(pluss::make_series({500.0, 999.0, 500.0}));
auto out = df.put("sales", mask, repl);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 9. Rewrite selected rows using a boolean Thread mask and another full Thread as the replacement values.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto repl = pluss::table::Thread(pluss::make_series({500.0, 999.0, 500.0}));
auto out = df.put("sales", mask, repl);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 10. Rewrite selected rows using a boolean Thread mask and a scalar numeric replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("sales", mask, 999.0);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 11. Rewrite selected rows using a boolean Thread mask and a scalar numeric replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("sales", mask, 999.0);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 12. Rewrite selected rows using a boolean Thread mask and `pluss::null` to mark the matched cells as missing.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("sales", mask, pluss::null);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 13. Rewrite selected rows using a boolean Thread mask and `pluss::null` to mark the matched cells as missing.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("sales", mask, pluss::null);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 14. Rewrite selected rows using a boolean Thread mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("city", mask, std::string_view("Tamale"));
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 15. Rewrite selected rows using a boolean Thread mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("city", mask, std::string_view("Tamale"));
```

### Output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```

## Practical Behavior

- `put(feature, mask, value)` rewrites only the rows where the mask evaluates true.
- The mask must match the frame case count.
- Use `put(...)` when you want a new frame result, and `put_(...)` when you want the same rewrite in place.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ptypes.html">
    <span class="label">Previous</span>
    <span class="title">ptypes</span>
  </a>
  <a class="next-link" href="put_.html">
    <span class="label">Next</span>
    <span class="title">put_</span>
  </a>
</div>
