# ThreadValue API Reference

# Practical ThreadValue API Reference

## API Reference Style

This page documents the public scalar-like `ThreadValue` wrapper returned by thread and column-value accessors.

### `as_note`

#### Signature

```cpp
ThreadValue::as_note() -> pluss::Note
```

#### Notes

Converts the wrapped scalar-like value into a `pluss::Note`.

#### Inputs

`None`

#### Output

`pluss::Note`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra"}));
auto value = thread.value();

auto out = value.as_note();
```

#### Expected output

```text
["Accra"]
```

### `as_series`

#### Signature

```cpp
ThreadValue::as_series() -> pluss::Series
```

#### Notes

Converts the wrapped scalar-like value into a `pluss::Series`.

#### Inputs

`None`

#### Output

`pluss::Series`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.as_series();
```

#### Expected output

```text
[10.0]
```

### `as_timeindex`

#### Signature

```cpp
ThreadValue::as_timeindex() -> pluss::TimeIndex
```

#### Notes

Converts the wrapped scalar-like value into a `pluss::TimeIndex`.

#### Inputs

`None`

#### Output

`pluss::TimeIndex`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z"}));
auto value = thread.value();

auto out = value.as_timeindex();
```

#### Expected output

```text
["2024-01-01T09:10:00Z"]
```

### `as_timeline`

#### Signature

```cpp
ThreadValue::as_timeline() -> pluss::Timeline
```

#### Notes

Converts the wrapped scalar-like value into a `pluss::Timeline`.

#### Inputs

`None`

#### Output

`pluss::Timeline`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z"}));
auto value = thread.value();

auto out = value.as_timeline();
```

#### Expected output

```text
["2024-01-01T09:10:00Z"]
```

### `defined`

#### Signature

```cpp
ThreadValue::defined() -> bool
```

#### Notes

Reports whether the current object owns a concrete payload that can be used safely in later operations.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.defined();
```

#### Expected output

```text
true
```

### `kind`

#### Signature

```cpp
ThreadValue::kind() -> ThreadKind
```

#### Notes

Returns the underlying thread kind carried by this value wrapper.

#### Inputs

`None`

#### Output

`ThreadKind`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.kind();
```

#### Expected output

```text
Series
```

### `payload`

#### Signature

```cpp
ThreadValue::payload() -> Payload
```

#### Notes



#### Inputs

`None`

#### Output

`Payload`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.payload();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `pluss::Note`

#### Signature

```cpp
ThreadValue::pluss::Note() -> operator
```

#### Notes

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Note`.

#### Inputs

`None`

#### Output

`operator`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.pluss::Note();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `pluss::Series`

#### Signature

```cpp
ThreadValue::pluss::Series() -> operator
```

#### Notes

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Series`.

#### Inputs

`None`

#### Output

`operator`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.pluss::Series();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `pluss::TimeIndex`

#### Signature

```cpp
ThreadValue::pluss::TimeIndex() -> operator
```

#### Notes

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::TimeIndex`.

#### Inputs

`None`

#### Output

`operator`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.pluss::TimeIndex();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `pluss::Timeline`

#### Signature

```cpp
ThreadValue::pluss::Timeline() -> operator
```

#### Notes

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Timeline`.

#### Inputs

`None`

#### Output

`operator`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.pluss::Timeline();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `thread`

#### Signature

```cpp
ThreadValue::thread() -> const Thread&
```

#### Notes

Returns the wrapped single-value `Thread` object.

#### Inputs

`None`

#### Output

`const Thread&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.thread();
```

#### Expected output

```text
[10.0]
```
