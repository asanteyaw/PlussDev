# Registry API Reference

This page documents the full public `Registry` surface for named bundle slots.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

## A

### `apply`

#### Signatures

```cpp
Registry::apply(const NameRange& keys, const std::function<Slot(const Slot&)>& fn) -> template <pluss::detail::StringViewNameRange NameRange> Registry

Registry::apply(std::initializer_list<std::string_view> keys, const std::function<Slot(const Slot&)>& fn) -> Registry

Registry::apply(std::span<const std::string_view> keys, const std::function<Slot(const Slot&)>& fn) -> Registry

Registry::apply(std::string_view key, const std::function<Slot(const Slot&)>& fn) -> Registry
```

#### What It Does

Applies a function to one or more named slots and returns the updated registry.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

**key**
    Name of the target slot or stream.

#### Returns

`template <pluss::detail::StringViewNameRange NameRange> Registry`
    Value produced by this call.

`Registry`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 2. Pass several feature names inline.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply({"ids"}, [](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 3. Apply a user function and inspect the transformed result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 4. Apply a user function and inspect the transformed result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

#### Practical Behavior

- `apply(...)` rewrites one slot or a selected slot set; `apply_all(...)` rewrites every slot.
- The transform must return a defined slot.
- Use the underscore form when you want the mutation in place.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="#apply_">
    <span class="label">Next</span>
    <span class="title">apply_</span>
  </a>
</div>

### `apply_`

#### Signatures

```cpp
Registry::apply_(const NameRange& keys, const std::function<Slot(const Slot&)>& fn) -> template <pluss::detail::StringViewNameRange NameRange> Registry&

Registry::apply_(std::initializer_list<std::string_view> keys, const std::function<Slot(const Slot&)>& fn) -> Registry&

Registry::apply_(std::span<const std::string_view> keys, const std::function<Slot(const Slot&)>& fn) -> Registry&

Registry::apply_(std::string_view key, const std::function<Slot(const Slot&)>& fn) -> Registry&
```

#### What It Does

Applies a function to one or more named slots in place.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

**key**
    Name of the target slot or stream.

#### Returns

`template <pluss::detail::StringViewNameRange NameRange> Registry&`
    Value produced by this call.

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.apply_("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 2. Pass several feature names inline.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.apply_({"ids"}, [](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 3. Apply a user function and inspect the transformed result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.apply_("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 4. Apply a user function and inspect the transformed result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.apply_("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#apply">
    <span class="label">Previous</span>
    <span class="title">apply</span>
  </a>
  <a class="next-link" href="#apply_all">
    <span class="label">Next</span>
    <span class="title">apply_all</span>
  </a>
</div>

### `apply_all`

#### Signature

```cpp
Registry::apply_all(const std::function<Slot(const Slot&)>& fn) -> Registry
```

#### What It Does

Applies the same function to every slot and returns the updated registry.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply_all([](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

#### Practical Behavior

- `apply(...)` rewrites one slot or a selected slot set; `apply_all(...)` rewrites every slot.
- The transform must return a defined slot.
- Use the underscore form when you want the mutation in place.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#apply_">
    <span class="label">Previous</span>
    <span class="title">apply_</span>
  </a>
  <a class="next-link" href="#as_kindle">
    <span class="label">Next</span>
    <span class="title">as_kindle</span>
  </a>
</div>

### `as_kindle`

#### Signature

```cpp
Registry::as_kindle() -> pluss::kindle::Catalog
```

#### What It Does

Converts or reinterprets the current payload as another public Pluss type so downstream operations route through the right text, numeric, or temporal surface.

#### Parameters

`None`

#### Returns

`pluss::kindle::Catalog`
    Value produced by this call.

#### Examples

Example 1. Convert the current object into the `kindle` representation before continuing with another type-specific workflow.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.as_kindle();
```

##### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Bridge Behavior

- `as_kindle()` converts the registry into the Kindle/Catalog model.
- Use this when the next operation is stream-oriented rather than bundle-oriented.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#apply_all">
    <span class="label">Previous</span>
    <span class="title">apply_all</span>
  </a>
  <a class="next-link" href="#begin">
    <span class="label">Next</span>
    <span class="title">begin</span>
  </a>
</div>

## B

### `begin`

#### Signature

```cpp
Registry::begin() -> const_iterator
```

#### What It Does

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

#### Parameters

`None`

#### Returns

`const_iterator`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

for (const auto& key : rg) {
  (void)key;
}
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#as_kindle">
    <span class="label">Previous</span>
    <span class="title">as_kindle</span>
  </a>
  <a class="next-link" href="#clear">
    <span class="label">Next</span>
    <span class="title">clear</span>
  </a>
</div>

## C

### `clear`

#### Signature

```cpp
Registry::clear() -> Registry
```

#### What It Does

Removes all slots and returns an empty registry.

#### Parameters

`None`

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.clear();
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
<empty rg>
```

#### Practical Behavior

- `clear()` returns an empty registry with no slots.
- Use `clear_()` when you want the same effect in place.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#begin">
    <span class="label">Previous</span>
    <span class="title">begin</span>
  </a>
  <a class="next-link" href="#clear_">
    <span class="label">Next</span>
    <span class="title">clear_</span>
  </a>
</div>

### `clear_`

#### Signature

```cpp
Registry::clear_() -> Registry&
```

#### What It Does

Removes all slots in place.

#### Parameters

`None`

#### Returns

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.clear_();
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#clear">
    <span class="label">Previous</span>
    <span class="title">clear</span>
  </a>
  <a class="next-link" href="#cloc">
    <span class="label">Next</span>
    <span class="title">cloc</span>
  </a>
</div>

### `cloc`

#### Signatures

```cpp
Registry::cloc(std::int64_t row) -> Registry

Registry::cloc(std::int64_t start, std::int64_t stop) -> Registry
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**row**
    Zero-based row position used by this overload.

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.cloc();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.cloc();
```

##### Output

```text
The call completes and returns the contract described under Output.
```

#### Practical Behavior

- `cloc(...)` slices positional rows across every slot while keeping slot alignment intact.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#clear_">
    <span class="label">Previous</span>
    <span class="title">clear_</span>
  </a>
  <a class="next-link" href="#combine">
    <span class="label">Next</span>
    <span class="title">combine</span>
  </a>
</div>

### `combine`

#### Signature

```cpp
Registry::combine(const Registry& other, std::string suffix = "_other") -> Registry
```

#### What It Does

Merges slots from another registry into this one, applying the collision suffix when keys overlap.

#### Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

**suffix**
    Text suffix appended after the existing label or value.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto other = pluss::bundle::Registry({{"flag", pluss::bundle::Slot({pluss::make_series(true, pluss::ScalarType::Bool), pluss::make_series(false, pluss::ScalarType::Bool), pluss::make_series(true, pluss::ScalarType::Bool)})}});
auto out = rg.combine(other, "_rhs");
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids      -> Slot([Series(1), Series(2), Series(3)])
city     -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
flag_rhs -> Slot([Series(true), Series(false), Series(true)])
```

#### Practical Behavior

- `combine(other, suffix)` appends slots from `other` and renames collisions using the supplied suffix.
- If the first suffixed name still collides, numbered suffixes are added until the name is unique.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#cloc">
    <span class="label">Previous</span>
    <span class="title">cloc</span>
  </a>
  <a class="next-link" href="#concat_values">
    <span class="label">Next</span>
    <span class="title">concat_values</span>
  </a>
</div>

### `concat_values`

#### Signature

```cpp
Registry::concat_values(std::string_view key, const Slot& other) -> Registry
```

#### What It Does

Appends extra values onto one named slot and returns the updated registry.

#### Parameters

**key**
    Name of the target slot or stream.

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
auto out = rg.concat_values("ids", extra);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3), Series(4), Series(5)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

#### Practical Behavior

- `concat_values(key, other)` appends the values from another slot onto one existing slot.
- The target key must exist and the appended slot must be defined.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#combine">
    <span class="label">Previous</span>
    <span class="title">combine</span>
  </a>
  <a class="next-link" href="#concat_values_">
    <span class="label">Next</span>
    <span class="title">concat_values_</span>
  </a>
</div>

### `concat_values_`

#### Signature

```cpp
Registry::concat_values_(std::string_view key, const Slot& other) -> Registry&
```

#### What It Does

Appends extra values onto one named slot in place.

#### Parameters

**key**
    Name of the target slot or stream.

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Returns

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
rg.concat_values_("ids", extra);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#concat_values">
    <span class="label">Previous</span>
    <span class="title">concat_values</span>
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
Registry::defined() -> bool
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.defined();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#concat_values_">
    <span class="label">Previous</span>
    <span class="title">concat_values_</span>
  </a>
  <a class="next-link" href="#distinct">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>

### `distinct`

#### Signature

```cpp
Registry::distinct(pluss::kindle::DistinctOptions options = {}) -> Registry
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.distinct();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#defined">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="#drop">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>

### `drop`

#### Signatures

```cpp
Registry::drop(const NameRange& keys) -> template <pluss::detail::StringViewNameRange NameRange> Registry

Registry::drop(const std::vector<std::string>& keys) -> Registry

Registry::drop(std::initializer_list<std::string_view> keys) -> Registry

Registry::drop(std::span<const std::string_view> keys) -> Registry

Registry::drop(std::string_view key) -> Registry
```

#### What It Does

Removes the named slots and keeps the remaining registry structure intact.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

**key**
    Name of the target slot or stream.

#### Returns

`template <pluss::detail::StringViewNameRange NameRange> Registry`
    Value produced by this call.

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop({"city"});
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
```
Example 2. Pass several names together from a standard string container.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string> names{"city"};
auto out = rg.drop(names);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
```
Example 3. Pass several feature names inline.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop({"city"});
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
```
Example 4. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop({"city"});
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
```
Example 5. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop("city");
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
```

#### Practical Behavior

- `drop(...)` removes named slots, not individual values.
- Unknown keys throw immediately.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#distinct">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="#drop_empty">
    <span class="label">Next</span>
    <span class="title">drop_empty</span>
  </a>
</div>

### `drop_empty`

#### Signature

```cpp
Registry::drop_empty() -> Registry
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop_empty();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#drop">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="#drop_empty_">
    <span class="label">Next</span>
    <span class="title">drop_empty_</span>
  </a>
</div>

### `drop_empty_`

#### Signature

```cpp
Registry::drop_empty_() -> Registry&
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop_empty_();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#drop_empty">
    <span class="label">Previous</span>
    <span class="title">drop_empty</span>
  </a>
  <a class="next-link" href="#dropnull">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>

### `dropnull`

#### Signature

```cpp
Registry::dropnull() -> Registry
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

`None`

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.dropnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#drop_empty_">
    <span class="label">Previous</span>
    <span class="title">drop_empty_</span>
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
Registry::empty() -> bool
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.empty();
```

##### Output

```text
false
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#dropnull">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="#end">
    <span class="label">Next</span>
    <span class="title">end</span>
  </a>
</div>

### `end`

#### Signature

```cpp
Registry::end() -> const_iterator
```

#### What It Does

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

#### Parameters

`None`

#### Returns

`const_iterator`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

for (const auto& key : rg) {
  (void)key;
}
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#empty">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="#ensure_stream_compatible">
    <span class="label">Next</span>
    <span class="title">ensure_stream_compatible</span>
  </a>
</div>

### `ensure_stream_compatible`

#### Signature

```cpp
Registry::ensure_stream_compatible(std::string_view where = "Registry") -> void
```

#### What It Does

Validates that the registry can be converted to a Kindle catalog without incompatible slot contents.

#### Parameters

**where**
    Filter expression or selector string interpreted by this overload.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.ensure_stream_compatible("demo");
```

##### Output

```text
no error
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#end">
    <span class="label">Previous</span>
    <span class="title">end</span>
  </a>
  <a class="next-link" href="#fieldaxis">
    <span class="label">Next</span>
    <span class="title">fieldaxis</span>
  </a>
</div>

## F

### `fieldaxis`

#### Signature

```cpp
Registry::fieldaxis() -> RegistryFieldAxisAccessor
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`RegistryFieldAxisAccessor`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fieldaxis();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#ensure_stream_compatible">
    <span class="label">Previous</span>
    <span class="title">ensure_stream_compatible</span>
  </a>
  <a class="next-link" href="#fields">
    <span class="label">Next</span>
    <span class="title">fields</span>
  </a>
</div>

### `fields`

#### Signatures

```cpp
Registry::fields() -> const std::vector<std::string>&

Registry::fields(const NameRange& keys) -> template <pluss::detail::StringViewNameRange NameRange> RegistryFieldAccessor

Registry::fields(std::initializer_list<std::string_view> keys) -> RegistryFieldAccessor

Registry::fields(std::span<const std::string_view> keys) -> RegistryFieldAccessor

Registry::fields(std::string_view key) -> RegistryFieldAccessor
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

**key**
    Name of the target slot or stream.

#### Returns

`const std::vector<std::string>&`
    Borrowed list of names.

`template <pluss::detail::StringViewNameRange NameRange> RegistryFieldAccessor`
    Value produced by this call.

`RegistryFieldAccessor`
    Value produced by this call.

#### Examples

Example 1. Pass several names together from a standard string container.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fields();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fields();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Pass several feature names inline.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fields();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 4. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fields();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 5. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fields();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#fieldaxis">
    <span class="label">Previous</span>
    <span class="title">fieldaxis</span>
  </a>
  <a class="next-link" href="#fillnull">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>

### `fillnull`

#### Signatures

```cpp
Registry::fillnull(const char* value) -> Registry

Registry::fillnull(const pluss::TimeIndex& value) -> Registry

Registry::fillnull(const pluss::Timeline& value) -> Registry

Registry::fillnull(double value) -> Registry

Registry::fillnull(std::string_view value) -> Registry
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fillnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fillnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fillnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 4. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fillnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 5. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.fillnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#fields">
    <span class="label">Previous</span>
    <span class="title">fields</span>
  </a>
  <a class="next-link" href="#filter_values">
    <span class="label">Next</span>
    <span class="title">filter_values</span>
  </a>
</div>

### `filter_values`

#### Signature

```cpp
Registry::filter_values(std::string_view key, const std::function<bool(const Slot::Value&)>& predicate) -> Registry
```

#### What It Does

Keeps only the values in one named slot that satisfy the supplied predicate.

#### Parameters

**key**
    Name of the target slot or stream.

**predicate**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.filter_values("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#fillnull">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="#filter_values_">
    <span class="label">Next</span>
    <span class="title">filter_values_</span>
  </a>
</div>

### `filter_values_`

#### Signature

```cpp
Registry::filter_values_(std::string_view key, const std::function<bool(const Slot::Value&)>& predicate) -> Registry&
```

#### What It Does

Filters the values in one named slot in place.

#### Parameters

**key**
    Name of the target slot or stream.

**predicate**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.filter_values_("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#filter_values">
    <span class="label">Previous</span>
    <span class="title">filter_values</span>
  </a>
  <a class="next-link" href="#freq">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>

### `freq`

#### Signature

```cpp
Registry::freq(pluss::kindle::FreqOptions options = {}) -> Registry
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.freq();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#filter_values_">
    <span class="label">Previous</span>
    <span class="title">filter_values_</span>
  </a>
  <a class="next-link" href="#from_kindle">
    <span class="label">Next</span>
    <span class="title">from_kindle</span>
  </a>
</div>

### `from_kindle`

#### Signature

```cpp
Registry::from_kindle(const pluss::kindle::Catalog& catalog) -> Registry
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

**catalog**
    Catalog payload supplied to this overload.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = pluss::bundle::Registry::from_kindle(catalog);
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#freq">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="#get">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>

## G

### `get`

#### Signature

```cpp
Registry::get(std::string_view key) -> std::optional<Slot>
```

#### What It Does

Fetches one value, member, or slice from the current object without changing the source object.

#### Parameters

**key**
    Name of the target slot or stream.

#### Returns

`std::optional<Slot>`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.get("ids");
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
  <a class="next-link" href="#has">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>

## H

### `has`

#### Signature

```cpp
Registry::has(std::string_view key) -> bool
```

#### What It Does

Checks whether a named slot exists.

#### Parameters

**key**
    Name of the target slot or stream.

#### Returns

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has("ids");
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#get">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="#has_all">
    <span class="label">Next</span>
    <span class="title">has_all</span>
  </a>
</div>

### `has_all`

#### Signatures

```cpp
Registry::has_all(const NameRange& keys) -> template <pluss::detail::StringViewNameRange NameRange> bool

Registry::has_all(std::initializer_list<std::string_view> keys) -> bool

Registry::has_all(std::span<const std::string_view> keys) -> bool
```

#### What It Does

Checks whether every requested slot name exists.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`template <pluss::detail::StringViewNameRange NameRange> bool`
    Value produced by this call.

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_all({"ids", "city"});
```

##### Output

```text
true
```
Example 2. Pass several feature names inline.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_all({"ids", "city"});
```

##### Output

```text
true
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_all({"ids", "city"});
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#has">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="#has_any">
    <span class="label">Next</span>
    <span class="title">has_any</span>
  </a>
</div>

### `has_any`

#### Signatures

```cpp
Registry::has_any(const NameRange& keys) -> template <pluss::detail::StringViewNameRange NameRange> bool

Registry::has_any(std::initializer_list<std::string_view> keys) -> bool

Registry::has_any(std::span<const std::string_view> keys) -> bool
```

#### What It Does

Checks whether at least one requested slot name exists.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`template <pluss::detail::StringViewNameRange NameRange> bool`
    Value produced by this call.

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_any({"ids", "city"});
```

##### Output

```text
true
```
Example 2. Pass several feature names inline.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_any({"ids", "city"});
```

##### Output

```text
true
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_any({"ids", "city"});
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#has_all">
    <span class="label">Previous</span>
    <span class="title">has_all</span>
  </a>
  <a class="next-link" href="#head">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>

### `head`

#### Signature

```cpp
Registry::head(std::int64_t n) -> Registry
```

#### What It Does

Returns the first `n` values from every slot in the registry.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Inspect the first part of the current object without changing the source data.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.head(2);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#has_any">
    <span class="label">Previous</span>
    <span class="title">has_any</span>
  </a>
  <a class="next-link" href="#invalid">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>

## I

### `invalid`

#### Signature

```cpp
Registry::invalid() -> Registry
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.invalid();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#head">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="#is_stream_compatible">
    <span class="label">Next</span>
    <span class="title">is_stream_compatible</span>
  </a>
</div>

### `is_stream_compatible`

#### Signature

```cpp
Registry::is_stream_compatible() -> bool
```

#### What It Does

Reports whether the registry can be converted to a Kindle catalog without incompatible slot contents.

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.is_stream_compatible();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#invalid">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="#isnull">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>

### `isnull`

#### Signature

```cpp
Registry::isnull() -> Registry
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

`None`

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.isnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#is_stream_compatible">
    <span class="label">Previous</span>
    <span class="title">is_stream_compatible</span>
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
Registry::kindelize() -> pluss::kindle::Catalog
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

`None`

#### Returns

`pluss::kindle::Catalog`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.kindelize();
```

##### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#isnull">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="#load">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>

## L

### `load`

#### Signatures

```cpp
Registry::load(std::string_view source_type, std::string_view filepath) -> Registry

Registry::load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config) -> Registry
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**source_type**
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::bundle::Registry::load("csv", "path/to/data.csv");
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto out = pluss::bundle::Registry::load("csv", "path/to/data.csv");
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#kindelize">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="#load_csv">
    <span class="label">Next</span>
    <span class="title">load_csv</span>
  </a>
</div>

### `load_csv`

#### Signatures

```cpp
Registry::load_csv(std::string_view filepath) -> Registry

Registry::load_csv(std::string_view filepath, const pluss::loader::Config& config) -> Registry

Registry::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) -> Registry
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::bundle::Registry::load_csv("path/to/data.csv");
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto out = pluss::bundle::Registry::load_csv("path/to/data.csv", {"nrows", 16});
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 3. Pass a typed reader-options object to control file loading details.

```cpp
pluss::loader::ReadCsvOptions options;
auto out = pluss::bundle::Registry::load_csv("path/to/data.csv", options);
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="#load_excel">
    <span class="label">Next</span>
    <span class="title">load_excel</span>
  </a>
</div>

### `load_excel`

#### Signatures

```cpp
Registry::load_excel(std::string_view filepath) -> Registry

Registry::load_excel(std::string_view filepath, const pluss::loader::Config& config) -> Registry

Registry::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) -> Registry
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::bundle::Registry::load_excel("path/to/data.xlsx");
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto out = pluss::bundle::Registry::load_excel("path/to/data.xlsx", {"nrows", 16});
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 3. Pass a typed reader-options object to control file loading details.

```cpp
pluss::loader::ReadExcelOptions options;
auto out = pluss::bundle::Registry::load_excel("path/to/data.xlsx", options);
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load_csv">
    <span class="label">Previous</span>
    <span class="title">load_csv</span>
  </a>
  <a class="next-link" href="#load_json">
    <span class="label">Next</span>
    <span class="title">load_json</span>
  </a>
</div>

### `load_json`

#### Signatures

```cpp
Registry::load_json(std::string_view filepath) -> Registry

Registry::load_json(std::string_view filepath, const pluss::loader::Config& config) -> Registry
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::bundle::Registry::load_json("path/to/data.json");
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto out = pluss::bundle::Registry::load_json("path/to/data.json", {"nrows", 16});
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load_excel">
    <span class="label">Previous</span>
    <span class="title">load_excel</span>
  </a>
  <a class="next-link" href="#load_parquet">
    <span class="label">Next</span>
    <span class="title">load_parquet</span>
  </a>
</div>

### `load_parquet`

#### Signatures

```cpp
Registry::load_parquet(std::string_view filepath) -> Registry

Registry::load_parquet(std::string_view filepath, const pluss::loader::Config& config) -> Registry
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::bundle::Registry::load_parquet("path/to/data.parquet");
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto out = pluss::bundle::Registry::load_parquet("path/to/data.parquet", {"nrows", 16});
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load_json">
    <span class="label">Previous</span>
    <span class="title">load_json</span>
  </a>
  <a class="next-link" href="#load_txt">
    <span class="label">Next</span>
    <span class="title">load_txt</span>
  </a>
</div>

### `load_txt`

#### Signatures

```cpp
Registry::load_txt(std::string_view filepath) -> Registry

Registry::load_txt(std::string_view filepath, const pluss::loader::Config& config) -> Registry
```

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto out = pluss::bundle::Registry::load_txt("path/to/data.txt");
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto out = pluss::bundle::Registry::load_txt("path/to/data.txt", {"nrows", 16});
```

##### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load_parquet">
    <span class="label">Previous</span>
    <span class="title">load_parquet</span>
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
Registry::map(const std::function<Slot(const Slot&)>& fn) -> Registry
```

#### What It Does

Transforms every slot with the same function and returns the updated registry.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.map([](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#load_txt">
    <span class="label">Previous</span>
    <span class="title">load_txt</span>
  </a>
  <a class="next-link" href="#map_">
    <span class="label">Next</span>
    <span class="title">map_</span>
  </a>
</div>

### `map_`

#### Signature

```cpp
Registry::map_(const std::function<Slot(const Slot&)>& fn) -> Registry&
```

#### What It Does

Transforms every slot in place with the same function.

#### Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.map_([](const pluss::bundle::Slot& slot) { return slot; });
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#map">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="#max">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>

### `max`

#### Signature

```cpp
Registry::max(std::string_view field) -> Slot
```

#### What It Does

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Parameters

**field**
    Name of the target stream in the catalog.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Compute `max` and inspect the resulting summary value or summary object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.max();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#map_">
    <span class="label">Previous</span>
    <span class="title">map_</span>
  </a>
  <a class="next-link" href="#mean">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>

### `mean`

#### Signature

```cpp
Registry::mean(std::string_view field) -> Slot
```

#### What It Does

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Parameters

**field**
    Name of the target stream in the catalog.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Compute `mean` and inspect the resulting summary value or summary object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.mean();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#max">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="#min">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>

### `min`

#### Signature

```cpp
Registry::min(std::string_view field) -> Slot
```

#### What It Does

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Parameters

**field**
    Name of the target stream in the catalog.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Compute `min` and inspect the resulting summary value or summary object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.min();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#mean">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="#ndistinct">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>

## N

### `ndistinct`

#### Signature

```cpp
Registry::ndistinct(pluss::kindle::DistinctOptions options = {}) -> Registry
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.ndistinct();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#min">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="#nfields">
    <span class="label">Next</span>
    <span class="title">nfields</span>
  </a>
</div>

### `nfields`

#### Signature

```cpp
Registry::nfields() -> std::int64_t
```

#### What It Does

Returns a size/count summary for the current object without changing it.

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.nfields();
```

##### Output

```text
2
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#ndistinct">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="#notnull">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>

### `notnull`

#### Signature

```cpp
Registry::notnull() -> Registry
```

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Parameters

`None`

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.notnull();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#nfields">
    <span class="label">Previous</span>
    <span class="title">nfields</span>
  </a>
  <a class="next-link" href="#push">
    <span class="label">Next</span>
    <span class="title">push</span>
  </a>
</div>

## P

### `push`

#### Signature

```cpp
Registry::push(std::string_view key, const Slot::Value& value) -> Registry
```

#### What It Does

Appends one new value to a named slot and returns the updated registry.

#### Parameters

**key**
    Name of the target slot or stream.

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.push("ids", pluss::make_series(4));
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#notnull">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="#push_">
    <span class="label">Next</span>
    <span class="title">push_</span>
  </a>
</div>

### `push_`

#### Signature

```cpp
Registry::push_(std::string_view key, const Slot::Value& value) -> Registry&
```

#### What It Does

Appends one new value to a named slot in place.

#### Parameters

**key**
    Name of the target slot or stream.

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

#### Returns

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.push_("ids", pluss::make_series(4));
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#push">
    <span class="label">Previous</span>
    <span class="title">push</span>
  </a>
  <a class="next-link" href="#push_all">
    <span class="label">Next</span>
    <span class="title">push_all</span>
  </a>
</div>

### `push_all`

#### Signature

```cpp
Registry::push_all(const Slot::Value& value) -> Registry
```

#### What It Does

Appends the same new value to every slot and returns the updated registry.

#### Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.push_all(pluss::make_note("ok"));
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#push_">
    <span class="label">Previous</span>
    <span class="title">push_</span>
  </a>
  <a class="next-link" href="#push_all_">
    <span class="label">Next</span>
    <span class="title">push_all_</span>
  </a>
</div>

### `push_all_`

#### Signature

```cpp
Registry::push_all_(const Slot::Value& value) -> Registry&
```

#### What It Does

Appends the same new value to every slot in place.

#### Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

#### Returns

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.push_all_(pluss::make_note("ok"));
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#push_all">
    <span class="label">Previous</span>
    <span class="title">push_all</span>
  </a>
  <a class="next-link" href="#rename">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>

## R

### `rename`

#### Signatures

```cpp
Registry::rename(const std::unordered_map<std::string, std::string>& mapping) -> Registry

Registry::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) -> Registry

Registry::rename(std::string_view from, std::string_view to) -> Registry
```

#### What It Does

Renames one or more slots without changing their stored values.

#### Parameters

**mapping**
    Name-to-name mapping used by this overload.

**from**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

**to**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::unordered_map<std::string, std::string> renames{{"ids", "ids_new"}};
auto out = rg.rename(renames);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.rename({{"ids", "ids_new"}});
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.rename("ids", "ids_new");
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#push_all_">
    <span class="label">Previous</span>
    <span class="title">push_all_</span>
  </a>
  <a class="next-link" href="#rename_">
    <span class="label">Next</span>
    <span class="title">rename_</span>
  </a>
</div>

### `rename_`

#### Signature

```cpp
Registry::rename_(std::string_view from, std::string_view to) -> Registry&
```

#### What It Does

Renames one or more slots in place without changing their stored values.

#### Parameters

**from**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

**to**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Returns

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.rename_("ids", "ids_new");
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#rename">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="#sample">
    <span class="label">Next</span>
    <span class="title">sample</span>
  </a>
</div>

## S

### `sample`

#### Signature

```cpp
Registry::sample(std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) -> Registry
```

#### What It Does

Returns a random sample of positions across every slot.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

**random_state**
    Optional seed used to make sampling or shuffling reproducible.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.sample(2, 7);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#rename_">
    <span class="label">Previous</span>
    <span class="title">rename_</span>
  </a>
  <a class="next-link" href="#save_as">
    <span class="label">Next</span>
    <span class="title">save_as</span>
  </a>
</div>

### `save_as`

#### Signature

```cpp
Registry::save_as(std::string_view target_type, std::string_view filepath, const pluss::loader::Config& config = {}) -> void
```

#### What It Does

Writes the registry using the explicitly supplied output format selector.

#### Parameters

**target_type**
    Format or target selector string used by this overload.

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Pass an explicit loader or saver config object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_as("json", "path/to/output.json");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#sample">
    <span class="label">Previous</span>
    <span class="title">sample</span>
  </a>
  <a class="next-link" href="#save_csv">
    <span class="label">Next</span>
    <span class="title">save_csv</span>
  </a>
</div>

### `save_csv`

#### Signatures

```cpp
Registry::save_csv(std::string_view filepath) -> void

Registry::save_csv(std::string_view filepath, const pluss::loader::Config& config) -> void
```

#### What It Does

Writes the registry to CSV after materializing each slot into a tabular export form.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_csv("path/to/output.csv");
```

##### Output

```text
saved: out.*
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_csv("path/to/output.csv");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_as">
    <span class="label">Previous</span>
    <span class="title">save_as</span>
  </a>
  <a class="next-link" href="#save_excel">
    <span class="label">Next</span>
    <span class="title">save_excel</span>
  </a>
</div>

### `save_excel`

#### Signatures

```cpp
Registry::save_excel(std::string_view filepath) -> void

Registry::save_excel(std::string_view filepath, const pluss::loader::Config& config) -> void
```

#### What It Does

Writes the registry to an Excel workbook after materializing each slot into a tabular export form.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_excel("path/to/output.xlsx");
```

##### Output

```text
saved: out.*
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_excel("path/to/output.xlsx");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_csv">
    <span class="label">Previous</span>
    <span class="title">save_csv</span>
  </a>
  <a class="next-link" href="#save_json">
    <span class="label">Next</span>
    <span class="title">save_json</span>
  </a>
</div>

### `save_json`

#### Signatures

```cpp
Registry::save_json(std::string_view filepath) -> void

Registry::save_json(std::string_view filepath, const pluss::loader::Config& config) -> void
```

#### What It Does

Writes the registry to JSON for interchange or later reload.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_json("path/to/output.json");
```

##### Output

```text
saved: out.*
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_json("path/to/output.json");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_excel">
    <span class="label">Previous</span>
    <span class="title">save_excel</span>
  </a>
  <a class="next-link" href="#save_parquet">
    <span class="label">Next</span>
    <span class="title">save_parquet</span>
  </a>
</div>

### `save_parquet`

#### Signatures

```cpp
Registry::save_parquet(std::string_view filepath) -> void

Registry::save_parquet(std::string_view filepath, const pluss::loader::Config& config) -> void
```

#### What It Does

Writes the registry to Parquet for compact typed storage.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_parquet("path/to/output.parquet");
```

##### Output

```text
saved: out.*
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_parquet("path/to/output.parquet");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_json">
    <span class="label">Previous</span>
    <span class="title">save_json</span>
  </a>
  <a class="next-link" href="#save_txt">
    <span class="label">Next</span>
    <span class="title">save_txt</span>
  </a>
</div>

### `save_txt`

#### Signatures

```cpp
Registry::save_txt(std::string_view filepath) -> void

Registry::save_txt(std::string_view filepath, const pluss::loader::Config& config) -> void
```

#### What It Does

Writes a text rendering of the registry for inspection or logging.

#### Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Examples

Example 1. Use the simplest filepath-based overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_txt("path/to/output.txt");
```

##### Output

```text
saved: out.*
```
Example 2. Pass an explicit loader or saver config object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_txt("path/to/output.txt");
```

##### Output

```text
saved: out.*
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_parquet">
    <span class="label">Previous</span>
    <span class="title">save_parquet</span>
  </a>
  <a class="next-link" href="#select">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>

### `select`

#### Signatures

```cpp
Registry::select(const NameRange& keys) -> template <pluss::detail::StringViewNameRange NameRange> Registry

Registry::select(const std::vector<std::string>& keys) -> Registry

Registry::select(std::initializer_list<std::string_view> keys) -> Registry

Registry::select(std::span<const std::string_view> keys) -> Registry

Registry::select(std::string_view key) -> Slot
```

#### What It Does

Keeps only the named slots and preserves their shared positional alignment.

#### Parameters

**keys**
    Collection of slot or stream names to keep, drop, or rewrite together.

**key**
    Name of the target slot or stream.

#### Returns

`template <pluss::detail::StringViewNameRange NameRange> Registry`
    Value produced by this call.

`Registry`
    Value produced by this call.

`Slot`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.select({"ids"});
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 2. Pass several names together from a standard string container.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string> names{"ids"};
auto out = rg.select(names);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 3. Pass several feature names inline.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.select({"ids"});
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 4. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.select({"ids"});
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 5. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.select("ids");
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#save_txt">
    <span class="label">Previous</span>
    <span class="title">save_txt</span>
  </a>
  <a class="next-link" href="#set">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>

### `set`

#### Signatures

```cpp
Registry::set(std::initializer_list<std::pair<std::string, Slot>> items) -> Registry

Registry::set(std::initializer_list<std::pair<std::string_view, Slot>> items) -> Registry

Registry::set(std::string_view key, const Slot& value) -> Registry
```

#### What It Does

Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.

#### Parameters

**items**
    Inline collection passed directly at the call site.

**key**
    Name of the target slot or stream.

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.set();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.set();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.set();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#select">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="#set_">
    <span class="label">Next</span>
    <span class="title">set_</span>
  </a>
</div>

### `set_`

#### Signatures

```cpp
Registry::set_(std::initializer_list<std::pair<std::string, Slot>> items) -> Registry&

Registry::set_(std::initializer_list<std::pair<std::string_view, Slot>> items) -> Registry&

Registry::set_(std::string_view key, const Slot& value) -> Registry&
```

#### What It Does

Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.

#### Parameters

**items**
    Inline collection passed directly at the call site.

**key**
    Name of the target slot or stream.

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

#### Returns

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.set_();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.set_();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
Example 3. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.set_();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#set">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="#shape">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>

### `shape`

#### Signature

```cpp
Registry::shape() -> std::pair<std::int64_t, std::int64_t>
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.shape();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#set_">
    <span class="label">Previous</span>
    <span class="title">set_</span>
  </a>
  <a class="next-link" href="#show">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>

### `show`

#### Signature

```cpp
Registry::show(std::ostream& os) -> void
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.show(std::cout);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
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
Registry::size() -> std::int64_t
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.size();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#show">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="#sizes">
    <span class="label">Next</span>
    <span class="title">sizes</span>
  </a>
</div>

### `sizes`

#### Signature

```cpp
Registry::sizes() -> std::unordered_map<std::string, std::int64_t>
```

#### What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

#### Parameters

`None`

#### Returns

`std::unordered_map<std::string, std::int64_t>`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.sizes();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#size">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="#slice">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>

### `slice`

#### Signature

```cpp
Registry::slice(std::int64_t start, std::int64_t stop) -> Registry
```

#### What It Does

Returns a contiguous positional slice `[start, stop)` across every slot.

#### Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Inspect one contiguous positional slice from the current object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.slice(0, 2);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#sizes">
    <span class="label">Previous</span>
    <span class="title">sizes</span>
  </a>
  <a class="next-link" href="#sum">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>

### `sum`

#### Signature

```cpp
Registry::sum(std::string_view field) -> Slot
```

#### What It Does

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Parameters

**field**
    Name of the target stream in the catalog.

#### Returns

`Slot`
    Value produced by this call.

#### Examples

Example 1. Compute `sum` and inspect the resulting summary value or summary object.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.sum();
```

##### Output

```text
The call completes and returns the contract described under Output.
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#slice">
    <span class="label">Previous</span>
    <span class="title">slice</span>
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
Registry::tail(std::int64_t n) -> Registry
```

#### What It Does

Returns the last `n` values from every slot in the registry.

#### Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Inspect the last part of the current object without changing the source data.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.tail(2);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#sum">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="#take">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>

### `take`

#### Signatures

```cpp
Registry::take(const std::vector<std::int64_t>& indices) -> Registry

Registry::take(std::span<const std::int64_t> indices) -> Registry
```

#### What It Does

Returns the values at the requested integer positions across every slot.

#### Parameters

**indices**
    Integer positions used to select, reorder, or address entries.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Pass several integer positions together.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = rg.take(picks);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
Example 2. Pass several integer positions together.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = rg.take(picks);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#tail">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="#take_values">
    <span class="label">Next</span>
    <span class="title">take_values</span>
  </a>
</div>

### `take_values`

#### Signature

```cpp
Registry::take_values(std::string_view key, std::span<const std::int64_t> indices) -> Registry
```

#### What It Does

Selects values from one named slot by integer position and returns the updated registry.

#### Parameters

**key**
    Name of the target slot or stream.

**indices**
    Integer positions used to select, reorder, or address entries.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Pass several integer positions together.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = rg.take_values("ids", picks);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#take">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="#take_values_">
    <span class="label">Next</span>
    <span class="title">take_values_</span>
  </a>
</div>

### `take_values_`

#### Signature

```cpp
Registry::take_values_(std::string_view key, std::span<const std::int64_t> indices) -> Registry&
```

#### What It Does

Selects values from one named slot by integer position in place.

#### Parameters

**key**
    Name of the target slot or stream.

**indices**
    Integer positions used to select, reorder, or address entries.

#### Returns

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Pass several integer positions together.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
rg.take_values_("ids", picks);
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#take_values">
    <span class="label">Previous</span>
    <span class="title">take_values</span>
  </a>
  <a class="next-link" href="#to_json">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>

### `to_json`

#### Signature

```cpp
Registry::to_json() -> std::string
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.to_json();
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#take_values_">
    <span class="label">Previous</span>
    <span class="title">take_values_</span>
  </a>
  <a class="next-link" href="#to_kindle">
    <span class="label">Next</span>
    <span class="title">to_kindle</span>
  </a>
</div>

### `to_kindle`

#### Signature

```cpp
Registry::to_kindle() -> pluss::kindle::Catalog
```

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Parameters

`None`

#### Returns

`pluss::kindle::Catalog`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.to_kindle();
```

##### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
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
Registry::to_string() -> std::string
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.to_string();
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_kindle">
    <span class="label">Previous</span>
    <span class="title">to_kindle</span>
  </a>
  <a class="next-link" href="#transform_values">
    <span class="label">Next</span>
    <span class="title">transform_values</span>
  </a>
</div>

### `transform_values`

#### Signature

```cpp
Registry::transform_values(std::string_view key, const std::function<Slot::Value(const Slot::Value&)>& fn) -> Registry
```

#### What It Does

Rewrites the values inside one named slot with a user function and returns the updated registry.

#### Parameters

**key**
    Name of the target slot or stream.

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Registry`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.transform_values("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#to_string">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="#transform_values_">
    <span class="label">Next</span>
    <span class="title">transform_values_</span>
  </a>
</div>

### `transform_values_`

#### Signature

```cpp
Registry::transform_values_(std::string_view key, const std::function<Slot::Value(const Slot::Value&)>& fn) -> Registry&
```

#### What It Does

Rewrites the values inside one named slot in place.

#### Parameters

**key**
    Name of the target slot or stream.

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Returns

`Registry&`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.transform_values_("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
```

##### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#transform_values">
    <span class="label">Previous</span>
    <span class="title">transform_values</span>
  </a>
  <a class="next-link" href="#type_profiles">
    <span class="label">Next</span>
    <span class="title">type_profiles</span>
  </a>
</div>

### `type_profiles`

#### Signature

```cpp
Registry::type_profiles() -> std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>>
```

#### What It Does

Summarizes the value types stored inside each slot.

#### Parameters

`None`

#### Returns

`std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>>`
    Value produced by this call.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.type_profiles();
```

##### Output

```text
{"ids": {"Series": 3}, "city": {"Note": 3}}
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#transform_values_">
    <span class="label">Previous</span>
    <span class="title">transform_values_</span>
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
Registry::unique() -> bool
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.unique();
```

##### Output

```text
true
```
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#type_profiles">
    <span class="label">Previous</span>
    <span class="title">type_profiles</span>
  </a>
</div>
