# ColumnRef API Reference

# Practical ColumnRef API Reference

## API Reference Style

This page documents the public `ColumnRef` helper returned by DataFrame column-reference calls.

### `head`

#### Signature

```cpp
Thread head(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Returns the first `n` values from the referenced column as a thread.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.head(2);
```

#### Output

```text
[100.0, 120.0]
```

### `mean`

#### Signature

```cpp
pluss::Series mean(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Computes the mean of the referenced numeric column.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.mean();
```

#### Output

```text
103.3
```

### `name`

#### Signature

```cpp
const std::string& name(self) const
```

#### Parameters

`None`

#### Returns

`const std::string&`
    Value produced by this call.

#### Notes

Returns the feature name associated with this column reference.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.name();
```

#### Output

```text
sales
```

### `thread`

#### Signature

```cpp
const Thread& thread(self) const
```

#### Parameters

`None`

#### Returns

`const Thread&`
    Value produced by this call.

#### Notes

Returns the referenced column as a `Thread` object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.thread();
```

#### Output

```text
[10.0, 20.0, 30.0]
```

### `value`

#### Signature

```cpp
ThreadValue value(self) const
```

#### Parameters

`None`

#### Returns

`ThreadValue`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.value();
```

#### Output

```text
100.0
```

### `values`

#### Signature

```cpp
Thread values(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.values();
```

#### Output

```text
[10.0, 20.0, 30.0]
```
