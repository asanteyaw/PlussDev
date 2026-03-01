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
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.column_ref("sales");

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

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.column_ref("sales");

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

This call is part of the public `ColumnRef` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.column_ref("sales");

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

This call is part of the public `ColumnRef` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.column_ref("sales");

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

This call is part of the public `ColumnRef` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.column_ref("sales");

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

This call is part of the public `ColumnRef` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto ref = df.column_ref("sales");

auto out = ref.values();
```

#### Output

```text
[10.0, 20.0, 30.0]
```
