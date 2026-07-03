# ColumnRef API Reference

# Practical ColumnRef API Reference

## API Reference Style

This page documents the public `ColumnRef` helper returned by DataFrame column-reference calls.

## H

### `head`

#### API Signature

```cpp
ColumnRef::head(n: std::int64_t) -> Thread
```

#### Notes

Returns the first `n` values from the referenced column as a thread.

#### Inputs

`n` : `std::int64_t`
    Number of leading values to preview from the referenced feature.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.head(2);
```

##### Expected output

```text
[100.0, 120.0]
```

## M

### `mean`

#### API Signature

```cpp
ColumnRef::mean() -> pluss::Series
```

#### Notes

Computes the mean of the referenced numeric column.

#### Inputs

`None`

#### Output

`pluss::Series`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.mean();
```

##### Expected output

```text
103.3
```

## N

### `name`

#### API Signature

```cpp
ColumnRef::name() -> const std::string&
```

#### Notes

Returns the feature name associated with this column reference.

#### Inputs

`None`

#### Output

`const std::string&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.name();
```

##### Expected output

```text
sales
```

## T

### `thread`

#### API Signature

```cpp
ColumnRef::thread() -> const Thread&
```

#### Notes

Returns the referenced column as a `Thread` object.

#### Inputs

`None`

#### Output

`const Thread&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.thread();
```

##### Expected output

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
ColumnRef::value() -> ThreadValue
```

#### Notes

Returns one scalar-like wrapper for the first referenced value, which is useful when you need typed single-cell access.

#### Inputs

`None`

#### Output

`ThreadValue`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.value();
```

##### Expected output

```text
100.0
```

#### Practical Behavior

- `value()` returns a `ThreadValue` wrapper over the first element of the referenced column.
- Use this when you need typed scalar extraction rather than a whole-column thread.


### `values`

#### API Signature

```cpp
ColumnRef::values() -> Thread
```

#### Notes

Materializes the full referenced column as a standalone `Thread` so you can reuse it outside the parent frame.

#### Inputs

`None`

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.feature_ref("sales");

auto out = ref.values();
```

##### Expected output

```text
[10.0, 20.0, 30.0]
```

#### Practical Behavior

- `values()` materializes the referenced feature as a standalone `Thread`.
- Use this when you want to pass the column into another API that expects a thread rather than a column reference.

