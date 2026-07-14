# DataFrame::sample

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::sample(Float frac) -> template <class Float> requires std::floating_point<Float> DataFrame

DataFrame::sample(Float frac, bool replace, Axis axis) -> template <class Float> requires std::floating_point<Float> DataFrame

DataFrame::sample(Float frac, std::uint64_t random_state, bool replace = false, Axis axis = Axis::Case) -> template <class Float> requires std::floating_point<Float> DataFrame

DataFrame::sample(std::int64_t n) -> DataFrame

DataFrame::sample(std::int64_t n, bool replace, Axis axis) -> DataFrame

DataFrame::sample(std::int64_t n, std::uint64_t random_state, bool replace = false, Axis axis = Axis::Case) -> DataFrame
```

## What It Does

Returns a random sample of rows.

## Parameters

**frac**
    Fraction of the selected axis to draw. Values above `1.0` require `replace=true`.

**replace**
    When `true`, the same row may be drawn more than once. Feature sampling currently requires `replace=false`.

**axis**
    Sampling axis. Use `0` for rows and `1` for features.

**random_state**
    Deterministic seed for reproducible sampling and shuffling.

**n**
    Number of rows or features to draw from the selected axis.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Sample a fraction of the selected axis without an explicit random seed.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sample(0.67, false, 0);
```

### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 2. Sample a fraction of the selected axis without an explicit random seed.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sample(0.67, false, 0);
```

### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 3. Sample a fraction of the selected axis with an explicit random seed.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sample(0.67, 7, false, 0);
```

### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 4. Sample rows with the default settings.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sample(2);
```

### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 5. Sample rows with the default settings.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sample(2);
```

### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 6. Sample rows or features with an explicit random seed.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto rows = df.sample(2, 7, false, 0);
auto cols = df.sample(2, 7, false, 1);
```

### Output

```text
rows.head():
id  sales  city
1   100.0  Accra
3    90.0  Kumasi

cols.head():
sales  city
100.0  Accra
120.0  Tema
90.0   Kumasi
```

## Practical Behavior

- `sample(n, ...)` draws a fixed number of rows by default, or features when `axis=1`.
- `sample(frac, ...)` draws a proportion of the selected axis.
- `replace=true` enables sampling with replacement. For `frac > 1`, replacement is required.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rxor.html">
    <span class="label">Previous</span>
    <span class="title">rxor</span>
  </a>
  <a class="next-link" href="save_as.html">
    <span class="label">Next</span>
    <span class="title">save_as</span>
  </a>
</div>
