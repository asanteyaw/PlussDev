# Slot API Reference

# Practical Slot API Reference

## API Reference Style

This page documents the full public `Slot` surface for one-dimensional heterogeneous bundles.

### `as_kindle`

#### Signature

```cpp
Slot::as_kindle() -> pluss::kindle::Stream
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`None`

#### Output

`pluss::kindle::Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.as_kindle();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `at`

#### Signature

```cpp
Slot::at(i: std::int64_t) -> const Value&
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`i` : `std::int64_t`
    Zero-based positional index used by this overload.

#### Output

`const Value&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.at(0);
```

#### Expected output

```text
Series(1)
```

### `concat`

#### Signature

```cpp
Slot::concat(other: const Slot&) -> Slot
```

#### Notes

Appends the values from another slot and returns the combined slot.

#### Inputs

`other` : `const Slot&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`Slot`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto other = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
auto out = slot.concat(other);
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `defined`

#### Signature

```cpp
Slot::defined() -> bool
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
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.defined();
```

#### Expected output

```text
true
```

### `empty`

#### Signature

```cpp
Slot::empty() -> bool
```

#### Notes

Reports whether the current object has no rows, entries, slots, or depth items.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.empty();
```

#### Expected output

```text
false
```

### `filter`

#### Signature

```cpp
Slot::filter(predicate: const std::function<bool(const Value&)>&) -> Slot
```

#### Notes

Keeps only the slot values that satisfy the supplied predicate.

#### Inputs

`predicate` : `const std::function<bool(const Value&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Slot`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("ok"), pluss::make_series(3)});

auto out = slot.filter([](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `from_kindle`

#### Signature

```cpp
Slot::from_kindle(stream: const pluss::kindle::Stream&) -> Slot
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`stream` : `const pluss::kindle::Stream&`
    Stream payload supplied to this overload.

#### Output

`Slot`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = pluss::bundle::Slot::from_kindle(stream);
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `head`

#### Signature

```cpp
Slot::head(n: std::int64_t) -> Slot
```

#### Notes

Returns the first `n` values from the slot.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Slot`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.head(2);
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `kindelize`

#### Signature

```cpp
Slot::kindelize() -> pluss::kindle::Stream
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`None`

#### Output

`pluss::kindle::Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.kindelize();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `push`

#### Signature

```cpp
Slot::push(value: const Value&) -> Slot
```

#### Notes

Returns a new slot with one additional value appended.

#### Inputs

`value` : `const Value&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Slot`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.push(pluss::make_note("tail"));
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `push_`

#### Signature

```cpp
Slot::push_(value: const Value&) -> Slot&
```

#### Notes

Appends one additional value to the slot in place.

#### Inputs

`value` : `const Value&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Slot&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

slot.push_(pluss::make_note("tail"));
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `show`

#### Signature

```cpp
Slot::show(os: std::ostream&) -> void
```

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Inputs

`os` : `std::ostream&`
    Destination output stream used instead of the default display or print target.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

slot.show(std::cout);
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `size`

#### Signature

```cpp
Slot::size() -> std::int64_t
```

#### Notes

Returns a size/count summary for the current object without changing it.

#### Inputs

`None`

#### Output

`std::int64_t`
    Integer count, size, or positional result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.size();
```

#### Expected output

```text
3
```

### `slice`

#### Signature

```cpp
Slot::slice(start: std::int64_t, stop: std::int64_t) -> Slot
```

#### Notes

Returns a contiguous positional slice `[start, stop)` from the slot.

#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Output

`Slot`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.slice(0, 2);
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `tail`

#### Signature

```cpp
Slot::tail(n: std::int64_t) -> Slot
```

#### Notes

Returns the last `n` values from the slot.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Slot`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.tail(2);
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `take`

#### Signature

```cpp
Slot::take(indices: std::span<const std::int64_t>) -> Slot
```

#### Notes

Returns the values at the requested integer positions.

#### Inputs

`indices` : `std::span<const std::int64_t>`
    Integer positions used to select, reorder, or address entries.

#### Output

`Slot`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

std::vector<std::int64_t> picks{0, 2};
auto out = slot.take(picks);
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `to_json`

#### Signature

```cpp
Slot::to_json() -> std::string
```

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Inputs

`None`

#### Output

`std::string`
    Rendered string result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.to_json();
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `to_kindle`

#### Signature

```cpp
Slot::to_kindle() -> pluss::kindle::Stream
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`None`

#### Output

`pluss::kindle::Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.to_kindle();
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `to_string`

#### Signature

```cpp
Slot::to_string() -> std::string
```

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Inputs

`None`

#### Output

`std::string`
    Rendered string result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.to_string();
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `transform`

#### Signature

```cpp
Slot::transform(fn: const std::function<Value(const Value&)>&) -> Slot
```

#### Notes

Applies a value-wise transform to each slot entry and returns the updated slot.

#### Inputs

`fn` : `const std::function<Value(const Value&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Slot`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.transform([](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `type_counts`

#### Signature

```cpp
Slot::type_counts() -> std::unordered_map<std::string, std::int64_t>
```

#### Notes

Counts how many slot entries belong to each stored pluss type.

#### Inputs

`None`

#### Output

`std::unordered_map<std::string, std::int64_t>`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("ok"), pluss::make_series(3)});

auto out = slot.type_counts();
```

#### Expected output

```text
{"Series": 2, "Note": 1}
```

### `unique`

#### Signature

```cpp
Slot::unique() -> bool
```

#### Notes

Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.unique();
```

#### Expected output

```text
true
```

### `values`

#### Signature

```cpp
Slot::values() -> const std::vector<Value>&
```

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Inputs

`None`

#### Output

`const std::vector<Value>&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.values();
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```
