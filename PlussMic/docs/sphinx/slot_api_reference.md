# Slot API Reference

# Practical Slot API Reference

## API Reference Style

This page documents the full public `Slot` surface for one-dimensional heterogeneous bundles.

### `as_kindle`

#### Signature

```cpp
pluss::kindle::Stream as_kindle(self) const
```

#### Parameters

`None`

#### Returns

`pluss::kindle::Stream`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.as_kindle();
```

#### Output

```text
Series([10.0, 20.0, 30.0])
```

### `at`

#### Signature

```cpp
const Value& at(self, std::int64_t i) const
```

#### Parameters

`i` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`const Value&`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.at(0);
```

#### Output

```text
Series(1)
```

### `concat`

#### Signature

```cpp
Slot concat(self, const Slot& other) const
```

#### Parameters

`other` : `const Slot&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

Appends the values from another slot and returns the combined slot.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto other = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
auto out = slot.concat(other);
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
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
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.defined();
```

#### Output

```text
true
```

### `empty`

#### Signature

```cpp
bool empty(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Reports whether the current object has no rows, entries, slots, or depth items.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.empty();
```

#### Output

```text
false
```

### `filter`

#### Signature

```cpp
Slot filter(self, const std::function<bool(const Value&)>& predicate) const
```

#### Parameters

`predicate` : `const std::function<bool(const Value&)>&`
    See the signature for the supported argument form.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

Keeps only the slot values that satisfy the supplied predicate.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("ok"), pluss::make_series(3)});

auto out = slot.filter([](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `from_kindle`

#### Signature

```cpp
static Slot from_kindle(const pluss::kindle::Stream& stream)
```

#### Parameters

`stream` : `const pluss::kindle::Stream&`
    See the signature for the supported argument form.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = pluss::bundle::Slot::from_kindle(stream);
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `head`

#### Signature

```cpp
Slot head(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

Returns the first `n` values from the slot.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.head(2);
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `kindelize`

#### Signature

```cpp
pluss::kindle::Stream kindelize(self) const
```

#### Parameters

`None`

#### Returns

`pluss::kindle::Stream`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.kindelize();
```

#### Output

```text
Series([10.0, 20.0, 30.0])
```

### `push`

#### Signature

```cpp
Slot push(self, const Value& value) const
```

#### Parameters

`value` : `const Value&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

Returns a new slot with one additional value appended.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.push(pluss::make_note("tail"));
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `push_`

#### Signature

```cpp
Slot& push_(self, const Value& value)
```

#### Parameters

`value` : `const Value&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Returns

`Slot&`
    Value produced by this call.

#### Notes

Appends one additional value to the slot in place.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

slot.push_(pluss::make_note("tail"));
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `show`

#### Signature

```cpp
void show(self, std::ostream& os) const
```

#### Parameters

`os` : `std::ostream&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

slot.show(std::cout);
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `size`

#### Signature

```cpp
std::int64_t size(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Notes

Returns a size/count summary for the current object without changing it.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.size();
```

#### Output

```text
3
```

### `slice`

#### Signature

```cpp
Slot slice(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

Returns a contiguous positional slice `[start, stop)` from the slot.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.slice(0, 2);
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `tail`

#### Signature

```cpp
Slot tail(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

Returns the last `n` values from the slot.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.tail(2);
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `take`

#### Signature

```cpp
Slot take(self, std::span<const std::int64_t> indices) const
```

#### Parameters

`indices` : `std::span<const std::int64_t>`
    See the signature for the supported argument form.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

Returns the values at the requested integer positions.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

std::vector<std::int64_t> picks{0, 2};
auto out = slot.take(picks);
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `to_json`

#### Signature

```cpp
std::string to_json(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.to_json();
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `to_kindle`

#### Signature

```cpp
pluss::kindle::Stream to_kindle(self) const
```

#### Parameters

`None`

#### Returns

`pluss::kindle::Stream`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.to_kindle();
```

#### Output

```text
Series([10.0, 20.0, 30.0])
```

### `to_string`

#### Signature

```cpp
std::string to_string(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.to_string();
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `transform`

#### Signature

```cpp
Slot transform(self, const std::function<Value(const Value&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Value(const Value&)>&`
    See the signature for the supported argument form.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

Applies a value-wise transform to each slot entry and returns the updated slot.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.transform([](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `type_counts`

#### Signature

```cpp
std::unordered_map<std::string, std::int64_t> type_counts(self) const
```

#### Parameters

`None`

#### Returns

`std::unordered_map<std::string, std::int64_t>`
    Value produced by this call.

#### Notes

Counts how many slot entries belong to each stored pluss type.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("ok"), pluss::make_series(3)});

auto out = slot.type_counts();
```

#### Output

```text
{"Series": 2, "Note": 1}
```

### `unique`

#### Signature

```cpp
bool unique(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.unique();
```

#### Output

```text
true
```

### `values`

#### Signature

```cpp
const std::vector<Value>& values(self) const
```

#### Parameters

`None`

#### Returns

`const std::vector<Value>&`
    Value produced by this call.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Examples

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.values();
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
