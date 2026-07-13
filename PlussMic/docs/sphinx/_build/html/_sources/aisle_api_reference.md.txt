# Aisle API Reference

This page documents the full public `Aisle` surface for depth-stacked registries.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

## A

### `apply_slot`

#### Signature

```cpp
Aisle::apply_slot(std::string_view key, const std::function<Slot(const Slot&)>& fn) -> Aisle
```

#### What It Does

Applies a slot transform to one named key inside every registry and returns the updated aisle.

#### Parameters

**key**
    Name of the target slot or stream.

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Aisle`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.apply_slot("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

#### Practical Behavior

- `apply_slot(key, fn)` walks every registry in the aisle and rewrites that slot only where it exists.
- Registries that do not have the requested key are left unchanged.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="#as_kindle">
    <span class="label">Next</span>
    <span class="title">as_kindle</span>
  </a>
</div>

### `as_kindle`

#### Signature

```cpp
Aisle::as_kindle() -> pluss::kindle::Archive
```

#### What It Does

Converts or reinterprets the current payload as another public Pluss type so downstream operations route through the right text, numeric, or temporal surface.

#### Parameters

`None`

#### Returns

`pluss::kindle::Archive`
    Value produced by this call.

#### Examples

Example 1. Convert the current object into the `kindle` representation before continuing with another type-specific workflow.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.as_kindle();
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

#### Bridge Behavior

- `as_kindle()` converts the aisle into the Archive model.
- Use this when the next workflow is archive/catalog based rather than bundle based.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#apply_slot">
    <span class="label">Previous</span>
    <span class="title">apply_slot</span>
  </a>
  <a class="next-link" href="#concat_depth">
    <span class="label">Next</span>
    <span class="title">concat_depth</span>
  </a>
</div>

## C

### `concat_depth`

#### Signature

```cpp
Aisle::concat_depth(const Aisle& other) -> Aisle
```

#### What It Does

Appends the registries from another aisle onto the current depth axis.

#### Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Returns

`Aisle`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.concat_depth(aisle);
```

##### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
depth2 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth3 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

#### Practical Behavior

- `concat_depth(other)` appends the other aisle’s registries onto the current depth axis.
- If both aisles have labels, the labels are concatenated too.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#as_kindle">
    <span class="label">Previous</span>
    <span class="title">as_kindle</span>
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
Aisle::defined() -> bool
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.defined();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#concat_depth">
    <span class="label">Previous</span>
    <span class="title">concat_depth</span>
  </a>
  <a class="next-link" href="#depth">
    <span class="label">Next</span>
    <span class="title">depth</span>
  </a>
</div>

### `depth`

#### Signature

```cpp
Aisle::depth() -> std::int64_t
```

#### What It Does

Returns a size/count summary for the current object without changing it.

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Examples

Example 1. Inspect the `depth` property of the current object.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.depth();
```

##### Output

```text
2
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#defined">
    <span class="label">Previous</span>
    <span class="title">defined</span>
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
Aisle::empty() -> bool
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.empty();
```

##### Output

```text
false
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#depth">
    <span class="label">Previous</span>
    <span class="title">depth</span>
  </a>
  <a class="next-link" href="#from_kindle">
    <span class="label">Next</span>
    <span class="title">from_kindle</span>
  </a>
</div>

## F

### `from_kindle`

#### Signature

```cpp
Aisle::from_kindle(const pluss::kindle::Archive& archive) -> Aisle
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

**archive**
    Archive payload supplied to this overload.

#### Returns

`Aisle`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = pluss::bundle::Aisle::from_kindle(archive);
```

##### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#empty">
    <span class="label">Previous</span>
    <span class="title">empty</span>
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
Aisle::head(std::int64_t n) -> Aisle
```

#### What It Does

Returns the first `n` registries from the aisle depth axis.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Aisle`
    Value produced by this call.

#### Examples

Example 1. Inspect the first part of the current object without changing the source data.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.head(1);
```

##### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
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
Aisle::kindelize() -> pluss::kindle::Archive
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

`None`

#### Returns

`pluss::kindle::Archive`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.kindelize();
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#head">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="#labels">
    <span class="label">Next</span>
    <span class="title">labels</span>
  </a>
</div>

## L

### `labels`

#### Signature

```cpp
Aisle::labels() -> const std::optional<Slot>&
```

#### What It Does

Returns the current labels used to address fields, slots, rows, or depth items.

#### Parameters

`None`

#### Returns

`const std::optional<Slot>&`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.labels();
```

##### Output

```text
["train", "test"]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#kindelize">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="#map">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>

## M

### `map`

#### Signature

```cpp
Aisle::map(const std::function<Registry(const Registry&)>& fn) -> Aisle
```

#### What It Does

Applies the same function to each registry and returns the transformed aisle.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Aisle`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.map([](const pluss::bundle::Registry& registry) { return registry; });
```

##### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#labels">
    <span class="label">Previous</span>
    <span class="title">labels</span>
  </a>
  <a class="next-link" href="#registries">
    <span class="label">Next</span>
    <span class="title">registries</span>
  </a>
</div>

## R

### `registries`

#### Signature

```cpp
Aisle::registries() -> const std::vector<Registry>&
```

#### What It Does

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Parameters

`None`

#### Returns

`const std::vector<Registry>&`
    Borrowed collection of registries.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.registries();
```

##### Output

```text
[Registry(depth0), Registry(depth1)]
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#map">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="#registry">
    <span class="label">Next</span>
    <span class="title">registry</span>
  </a>
</div>

### `registry`

#### Signature

```cpp
Aisle::registry(std::int64_t i) -> Registry
```

#### What It Does

Fetches one value, member, or slice from the current object without changing the source object.

#### Parameters

**i**
    Zero-based positional index used by this overload.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.registry(0);
```

##### Output

```text
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#registries">
    <span class="label">Previous</span>
    <span class="title">registries</span>
  </a>
  <a class="next-link" href="#select">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>

## S

### `select`

#### Signature

```cpp
Aisle::select(std::initializer_list<std::string_view> keys) -> Aisle
```

#### What It Does

Keeps only the named keys inside every registry in the aisle.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Aisle`
    Value produced by this call.

#### Examples

Example 1. Pass several feature names inline.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.select({"ids"});
```

##### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#registry">
    <span class="label">Previous</span>
    <span class="title">registry</span>
  </a>
  <a class="next-link" href="#show">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>

### `show`

#### Signature

```cpp
Aisle::show(std::ostream& os) -> void
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

aisle.show(std::cout);
```

##### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#select">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="#slice">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>

### `slice`

#### Signature

```cpp
Aisle::slice(std::int64_t start, std::int64_t stop) -> Aisle
```

#### What It Does

Returns a contiguous depth slice `[start, stop)` from the aisle.

#### Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

#### Returns

`Aisle`
    Value produced by this call.

#### Examples

Example 1. Inspect one contiguous positional slice from the current object.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.slice(0, 1);
```

##### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#show">
    <span class="label">Previous</span>
    <span class="title">show</span>
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
Aisle::tail(std::int64_t n) -> Aisle
```

#### What It Does

Returns the last `n` registries from the aisle depth axis.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Aisle`
    Value produced by this call.

#### Examples

Example 1. Inspect the last part of the current object without changing the source data.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.tail(1);
```

##### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#slice">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="#to_json">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>

### `to_json`

#### Signature

```cpp
Aisle::to_json() -> std::string
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.to_json();
```

##### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#tail">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="#to_kindle">
    <span class="label">Next</span>
    <span class="title">to_kindle</span>
  </a>
</div>

### `to_kindle`

#### Signature

```cpp
Aisle::to_kindle() -> pluss::kindle::Archive
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

`None`

#### Returns

`pluss::kindle::Archive`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.to_kindle();
```

##### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
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
Aisle::to_string() -> std::string
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.to_string();
```

##### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_kindle">
    <span class="label">Previous</span>
    <span class="title">to_kindle</span>
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
Aisle::unique() -> bool
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.unique();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_string">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
</div>
