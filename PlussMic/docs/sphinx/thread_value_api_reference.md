# ThreadValue API Reference

# Practical ThreadValue API Reference

## API Reference Style

This page documents the public scalar-like `ThreadValue` wrapper returned by thread and column-value accessors.

### `as_note`

#### Signature

```cpp
pluss::Note as_note(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Note`
    Value produced by this call.

#### Notes

Converts the wrapped scalar-like value into a `pluss::Note`.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_note({"Accra"}));
auto value = thread.value();

auto out = value.as_note();
```

#### Output

```text
["Accra"]
```

### `as_series`

#### Signature

```cpp
pluss::Series as_series(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Converts the wrapped scalar-like value into a `pluss::Series`.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.as_series();
```

#### Output

```text
[10.0]
```

### `as_timeindex`

#### Signature

```cpp
pluss::TimeIndex as_timeindex(self) const
```

#### Parameters

`None`

#### Returns

`pluss::TimeIndex`
    Value produced by this call.

#### Notes

Converts the wrapped scalar-like value into a `pluss::TimeIndex`.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z"}));
auto value = thread.value();

auto out = value.as_timeindex();
```

#### Output

```text
["2024-01-01T09:10:00Z"]
```

### `as_timeline`

#### Signature

```cpp
pluss::Timeline as_timeline(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Timeline`
    Value produced by this call.

#### Notes

Converts the wrapped scalar-like value into a `pluss::Timeline`.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z"}));
auto value = thread.value();

auto out = value.as_timeline();
```

#### Output

```text
["2024-01-01T09:10:00Z"]
```

### `defined`

#### Signature

```cpp
bool defined(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Reports whether the current object owns a concrete payload that can be used safely in later operations.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.defined();
```

#### Output

```text
true
```

### `kind`

#### Signature

```cpp
ThreadKind kind(self) const
```

#### Parameters

`None`

#### Returns

`ThreadKind`
    Value produced by this call.

#### Notes

Returns the underlying thread kind carried by this value wrapper.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.kind();
```

#### Output

```text
Series
```

### `payload`

#### Signature

```cpp
Payload payload(self) const
```

#### Parameters

`None`

#### Returns

`Payload`
    Value produced by this call.

#### Notes

Applies `payload` to the current `ThreadValue` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.payload();
```

### `pluss::Note`

#### Signature

```cpp
operator pluss::Note(self) const
```

#### Parameters

`None`

#### Returns

`operator`
    Value produced by this call.

#### Notes

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Note`.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.pluss::Note();
```

### `pluss::Series`

#### Signature

```cpp
operator pluss::Series(self) const
```

#### Parameters

`None`

#### Returns

`operator`
    Value produced by this call.

#### Notes

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Series`.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.pluss::Series();
```

### `pluss::TimeIndex`

#### Signature

```cpp
operator pluss::TimeIndex(self) const
```

#### Parameters

`None`

#### Returns

`operator`
    Value produced by this call.

#### Notes

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::TimeIndex`.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.pluss::TimeIndex();
```

### `pluss::Timeline`

#### Signature

```cpp
operator pluss::Timeline(self) const
```

#### Parameters

`None`

#### Returns

`operator`
    Value produced by this call.

#### Notes

Uses the implicit conversion operator to materialize the wrapped value as a `pluss::Timeline`.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.pluss::Timeline();
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

Returns the wrapped single-value `Thread` object.

#### Examples

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0}));
auto value = thread.value();

auto out = value.thread();
```

#### Output

```text
[10.0]
```
