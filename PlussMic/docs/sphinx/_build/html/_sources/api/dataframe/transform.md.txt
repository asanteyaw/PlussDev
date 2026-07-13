# DataFrame::transform

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::transform(const std::function<DataFrame(const DataFrame&)>& fn, Axis axis, ResultShape shape = ResultShape::Infer) -> DataFrame

DataFrame::transform(const std::function<Thread(const Thread&)>& fn) -> DataFrame

DataFrame::transform(const std::function<Thread(const Thread&)>& fn, Axis axis) -> DataFrame

DataFrame::transform(const std::function<ThreadValue(const DataFrame&)>& fn, Axis axis) -> Thread

DataFrame::transform(pluss::RangeView<TransformSpec> specs) -> DataFrame

DataFrame::transform(std::string_view feature, const std::function<Thread(const DataFrame&)>& fn) -> DataFrame

DataFrame::transform(std::string_view feature, const std::function<Thread(const Thread&)>& fn) -> DataFrame
```

## What It Does

Rewrites one feature, several features, or all features by applying a user function to each targeted thread.

## Parameters

**fn**
    Thread-to-thread or frame-to-thread transform function, depending on the overload. The returned thread must match the frame case count.

**axis**
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

**shape**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**specs**
    Collection of structured operation specs. Each element describes one requested summary, derive, transform, group, or placement rule depending on the method.

**feature**
    Target feature name to rewrite, or output feature name when you use the frame-aware derive overload.

## Returns

`DataFrame`
    Value produced by this call.

`Thread`
    Value produced by this call.

## Examples

Example 1. Rewrite one named feature with a thread-level transform.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });
```

### Output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```
Example 2. Rewrite every feature with the same thread-level transform.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });
```

### Output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```
Example 3. Rewrite every feature with the same thread-level transform.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });
```

### Output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```
Example 4. Rewrite one named feature with a thread-level transform.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });
```

### Output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```
Example 5. Rewrite one or more selected features with structured transform specs.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform({{.feature = "sales", .expr = [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); }}});
```

### Output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```
Example 6. Derive or replace one feature by computing it from the full frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform("sales_plus_10", [](const pluss::table::DataFrame& frame) { return pluss::table::Thread(frame.features("sales").as_series().add(10.0)); });
```

### Output

```text
id  sales  city    sales_plus_10
1   100.0  Accra   110.0
2   120.0  Tema    130.0
3    90.0  Kumasi  100.0
```
Example 7. Rewrite one named feature with a thread-level transform.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform("sales", [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); });
```

### Output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```

## Practical Behavior

- `transform(fn)` rewrites every feature with the same thread-level function.
- `transform(feature, thread_fn)` rewrites one existing feature in place within the returned frame.
- `transform(feature, frame_fn)` is the derive-style overload: it can create a new feature from the whole frame, as long as the returned thread matches the frame case count.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="top.html">
    <span class="label">Previous</span>
    <span class="title">top</span>
  </a>
  <a class="next-link" href="transpose.html">
    <span class="label">Next</span>
    <span class="title">transpose</span>
  </a>
</div>
