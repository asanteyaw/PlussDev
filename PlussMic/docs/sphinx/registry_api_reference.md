# Registry API Reference

# Practical Registry API Reference

## API Reference Style

This page documents the full public `Registry` surface for named bundle slots.

### `apply`

#### Signature

```cpp
Registry::apply(key: std::string_view, fn: const std::function<Slot(const Slot&)>&) -> Registry
```

#### Notes

Applies a function to one or more named slots and returns the updated registry.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`fn` : `const std::function<Slot(const Slot&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::apply(keys: std::initializer_list<std::string_view>, fn: const std::function<Slot(const Slot&)>&) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

#### Expected output

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


### `apply`

#### Signature

```cpp
Registry::apply(keys: std::initializer_list<std::string_view>, fn: const std::function<Slot(const Slot&)>&) -> Registry
```

#### Notes

Applies a function to one or more named slots and returns the updated registry.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

`fn` : `const std::function<Slot(const Slot&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::apply(key: std::string_view, fn: const std::function<Slot(const Slot&)>&) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply({"ids"}, [](const pluss::bundle::Slot& slot) { return slot; });
```

#### Expected output

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


### `apply_`

#### Signature

```cpp
Registry::apply_(key: std::string_view, fn: const std::function<Slot(const Slot&)>&) -> Registry&
```

#### Notes

Applies a function to one or more named slots in place.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`fn` : `const std::function<Slot(const Slot&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::apply_(keys: std::initializer_list<std::string_view>, fn: const std::function<Slot(const Slot&)>&) -> Registry&`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.apply_("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `apply_`

#### Signature

```cpp
Registry::apply_(keys: std::initializer_list<std::string_view>, fn: const std::function<Slot(const Slot&)>&) -> Registry&
```

#### Notes

Applies a function to one or more named slots in place.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

`fn` : `const std::function<Slot(const Slot&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::apply_(key: std::string_view, fn: const std::function<Slot(const Slot&)>&) -> Registry&`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.apply_({"ids"}, [](const pluss::bundle::Slot& slot) { return slot; });
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `apply_all`

#### Signature

```cpp
Registry::apply_all(fn: const std::function<Slot(const Slot&)>&) -> Registry
```

#### Notes

Applies the same function to every slot and returns the updated registry.

#### Inputs

`fn` : `const std::function<Slot(const Slot&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply_all([](const pluss::bundle::Slot& slot) { return slot; });
```

#### Expected output

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


### `as_kindle`

#### Signature

```cpp
Registry::as_kindle() -> pluss::kindle::Catalog
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`None`

#### Output

`pluss::kindle::Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.as_kindle();
```

#### Expected output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Bridge Behavior

- `as_kindle()` converts the registry into the Kindle/Catalog model.
- Use this when the next operation is stream-oriented rather than bundle-oriented.


### `at`

#### Signature

```cpp
Registry::at(key: std::string_view) -> Slot
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`Slot`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.at("ids");
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

#### Practical Behavior

- `at(key)` fetches one named slot from the registry.
- Use it when you want direct slot access before applying slot-level transforms.


### `begin`

#### Signature

```cpp
Registry::begin() -> const_iterator
```

#### Notes

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

#### Inputs

`None`

#### Output

`const_iterator`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

for (const auto& key : rg) {
  (void)key;
}
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `clear`

#### Signature

```cpp
Registry::clear() -> Registry
```

#### Notes

Removes all slots and returns an empty registry.

#### Inputs

`None`

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.clear();
```

#### Expected output

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


### `clear_`

#### Signature

```cpp
Registry::clear_() -> Registry&
```

#### Notes

Removes all slots in place.

#### Inputs

`None`

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.clear_();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `cloc`

#### Signature

```cpp
Registry::cloc(row: std::int64_t) -> Registry
```

#### Notes



#### Inputs

`row` : `std::int64_t`
    Zero-based row position used by this overload.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::cloc(start: std::int64_t, stop: std::int64_t) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.cloc();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

#### Practical Behavior

- `cloc(...)` slices positional rows across every slot while keeping slot alignment intact.


### `cloc`

#### Signature

```cpp
Registry::cloc(start: std::int64_t, stop: std::int64_t) -> Registry
```

#### Notes



#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::cloc(row: std::int64_t) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.cloc();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

#### Practical Behavior

- `cloc(...)` slices positional rows across every slot while keeping slot alignment intact.


### `combine`

#### Signature

```cpp
Registry::combine(other: const Registry&, suffix: std::string = "_other") -> Registry
```

#### Notes

Merges slots from another registry into this one, applying the collision suffix when keys overlap.

#### Inputs

`other` : `const Registry&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`suffix` : `std::string`
    Text suffix appended after the existing label or value.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto other = pluss::bundle::Registry({{"flag", pluss::bundle::Slot({pluss::make_series(true, pluss::ScalarType::Bool), pluss::make_series(false, pluss::ScalarType::Bool), pluss::make_series(true, pluss::ScalarType::Bool)})}});
auto out = rg.combine(other, "_rhs");
```

#### Expected output

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


### `concat_values`

#### Signature

```cpp
Registry::concat_values(key: std::string_view, other: const Slot&) -> Registry
```

#### Notes

Appends extra values onto one named slot and returns the updated registry.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`other` : `const Slot&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
auto out = rg.concat_values("ids", extra);
```

#### Expected output

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


### `concat_values_`

#### Signature

```cpp
Registry::concat_values_(key: std::string_view, other: const Slot&) -> Registry&
```

#### Notes

Appends extra values onto one named slot in place.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`other` : `const Slot&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
rg.concat_values_("ids", extra);
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `defined`

#### Signature

```cpp
Registry::defined() -> bool
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.defined();
```

#### Expected output

```text
true
```

### `drop`

#### Signature

```cpp
Registry::drop(key: std::string_view) -> Registry
```

#### Notes

Removes the named slots and keeps the remaining registry structure intact.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::drop(keys: std::initializer_list<std::string_view>) -> Registry`
- `Registry::drop(keys: const std::vector<std::string>&) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop("city");
```

#### Expected output

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


### `drop`

#### Signature

```cpp
Registry::drop(keys: const std::vector<std::string>&) -> Registry
```

#### Notes

Removes the named slots and keeps the remaining registry structure intact.

#### Inputs

`keys` : `const std::vector<std::string>&`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::drop(keys: std::initializer_list<std::string_view>) -> Registry`
- `Registry::drop(key: std::string_view) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string> names{"city"};
auto out = rg.drop(names);
```

#### Expected output

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


### `drop`

#### Signature

```cpp
Registry::drop(keys: std::initializer_list<std::string_view>) -> Registry
```

#### Notes

Removes the named slots and keeps the remaining registry structure intact.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::drop(keys: const std::vector<std::string>&) -> Registry`
- `Registry::drop(key: std::string_view) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop({"city"});
```

#### Expected output

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


### `drop_empty_slots`

#### Signature

```cpp
Registry::drop_empty_slots() -> Registry
```

#### Notes

Removes slots whose payload is empty and returns the reduced registry.

#### Inputs

`None`

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"empty", pluss::bundle::Slot({})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop_empty_slots();
```

#### Expected output

```text
rg.head():
ids    -> Slot([Series(1), Series(2), Series(3)])
empty  -> Slot([])
city   -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

#### Practical Behavior

- These methods remove slots whose payload is empty.
- Non-empty slots are passed through unchanged.


### `drop_empty_slots_`

#### Signature

```cpp
Registry::drop_empty_slots_() -> Registry&
```

#### Notes

Removes empty slots in place.

#### Inputs

`None`

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"empty", pluss::bundle::Slot({})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.drop_empty_slots_();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

#### Practical Behavior

- These methods remove slots whose payload is empty.
- Non-empty slots are passed through unchanged.


### `empty`

#### Signature

```cpp
Registry::empty() -> bool
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.empty();
```

#### Expected output

```text
false
```

### `end`

#### Signature

```cpp
Registry::end() -> const_iterator
```

#### Notes

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

#### Inputs

`None`

#### Output

`const_iterator`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

for (const auto& key : rg) {
  (void)key;
}
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `ensure_stream_compatible`

#### Signature

```cpp
Registry::ensure_stream_compatible(where: std::string_view = "Registry") -> void
```

#### Notes

Validates that the registry can be converted to a Kindle catalog without incompatible slot contents.

#### Inputs

`where` : `std::string_view`
    Filter expression or selector string interpreted by this overload.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.ensure_stream_compatible("demo");
```

#### Expected output

```text
no error
```

### `field_names`

#### Signature

```cpp
Registry::field_names() -> const std::vector<std::string>&
```

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

#### Inputs

`None`

#### Output

`const std::vector<std::string>&`
    Borrowed list of names.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.field_names();
```

#### Expected output

```text
["ids", "city"]
```

### `filter_values`

#### Signature

```cpp
Registry::filter_values(key: std::string_view, predicate: const std::function<bool(const Slot::Value&)>&) -> Registry
```

#### Notes

Keeps only the values in one named slot that satisfy the supplied predicate.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`predicate` : `const std::function<bool(const Slot::Value&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.filter_values("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `filter_values_`

#### Signature

```cpp
Registry::filter_values_(key: std::string_view, predicate: const std::function<bool(const Slot::Value&)>&) -> Registry&
```

#### Notes

Filters the values in one named slot in place.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`predicate` : `const std::function<bool(const Slot::Value&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.filter_values_("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `from_kindle`

#### Signature

```cpp
Registry::from_kindle(catalog: const pluss::kindle::Catalog&) -> Registry
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`catalog` : `const pluss::kindle::Catalog&`
    Catalog payload supplied to this overload.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = pluss::bundle::Registry::from_kindle(catalog);
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `get`

#### Signature

```cpp
Registry::get(key: std::string_view) -> std::optional<Slot>
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`std::optional<Slot>`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.get("ids");
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `get_slot`

#### Signature

```cpp
Registry::get_slot(key: std::string_view) -> std::optional<Slot>
```

#### Notes

Returns one named slot from the registry.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`std::optional<Slot>`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.get_slot("ids");
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `has`

#### Signature

```cpp
Registry::has(key: std::string_view) -> bool
```

#### Notes

Checks whether a named slot exists.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has("ids");
```

#### Expected output

```text
true
```

### `has_all`

#### Signature

```cpp
Registry::has_all(keys: std::initializer_list<std::string_view>) -> bool
```

#### Notes

Checks whether every requested slot name exists.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_all({"ids", "city"});
```

#### Expected output

```text
true
```

### `has_any`

#### Signature

```cpp
Registry::has_any(keys: std::initializer_list<std::string_view>) -> bool
```

#### Notes

Checks whether at least one requested slot name exists.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_any({"ids", "city"});
```

#### Expected output

```text
true
```

### `has_slot`

#### Signature

```cpp
Registry::has_slot(key: std::string_view) -> bool
```

#### Notes

Checks whether a named slot exists.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_slot("ids");
```

#### Expected output

```text
true
```

### `head`

#### Signature

```cpp
Registry::head(n: std::int64_t) -> Registry
```

#### Notes

Returns the first `n` values from every slot in the registry.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.head(2);
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `is_stream_compatible`

#### Signature

```cpp
Registry::is_stream_compatible() -> bool
```

#### Notes

Reports whether the registry can be converted to a Kindle catalog without incompatible slot contents.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.is_stream_compatible();
```

#### Expected output

```text
true
```

### `keys`

#### Signature

```cpp
Registry::keys() -> const std::vector<std::string>&
```

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

#### Inputs

`None`

#### Output

`const std::vector<std::string>&`
    Borrowed list of names.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.keys();
```

#### Expected output

```text
["ids", "city"]
```

### `kindelize`

#### Signature

```cpp
Registry::kindelize() -> pluss::kindle::Catalog
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`None`

#### Output

`pluss::kindle::Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.kindelize();
```

#### Expected output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load`

#### Signature

```cpp
Registry::load(source_type: std::string_view, filepath: std::string_view) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load(source_type: std::string_view, filepath: std::string_view, config: const pluss::loader::Config&) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load("csv", "path/to/data.csv");
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load`

#### Signature

```cpp
Registry::load(source_type: std::string_view, filepath: std::string_view, config: const pluss::loader::Config&) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load(source_type: std::string_view, filepath: std::string_view) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load("csv", "path/to/data.csv");
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_csv`

#### Signature

```cpp
Registry::load_csv(filepath: std::string_view) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_csv(filepath: std::string_view, options: const pluss::loader::ReadCsvOptions&) -> Registry`
- `Registry::load_csv(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_csv("path/to/data.csv");
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_csv`

#### Signature

```cpp
Registry::load_csv(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_csv(filepath: std::string_view) -> Registry`
- `Registry::load_csv(filepath: std::string_view, options: const pluss::loader::ReadCsvOptions&) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_csv`

#### Signature

```cpp
Registry::load_csv(filepath: std::string_view, options: const pluss::loader::ReadCsvOptions&) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::ReadCsvOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_csv(filepath: std::string_view) -> Registry`
- `Registry::load_csv(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry`

#### Examples

```cpp
pluss::loader::ReadCsvOptions options;
auto out = pluss::bundle::Registry::load_csv("path/to/data.csv", options);
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_excel`

#### Signature

```cpp
Registry::load_excel(filepath: std::string_view) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_excel(filepath: std::string_view, options: const pluss::loader::ReadExcelOptions&) -> Registry`
- `Registry::load_excel(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_excel("path/to/data.xlsx");
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_excel`

#### Signature

```cpp
Registry::load_excel(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_excel(filepath: std::string_view) -> Registry`
- `Registry::load_excel(filepath: std::string_view, options: const pluss::loader::ReadExcelOptions&) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_excel("path/to/data.xlsx", {"nrows", 16});
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_excel`

#### Signature

```cpp
Registry::load_excel(filepath: std::string_view, options: const pluss::loader::ReadExcelOptions&) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::ReadExcelOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_excel(filepath: std::string_view) -> Registry`
- `Registry::load_excel(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry`

#### Examples

```cpp
pluss::loader::ReadExcelOptions options;
auto out = pluss::bundle::Registry::load_excel("path/to/data.xlsx", options);
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_json`

#### Signature

```cpp
Registry::load_json(filepath: std::string_view) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_json(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_json("path/to/data.json");
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_json`

#### Signature

```cpp
Registry::load_json(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_json(filepath: std::string_view) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_json("path/to/data.json", {"nrows", 16});
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_parquet`

#### Signature

```cpp
Registry::load_parquet(filepath: std::string_view) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_parquet(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_parquet("path/to/data.parquet");
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_parquet`

#### Signature

```cpp
Registry::load_parquet(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_parquet(filepath: std::string_view) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_parquet("path/to/data.parquet", {"nrows", 16});
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_txt`

#### Signature

```cpp
Registry::load_txt(filepath: std::string_view) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_txt(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_txt("path/to/data.txt");
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_txt`

#### Signature

```cpp
Registry::load_txt(filepath: std::string_view, config: const pluss::loader::Config&) -> Registry
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::load_txt(filepath: std::string_view) -> Registry`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_txt("path/to/data.txt", {"nrows", 16});
```

#### Expected output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `map`

#### Signature

```cpp
Registry::map(fn: const std::function<Slot(const Slot&)>&) -> Registry
```

#### Notes

Transforms every slot with the same function and returns the updated registry.

#### Inputs

`fn` : `const std::function<Slot(const Slot&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.map([](const pluss::bundle::Slot& slot) { return slot; });
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `map_`

#### Signature

```cpp
Registry::map_(fn: const std::function<Slot(const Slot&)>&) -> Registry&
```

#### Notes

Transforms every slot in place with the same function.

#### Inputs

`fn` : `const std::function<Slot(const Slot&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.map_([](const pluss::bundle::Slot& slot) { return slot; });
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `nfields`

#### Signature

```cpp
Registry::nfields() -> std::int64_t
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.nfields();
```

#### Expected output

```text
2
```

### `nslots`

#### Signature

```cpp
Registry::nslots() -> std::int64_t
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.nslots();
```

#### Expected output

```text
2
```

### `push`

#### Signature

```cpp
Registry::push(key: std::string_view, value: const Slot::Value&) -> Registry
```

#### Notes

Appends one new value to a named slot and returns the updated registry.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Slot::Value&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.push("ids", pluss::make_series(4));
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `push_`

#### Signature

```cpp
Registry::push_(key: std::string_view, value: const Slot::Value&) -> Registry&
```

#### Notes

Appends one new value to a named slot in place.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Slot::Value&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.push_("ids", pluss::make_series(4));
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `push_all`

#### Signature

```cpp
Registry::push_all(value: const Slot::Value&) -> Registry
```

#### Notes

Appends the same new value to every slot and returns the updated registry.

#### Inputs

`value` : `const Slot::Value&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.push_all(pluss::make_note("ok"));
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `push_all_`

#### Signature

```cpp
Registry::push_all_(value: const Slot::Value&) -> Registry&
```

#### Notes

Appends the same new value to every slot in place.

#### Inputs

`value` : `const Slot::Value&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.push_all_(pluss::make_note("ok"));
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `rename`

#### Signature

```cpp
Registry::rename(from: std::string_view, to: std::string_view) -> Registry
```

#### Notes

Renames one or more slots without changing their stored values.

#### Inputs

`from` : `std::string_view`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `std::string_view`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::rename(mapping: std::initializer_list<std::pair<std::string_view, std::string_view>>) -> Registry`
- `Registry::rename(mapping: const std::unordered_map<std::string, std::string>&) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.rename("ids", "ids_new");
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `rename`

#### Signature

```cpp
Registry::rename(mapping: const std::unordered_map<std::string, std::string>&) -> Registry
```

#### Notes

Renames one or more slots without changing their stored values.

#### Inputs

`mapping` : `const std::unordered_map<std::string, std::string>&`
    Name-to-name mapping used by this overload.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::rename(mapping: std::initializer_list<std::pair<std::string_view, std::string_view>>) -> Registry`
- `Registry::rename(from: std::string_view, to: std::string_view) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::unordered_map<std::string, std::string> renames{{"ids", "ids_new"}};
auto out = rg.rename(renames);
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `rename`

#### Signature

```cpp
Registry::rename(mapping: std::initializer_list<std::pair<std::string_view, std::string_view>>) -> Registry
```

#### Notes

Renames one or more slots without changing their stored values.

#### Inputs

`mapping` : `std::initializer_list<std::pair<std::string_view, std::string_view>>`
    Inline collection passed directly at the call site.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::rename(mapping: const std::unordered_map<std::string, std::string>&) -> Registry`
- `Registry::rename(from: std::string_view, to: std::string_view) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.rename({{"ids", "ids_new"}});
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `rename_`

#### Signature

```cpp
Registry::rename_(from: std::string_view, to: std::string_view) -> Registry&
```

#### Notes

Renames one or more slots in place without changing their stored values.

#### Inputs

`from` : `std::string_view`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `std::string_view`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.rename_("ids", "ids_new");
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `sample`

#### Signature

```cpp
Registry::sample(n: std::int64_t, random_state: std::optional<std::uint64_t> = std::nullopt) -> Registry
```

#### Notes

Returns a random sample of positions across every slot.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`random_state` : `std::optional<std::uint64_t>`
    Optional seed used to make sampling or shuffling reproducible.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.sample(2, 7);
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `save_as`

#### Signature

```cpp
Registry::save_as(target_type: std::string_view, filepath: std::string_view, config: const pluss::loader::Config& = {}) -> void
```

#### Notes

Writes the registry using the explicitly supplied output format selector.

#### Inputs

`target_type` : `std::string_view`
    Format or target selector string used by this overload.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_as("json", "path/to/output.json");
```

#### Expected output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
Registry::save_csv(filepath: std::string_view) -> void
```

#### Notes

Writes the registry to CSV after materializing each slot into a tabular export form.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::save_csv(filepath: std::string_view, config: const pluss::loader::Config&) -> void`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_csv("path/to/output.csv");
```

#### Expected output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
Registry::save_csv(filepath: std::string_view, config: const pluss::loader::Config&) -> void
```

#### Notes

Writes the registry to CSV after materializing each slot into a tabular export form.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::save_csv(filepath: std::string_view) -> void`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_csv("path/to/output.csv");
```

#### Expected output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
Registry::save_excel(filepath: std::string_view) -> void
```

#### Notes

Writes the registry to an Excel workbook after materializing each slot into a tabular export form.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::save_excel(filepath: std::string_view, config: const pluss::loader::Config&) -> void`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_excel("path/to/output.xlsx");
```

#### Expected output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
Registry::save_excel(filepath: std::string_view, config: const pluss::loader::Config&) -> void
```

#### Notes

Writes the registry to an Excel workbook after materializing each slot into a tabular export form.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::save_excel(filepath: std::string_view) -> void`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_excel("path/to/output.xlsx");
```

#### Expected output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
Registry::save_json(filepath: std::string_view) -> void
```

#### Notes

Writes the registry to JSON for interchange or later reload.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::save_json(filepath: std::string_view, config: const pluss::loader::Config&) -> void`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_json("path/to/output.json");
```

#### Expected output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
Registry::save_json(filepath: std::string_view, config: const pluss::loader::Config&) -> void
```

#### Notes

Writes the registry to JSON for interchange or later reload.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::save_json(filepath: std::string_view) -> void`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_json("path/to/output.json");
```

#### Expected output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
Registry::save_parquet(filepath: std::string_view) -> void
```

#### Notes

Writes the registry to Parquet for compact typed storage.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::save_parquet(filepath: std::string_view, config: const pluss::loader::Config&) -> void`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_parquet("path/to/output.parquet");
```

#### Expected output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
Registry::save_parquet(filepath: std::string_view, config: const pluss::loader::Config&) -> void
```

#### Notes

Writes the registry to Parquet for compact typed storage.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::save_parquet(filepath: std::string_view) -> void`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_parquet("path/to/output.parquet");
```

#### Expected output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
Registry::save_txt(filepath: std::string_view) -> void
```

#### Notes

Writes a text rendering of the registry for inspection or logging.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::save_txt(filepath: std::string_view, config: const pluss::loader::Config&) -> void`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_txt("path/to/output.txt");
```

#### Expected output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
Registry::save_txt(filepath: std::string_view, config: const pluss::loader::Config&) -> void
```

#### Notes

Writes a text rendering of the registry for inspection or logging.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::save_txt(filepath: std::string_view) -> void`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_txt("path/to/output.txt");
```

#### Expected output

```text
saved: out.*
```

### `select`

#### Signature

```cpp
Registry::select(key: std::string_view) -> Registry
```

#### Notes

Keeps only the named slots and preserves their shared positional alignment.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::select(keys: std::initializer_list<std::string_view>) -> Registry`
- `Registry::select(keys: const std::vector<std::string>&) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.select("ids");
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `select`

#### Signature

```cpp
Registry::select(keys: const std::vector<std::string>&) -> Registry
```

#### Notes

Keeps only the named slots and preserves their shared positional alignment.

#### Inputs

`keys` : `const std::vector<std::string>&`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::select(keys: std::initializer_list<std::string_view>) -> Registry`
- `Registry::select(key: std::string_view) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string> names{"ids"};
auto out = rg.select(names);
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `select`

#### Signature

```cpp
Registry::select(keys: std::initializer_list<std::string_view>) -> Registry
```

#### Notes

Keeps only the named slots and preserves their shared positional alignment.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::select(keys: const std::vector<std::string>&) -> Registry`
- `Registry::select(key: std::string_view) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.select({"ids"});
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `set_slot`

#### Signature

```cpp
Registry::set_slot(key: std::string_view, value: const Slot&) -> Registry
```

#### Notes

Adds or replaces one named slot and returns the updated registry.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Slot&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.set_slot("score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)}));
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `set_slot_`

#### Signature

```cpp
Registry::set_slot_(key: std::string_view, value: const Slot&) -> Registry&
```

#### Notes

Adds or replaces one named slot in place.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Slot&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.set_slot_("score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)}));
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `set_slots`

#### Signature

```cpp
Registry::set_slots(items: std::initializer_list<std::pair<std::string, Slot>>) -> Registry
```

#### Notes

Adds or replaces several named slots in one call and returns the updated registry.

#### Inputs

`items` : `std::initializer_list<std::pair<std::string, Slot>>`
    Inline collection passed directly at the call site.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::set_slots(items: std::initializer_list<std::pair<std::string_view, Slot>>) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.set_slots({
    {"score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)})},
});
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `set_slots`

#### Signature

```cpp
Registry::set_slots(items: std::initializer_list<std::pair<std::string_view, Slot>>) -> Registry
```

#### Notes

Adds or replaces several named slots in one call and returns the updated registry.

#### Inputs

`items` : `std::initializer_list<std::pair<std::string_view, Slot>>`
    Inline collection passed directly at the call site.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::set_slots(items: std::initializer_list<std::pair<std::string, Slot>>) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.set_slots({
    {"score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)})},
});
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `set_slots_`

#### Signature

```cpp
Registry::set_slots_(items: std::initializer_list<std::pair<std::string, Slot>>) -> Registry&
```

#### Notes

Adds or replaces several named slots in place.

#### Inputs

`items` : `std::initializer_list<std::pair<std::string, Slot>>`
    Inline collection passed directly at the call site.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::set_slots_(items: std::initializer_list<std::pair<std::string_view, Slot>>) -> Registry&`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.set_slots_({
    {"score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)})},
});
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `set_slots_`

#### Signature

```cpp
Registry::set_slots_(items: std::initializer_list<std::pair<std::string_view, Slot>>) -> Registry&
```

#### Notes

Adds or replaces several named slots in place.

#### Inputs

`items` : `std::initializer_list<std::pair<std::string_view, Slot>>`
    Inline collection passed directly at the call site.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::set_slots_(items: std::initializer_list<std::pair<std::string, Slot>>) -> Registry&`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.set_slots_({
    {"score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)})},
});
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `show`

#### Signature

```cpp
Registry::show(os: std::ostream&) -> void
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.show(std::cout);
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `slice`

#### Signature

```cpp
Registry::slice(start: std::int64_t, stop: std::int64_t) -> Registry
```

#### Notes

Returns a contiguous positional slice `[start, stop)` across every slot.

#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.slice(0, 2);
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `slot`

#### Signature

```cpp
Registry::slot(key: std::string_view) -> Slot
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`Slot`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.slot("ids");
```

#### Expected output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `slot_sizes`

#### Signature

```cpp
Registry::slot_sizes() -> std::unordered_map<std::string, std::int64_t>
```

#### Notes

Returns the current length of each slot.

#### Inputs

`None`

#### Output

`std::unordered_map<std::string, std::int64_t>`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.slot_sizes();
```

#### Expected output

```text
{"ids": 3, "city": 3}
```

### `slots`

#### Signature

```cpp
Registry::slots() -> std::vector<Slot>
```

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Inputs

`None`

#### Output

`std::vector<Slot>`
    Materialized collection of slot objects.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.slots();
```

#### Expected output

```text
[Slot(ids), Slot(city)]
```

### `tail`

#### Signature

```cpp
Registry::tail(n: std::int64_t) -> Registry
```

#### Notes

Returns the last `n` values from every slot in the registry.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.tail(2);
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `take`

#### Signature

```cpp
Registry::take(indices: const std::vector<std::int64_t>&) -> Registry
```

#### Notes

Returns the values at the requested integer positions across every slot.

#### Inputs

`indices` : `const std::vector<std::int64_t>&`
    Integer positions used to select, reorder, or address entries.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::take(indices: std::span<const std::int64_t>) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = rg.take(picks);
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `take`

#### Signature

```cpp
Registry::take(indices: std::span<const std::int64_t>) -> Registry
```

#### Notes

Returns the values at the requested integer positions across every slot.

#### Inputs

`indices` : `std::span<const std::int64_t>`
    Integer positions used to select, reorder, or address entries.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Registry::take(indices: const std::vector<std::int64_t>&) -> Registry`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = rg.take(picks);
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `take_values`

#### Signature

```cpp
Registry::take_values(key: std::string_view, indices: std::span<const std::int64_t>) -> Registry
```

#### Notes

Selects values from one named slot by integer position and returns the updated registry.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`indices` : `std::span<const std::int64_t>`
    Integer positions used to select, reorder, or address entries.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = rg.take_values("ids", picks);
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `take_values_`

#### Signature

```cpp
Registry::take_values_(key: std::string_view, indices: std::span<const std::int64_t>) -> Registry&
```

#### Notes

Selects values from one named slot by integer position in place.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`indices` : `std::span<const std::int64_t>`
    Integer positions used to select, reorder, or address entries.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
rg.take_values_("ids", picks);
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `to_json`

#### Signature

```cpp
Registry::to_json() -> std::string
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.to_json();
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `to_kindle`

#### Signature

```cpp
Registry::to_kindle() -> pluss::kindle::Catalog
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`None`

#### Output

`pluss::kindle::Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.to_kindle();
```

#### Expected output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `to_string`

#### Signature

```cpp
Registry::to_string() -> std::string
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.to_string();
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `transform_values`

#### Signature

```cpp
Registry::transform_values(key: std::string_view, fn: const std::function<Slot::Value(const Slot::Value&)>&) -> Registry
```

#### Notes

Rewrites the values inside one named slot with a user function and returns the updated registry.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`fn` : `const std::function<Slot::Value(const Slot::Value&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.transform_values("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
```

#### Expected output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `transform_values_`

#### Signature

```cpp
Registry::transform_values_(key: std::string_view, fn: const std::function<Slot::Value(const Slot::Value&)>&) -> Registry&
```

#### Notes

Rewrites the values inside one named slot in place.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`fn` : `const std::function<Slot::Value(const Slot::Value&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Registry&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.transform_values_("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `type_profiles`

#### Signature

```cpp
Registry::type_profiles() -> std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>>
```

#### Notes

Summarizes the value types stored inside each slot.

#### Inputs

`None`

#### Output

`std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>>`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.type_profiles();
```

#### Expected output

```text
{"ids": {"Series": 3}, "city": {"Note": 3}}
```

### `unique`

#### Signature

```cpp
Registry::unique() -> bool
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.unique();
```

#### Expected output

```text
true
```
