# ColumnRef API Reference

# Practical ColumnRef API Reference

## API Reference Style

This page documents the public `ColumnRef` helper returned by DataFrame column-reference calls.

## H

### `head`

#### API Signature

```cpp
Thread head(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Number of leading values to preview from the referenced feature.

#### Returns

`Thread`
    Value produced by this call.

#### What It Does

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

```text
[100.0, 120.0]
```


## M

### `mean`

#### API Signature

```cpp
pluss::Series mean(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### What It Does

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

```text
103.3
```


## N

### `name`

#### API Signature

```cpp
const std::string& name(self) const
```

#### Parameters

`None`

#### Returns

`const std::string&`
    Value produced by this call.

#### What It Does

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

```text
sales
```


## T

### `thread`

#### API Signature

```cpp
const Thread& thread(self) const
```

#### Parameters

`None`

#### Returns

`const Thread&`
    Value produced by this call.

#### What It Does

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

```text
[10.0, 20.0, 30.0]
```

#### Practical Behavior

- `thread()` gives you the exact referenced column payload without copying it into a new frame.
- Use this when you want to switch into `Thread` operations such as comparisons, casting, or text cleanup.


## V

### `value`

#### API Signature

```cpp
ThreadValue value(self) const
```

#### Parameters

`None`

#### Returns

`ThreadValue`
    Value produced by this call.

#### What It Does

Returns one scalar-like wrapper for the first referenced value, which is useful when you need typed single-cell access.

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

```text
100.0
```

#### Practical Behavior

- `value()` returns a `ThreadValue` wrapper over the first element of the referenced column.
- Use this when you need typed scalar extraction rather than a whole-column thread.


### `values`

#### API Signature

```cpp
Thread values(self) const
```

#### Parameters

`None`

#### Returns

`Thread`
    Value produced by this call.

#### What It Does

Materializes the full referenced column as a standalone `Thread` so you can reuse it outside the parent frame.

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

```text
[10.0, 20.0, 30.0]
```

#### Practical Behavior

- `values()` materializes the referenced feature as a standalone `Thread`.
- Use this when you want to pass the column into another API that expects a thread rather than a column reference.

