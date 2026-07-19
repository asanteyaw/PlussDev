# Slot API Reference

This page documents the full public `Slot` surface for one-dimensional heterogeneous bundles.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

## A

### `as_kindle`

#### Signature

```cpp
Slot::as_kindle() -> pluss::kindle::Stream
```

#### What It Does

Converts or reinterprets the current payload as another public Pluss type so downstream operations route through the right text, numeric, or temporal surface.

#### Parameters

`None`

#### Returns

`pluss::kindle::Stream`
    Value produced by this call.

#### Examples

Example 1. Convert the current object into the `kindle` representation before continuing with another type-specific workflow.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.as_kindle();
```

##### Output

```text
Series([10.0, 20.0, 30.0])
```
<div class="pluss-inline-prev-next">
  <a class="next-link" href="#at">
    <span class="label">Next</span>
    <span class="title">at</span>
  </a>
</div>

### `at`

#### Signature

```cpp
Slot::at(std::int64_t i) -> const Value&
```

#### What It Does

Fetches one value, member, or slice from the current object without changing the source object.

#### Parameters

**i**
    Zero-based positional index used by this overload.

#### Returns

`const Value&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.at(0);
```

##### Output

```text
Series(1)
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#as_kindle">
    <span class="label">Previous</span>
    <span class="title">as_kindle</span>
  </a>
  <a class="next-link" href="#concat">
    <span class="label">Next</span>
    <span class="title">concat</span>
  </a>
</div>

## C

### `concat`

#### Signature

```cpp
Slot::concat(const Slot& other) -> Slot
```

#### What It Does

Appends the values from another slot and returns the combined slot.

#### Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto other = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
auto out = slot.concat(other);
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#at">
    <span class="label">Previous</span>
    <span class="title">at</span>
  </a>
  <a class="next-link" href="#defined">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>

## D

### `defined`

#### Signature

```cpp
Slot::defined() -> bool
```

#### What It Does

Reports whether the current object owns a concrete payload that can be used safely in later operations.

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Inspect the `defined` property of the current object.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.defined();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#concat">
    <span class="label">Previous</span>
    <span class="title">concat</span>
  </a>
  <a class="next-link" href="#empty">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>

## E

### `empty`

#### Signature

```cpp
Slot::empty() -> bool
```

#### What It Does

Reports whether the current object has no rows, entries, slots, or depth items.

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Inspect the `empty` property of the current object.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.empty();
```

##### Output

```text
false
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#defined">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="#filter">
    <span class="label">Next</span>
    <span class="title">filter</span>
  </a>
</div>

## F

### `filter`

#### Signature

```cpp
Slot::filter(const std::function<bool(const Value&)>& predicate) -> Slot
```

#### What It Does

Keeps only the slot values that satisfy the supplied predicate.

#### Parameters

**predicate**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("ok"), pluss::make_series(3)});

auto out = slot.filter([](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#empty">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="#from_kindle">
    <span class="label">Next</span>
    <span class="title">from_kindle</span>
  </a>
</div>

### `from_kindle`

#### Signature

```cpp
Slot::from_kindle(const pluss::kindle::Stream& stream) -> Slot
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

**stream**
    Stream payload supplied to this overload.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = pluss::bundle::Slot::from_kindle(stream);
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#filter">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="#head">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>

## H

### `head`

#### Signature

```cpp
Slot::head(std::int64_t n) -> Slot
```

#### What It Does

Returns the first `n` values from the slot.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Inspect the first part of the current object without changing the source data.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.head(2);
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#from_kindle">
    <span class="label">Previous</span>
    <span class="title">from_kindle</span>
  </a>
  <a class="next-link" href="#kindelize">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>

## K

### `kindelize`

#### Signature

```cpp
Slot::kindelize() -> pluss::kindle::Stream
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

`None`

#### Returns

`pluss::kindle::Stream`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.kindelize();
```

##### Output

```text
Series([10.0, 20.0, 30.0])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#head">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="#ptype">
    <span class="label">Next</span>
    <span class="title">ptype</span>
  </a>
</div>

## P

### `ptype`

#### Signature

```cpp
Slot::ptype() -> std::string
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.ptype();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#kindelize">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="#push">
    <span class="label">Next</span>
    <span class="title">push</span>
  </a>
</div>

### `push`

#### Signature

```cpp
Slot::push(const Value& value) -> Slot
```

#### What It Does

Returns a new slot with one additional value appended.

#### Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.push(pluss::make_note("tail"));
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#ptype">
    <span class="label">Previous</span>
    <span class="title">ptype</span>
  </a>
  <a class="next-link" href="#push_">
    <span class="label">Next</span>
    <span class="title">push_</span>
  </a>
</div>

### `push_`

#### Signature

```cpp
Slot::push_(const Value& value) -> Slot&
```

#### What It Does

Appends one additional value to the slot in place.

#### Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

#### Returns

`Slot&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

slot.push_(pluss::make_note("tail"));
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#push">
    <span class="label">Previous</span>
    <span class="title">push</span>
  </a>
  <a class="next-link" href="#shape">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>

## S

### `shape`

#### Signature

```cpp
Slot::shape() -> std::pair<std::int64_t, std::int64_t>
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`std::pair<std::int64_t, std::int64_t>`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.shape();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#push_">
    <span class="label">Previous</span>
    <span class="title">push_</span>
  </a>
  <a class="next-link" href="#show">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>

### `show`

#### Signature

```cpp
Slot::show(std::ostream& os) -> void
```

#### What It Does

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Parameters

**os**
    Destination output stream used instead of the default display or print target.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Write the output to a caller-provided stream.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

slot.show(std::cout);
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#shape">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="#size">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>

### `size`

#### Signature

```cpp
Slot::size() -> std::int64_t
```

#### What It Does

Returns a size/count summary for the current object without changing it.

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Examples

Example 1. Inspect the `size` property of the current object.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.size();
```

##### Output

```text
3
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#show">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="#slice">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>

### `slice`

#### Signature

```cpp
Slot::slice(std::int64_t start, std::int64_t stop) -> Slot
```

#### What It Does

Returns a contiguous positional slice `[start, stop)` from the slot.

#### Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Inspect one contiguous positional slice from the current object.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.slice(0, 2);
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#size">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="#tail">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>

## T

### `tail`

#### Signature

```cpp
Slot::tail(std::int64_t n) -> Slot
```

#### What It Does

Returns the last `n` values from the slot.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Inspect the last part of the current object without changing the source data.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.tail(2);
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#slice">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="#take">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>

### `take`

#### Signature

```cpp
Slot::take(std::span<const std::int64_t> indices) -> Slot
```

#### What It Does

Returns the values at the requested integer positions.

#### Parameters

**indices**
    Integer positions used to select, reorder, or address entries.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Pass several integer positions together.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

std::vector<std::int64_t> picks{0, 2};
auto out = slot.take(picks);
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#tail">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="#to_json">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>

### `to_json`

#### Signature

```cpp
Slot::to_json() -> std::string
```

#### What It Does

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.to_json();
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#take">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="#to_kindle">
    <span class="label">Next</span>
    <span class="title">to_kindle</span>
  </a>
</div>

### `to_kindle`

#### Signature

```cpp
Slot::to_kindle() -> pluss::kindle::Stream
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

`None`

#### Returns

`pluss::kindle::Stream`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.to_kindle();
```

##### Output

```text
Series([10.0, 20.0, 30.0])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_json">
    <span class="label">Previous</span>
    <span class="title">to_json</span>
  </a>
  <a class="next-link" href="#to_string">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>

### `to_string`

#### Signature

```cpp
Slot::to_string() -> std::string
```

#### What It Does

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.to_string();
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_kindle">
    <span class="label">Previous</span>
    <span class="title">to_kindle</span>
  </a>
  <a class="next-link" href="#transform">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>

### `transform`

#### Signature

```cpp
Slot::transform(const std::function<Value(const Value&)>& fn) -> Slot
```

#### What It Does

Applies a value-wise transform to each slot entry and returns the updated slot.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.transform([](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_string">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="#type_counts">
    <span class="label">Next</span>
    <span class="title">type_counts</span>
  </a>
</div>

### `type_counts`

#### Signature

```cpp
Slot::type_counts() -> std::unordered_map<std::string, std::int64_t>
```

#### What It Does

Counts how many slot entries belong to each stored pluss type.

#### Parameters

`None`

#### Returns

`std::unordered_map<std::string, std::int64_t>`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("ok"), pluss::make_series(3)});

auto out = slot.type_counts();
```

##### Output

```text
{"Series": 2, "Note": 1}
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#transform">
    <span class="label">Previous</span>
    <span class="title">transform</span>
  </a>
  <a class="next-link" href="#unique">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>

## U

### `unique`

#### Signature

```cpp
Slot::unique() -> bool
```

#### What It Does

Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.unique();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#type_counts">
    <span class="label">Previous</span>
    <span class="title">type_counts</span>
  </a>
  <a class="next-link" href="#values">
    <span class="label">Next</span>
    <span class="title">values</span>
  </a>
</div>

## V

### `values`

#### Signature

```cpp
Slot::values() -> const std::vector<Value>&
```

#### What It Does

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Parameters

`None`

#### Returns

`const std::vector<Value>&`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.values();
```

##### Output

```text
Slot([Series(1), Series(2), Series(3)])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#unique">
    <span class="label">Previous</span>
    <span class="title">unique</span>
  </a>
</div>
