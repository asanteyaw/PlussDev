# Registry API Reference

# Practical Registry API Reference

## API Reference Style

This page documents the full public `Registry` surface for named bundle slots.

### `apply`

#### Signature

```cpp
Registry apply(self, std::initializer_list<std::string_view> keys, const std::function<Slot(const Slot&)>& fn) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Applies a function to one or more named slots and returns the updated registry.

#### See also

- `Registry apply(self, std::string_view key, const std::function<Slot(const Slot&)>& fn) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply({"ids"}, [](const pluss::bundle::Slot& slot) { return slot; });
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `apply`

#### Signature

```cpp
Registry apply(self, std::string_view key, const std::function<Slot(const Slot&)>& fn) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Applies a function to one or more named slots and returns the updated registry.

#### See also

- `Registry apply(self, std::initializer_list<std::string_view> keys, const std::function<Slot(const Slot&)>& fn) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `apply_`

#### Signature

```cpp
Registry& apply_(self, std::initializer_list<std::string_view> keys, const std::function<Slot(const Slot&)>& fn)
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Applies a function to one or more named slots in place.

#### See also

- `Registry& apply_(self, std::string_view key, const std::function<Slot(const Slot&)>& fn)`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.apply_({"ids"}, [](const pluss::bundle::Slot& slot) { return slot; });
```

### `apply_`

#### Signature

```cpp
Registry& apply_(self, std::string_view key, const std::function<Slot(const Slot&)>& fn)
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Applies a function to one or more named slots in place.

#### See also

- `Registry& apply_(self, std::initializer_list<std::string_view> keys, const std::function<Slot(const Slot&)>& fn)`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.apply_("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

### `apply_all`

#### Signature

```cpp
Registry apply_all(self, const std::function<Slot(const Slot&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Applies the same function to every slot and returns the updated registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.apply_all([](const pluss::bundle::Slot& slot) { return slot; });
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `as_kindle`

#### Signature

```cpp
pluss::kindle::Catalog as_kindle(self) const
```

#### Parameters

`None`

#### Returns

`pluss::kindle::Catalog`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.as_kindle();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `at`

#### Signature

```cpp
Slot at(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.at("ids");
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `begin`

#### Signature

```cpp
const_iterator begin(self) const
```

#### Parameters

`None`

#### Returns

`const_iterator`
    Value produced by this call.

#### Notes

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

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

### `clear`

#### Signature

```cpp
Registry clear(self) const
```

#### Parameters

`None`

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Removes all slots and returns an empty registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.clear();
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `clear_`

#### Signature

```cpp
Registry& clear_(self)
```

#### Parameters

`None`

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Removes all slots in place.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.clear_();
```

### `combine`

#### Signature

```cpp
Registry combine(self, const Registry& other, std::string suffix = "_other") const
```

#### Parameters

`other` : `const Registry&`
    See the signature for the supported argument form.

`suffix` : `std::string`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Merges slots from another registry into this one, applying the collision suffix when keys overlap.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto other = pluss::bundle::Registry({{"flag", pluss::bundle::Slot({pluss::make_series(true, pluss::codem::ScalarType::Bool), pluss::make_series(false, pluss::codem::ScalarType::Bool), pluss::make_series(true, pluss::codem::ScalarType::Bool)})}});
auto out = rg.combine(other, "_rhs");
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `concat_values`

#### Signature

```cpp
Registry concat_values(self, std::string_view key, const Slot& other) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`other` : `const Slot&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Appends extra values onto one named slot and returns the updated registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
auto out = rg.concat_values("ids", extra);
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `concat_values_`

#### Signature

```cpp
Registry& concat_values_(self, std::string_view key, const Slot& other)
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`other` : `const Slot&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Appends extra values onto one named slot in place.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
rg.concat_values_("ids", extra);
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.defined();
```

#### Output

```text
true
```

### `drop`

#### Signature

```cpp
Registry drop(self, const std::vector<std::string>& keys) const
```

#### Parameters

`keys` : `const std::vector<std::string>&`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Removes the named slots and keeps the remaining registry structure intact.

#### See also

- `Registry drop(self, std::initializer_list<std::string_view> keys) const`
- `Registry drop(self, std::string_view key) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string> names{"city"};
auto out = rg.drop(names);
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `drop`

#### Signature

```cpp
Registry drop(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Removes the named slots and keeps the remaining registry structure intact.

#### See also

- `Registry drop(self, const std::vector<std::string>& keys) const`
- `Registry drop(self, std::string_view key) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop({"city"});
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `drop`

#### Signature

```cpp
Registry drop(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Removes the named slots and keeps the remaining registry structure intact.

#### See also

- `Registry drop(self, std::initializer_list<std::string_view> keys) const`
- `Registry drop(self, const std::vector<std::string>& keys) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop("city");
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `drop_empty_slots`

#### Signature

```cpp
Registry drop_empty_slots(self) const
```

#### Parameters

`None`

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Removes slots whose payload is empty and returns the reduced registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.drop_empty_slots();
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `drop_empty_slots_`

#### Signature

```cpp
Registry& drop_empty_slots_(self)
```

#### Parameters

`None`

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Removes empty slots in place.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.drop_empty_slots_();
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.empty();
```

#### Output

```text
false
```

### `end`

#### Signature

```cpp
const_iterator end(self) const
```

#### Parameters

`None`

#### Returns

`const_iterator`
    Value produced by this call.

#### Notes

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

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

### `ensure_stream_compatible`

#### Signature

```cpp
void ensure_stream_compatible(self, std::string_view where = "Registry") const
```

#### Parameters

`where` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Validates that the registry can be converted to a Kindle catalog without incompatible slot contents.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.ensure_stream_compatible("demo");
```

#### Output

```text
no error
```

### `field_names`

#### Signature

```cpp
const std::vector<std::string>& field_names(self) const
```

#### Parameters

`None`

#### Returns

`const std::vector<std::string>&`
    Borrowed list of names.

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.field_names();
```

#### Output

```text
["ids", "city"]
```

### `filter_values`

#### Signature

```cpp
Registry filter_values(self, std::string_view key, const std::function<bool(const Slot::Value&)>& predicate) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`predicate` : `const std::function<bool(const Slot::Value&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Keeps only the values in one named slot that satisfy the supplied predicate.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.filter_values("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
```

#### Output

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
Registry& filter_values_(self, std::string_view key, const std::function<bool(const Slot::Value&)>& predicate)
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`predicate` : `const std::function<bool(const Slot::Value&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Filters the values in one named slot in place.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.filter_values_("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
```

### `from_kindle`

#### Signature

```cpp
static Registry from_kindle(const pluss::kindle::Catalog& catalog)
```

#### Parameters

`catalog` : `const pluss::kindle::Catalog&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

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

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `get`

#### Signature

```cpp
std::optional<Slot> get(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`std::optional<Slot>`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.get("ids");
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `get_slot`

#### Signature

```cpp
std::optional<Slot> get_slot(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`std::optional<Slot>`
    Value produced by this call.

#### Notes

Returns one named slot from the registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.get_slot("ids");
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `has`

#### Signature

```cpp
bool has(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Checks whether a named slot exists.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has("ids");
```

#### Output

```text
true
```

### `has_all`

#### Signature

```cpp
bool has_all(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Checks whether every requested slot name exists.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_all({"ids", "city"});
```

#### Output

```text
true
```

### `has_any`

#### Signature

```cpp
bool has_any(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Checks whether at least one requested slot name exists.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_any({"ids", "city"});
```

#### Output

```text
true
```

### `has_slot`

#### Signature

```cpp
bool has_slot(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Checks whether a named slot exists.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.has_slot("ids");
```

#### Output

```text
true
```

### `head`

#### Signature

```cpp
Registry head(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Returns the first `n` values from every slot in the registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.head(2);
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `iloc`

#### Signature

```cpp
Registry iloc(self, std::int64_t row) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `Registry iloc(self, std::int64_t start, std::int64_t stop) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.iloc(1);
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])

out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `iloc`

#### Signature

```cpp
Registry iloc(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `Registry iloc(self, std::int64_t row) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.iloc(0, 2);
```

#### Output

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
bool is_stream_compatible(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Reports whether the registry can be converted to a Kindle catalog without incompatible slot contents.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.is_stream_compatible();
```

#### Output

```text
true
```

### `keys`

#### Signature

```cpp
const std::vector<std::string>& keys(self) const
```

#### Parameters

`None`

#### Returns

`const std::vector<std::string>&`
    Borrowed list of names.

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.keys();
```

#### Output

```text
["ids", "city"]
```

### `kindelize`

#### Signature

```cpp
pluss::kindle::Catalog kindelize(self) const
```

#### Parameters

`None`

#### Returns

`pluss::kindle::Catalog`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.kindelize();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load`

#### Signature

```cpp
static Registry load(std::string_view source_type, std::string_view filepath)
```

#### Parameters

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load("csv", "path/to/data.csv");
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load`

#### Signature

```cpp
static Registry load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load(std::string_view source_type, std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load("csv", "path/to/data.csv");
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_csv`

#### Signature

```cpp
static Registry load_csv(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static Registry load_csv(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_csv("path/to/data.csv");
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_csv`

#### Signature

```cpp
static Registry load_csv(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_csv(std::string_view filepath)`
- `static Registry load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_csv`

#### Signature

```cpp
static Registry load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::ReadCsvOptions&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_csv(std::string_view filepath)`
- `static Registry load_csv(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
pluss::loader::ReadCsvOptions options;
auto out = pluss::bundle::Registry::load_csv("path/to/data.csv", options);
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_excel`

#### Signature

```cpp
static Registry load_excel(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`
- `static Registry load_excel(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_excel("path/to/data.xlsx");
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_excel`

#### Signature

```cpp
static Registry load_excel(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_excel(std::string_view filepath)`
- `static Registry load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_excel("path/to/data.xlsx", {"nrows", 16});
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_excel`

#### Signature

```cpp
static Registry load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::ReadExcelOptions&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_excel(std::string_view filepath)`
- `static Registry load_excel(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
pluss::loader::ReadExcelOptions options;
auto out = pluss::bundle::Registry::load_excel("path/to/data.xlsx", options);
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_json`

#### Signature

```cpp
static Registry load_json(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_json(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_json("path/to/data.json");
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_json`

#### Signature

```cpp
static Registry load_json(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_json(std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_json("path/to/data.json", {"nrows", 16});
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_parquet`

#### Signature

```cpp
static Registry load_parquet(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_parquet(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_parquet("path/to/data.parquet");
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_parquet`

#### Signature

```cpp
static Registry load_parquet(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_parquet(std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_parquet("path/to/data.parquet", {"nrows", 16});
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_txt`

#### Signature

```cpp
static Registry load_txt(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_txt(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_txt("path/to/data.txt");
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `load_txt`

#### Signature

```cpp
static Registry load_txt(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_txt(std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_txt("path/to/data.txt", {"nrows", 16});
```

#### Output

```text
out.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `map`

#### Signature

```cpp
Registry map(self, const std::function<Slot(const Slot&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Transforms every slot with the same function and returns the updated registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.map([](const pluss::bundle::Slot& slot) { return slot; });
```

#### Output

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
Registry& map_(self, const std::function<Slot(const Slot&)>& fn)
```

#### Parameters

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Transforms every slot in place with the same function.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.map_([](const pluss::bundle::Slot& slot) { return slot; });
```

### `nfields`

#### Signature

```cpp
std::int64_t nfields(self) const
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.nfields();
```

#### Output

```text
2
```

### `nslots`

#### Signature

```cpp
std::int64_t nslots(self) const
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.nslots();
```

#### Output

```text
2
```

### `push`

#### Signature

```cpp
Registry push(self, std::string_view key, const Slot::Value& value) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Slot::Value&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Appends one new value to a named slot and returns the updated registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.push("ids", pluss::make_series(4));
```

#### Output

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
Registry& push_(self, std::string_view key, const Slot::Value& value)
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Slot::Value&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Appends one new value to a named slot in place.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.push_("ids", pluss::make_series(4));
```

### `push_all`

#### Signature

```cpp
Registry push_all(self, const Slot::Value& value) const
```

#### Parameters

`value` : `const Slot::Value&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Appends the same new value to every slot and returns the updated registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.push_all(pluss::make_note("ok"));
```

#### Output

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
Registry& push_all_(self, const Slot::Value& value)
```

#### Parameters

`value` : `const Slot::Value&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Appends the same new value to every slot in place.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.push_all_(pluss::make_note("ok"));
```

### `rename`

#### Signature

```cpp
Registry rename(self, const std::unordered_map<std::string, std::string>& mapping) const
```

#### Parameters

`mapping` : `const std::unordered_map<std::string, std::string>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Renames one or more slots without changing their stored values.

#### See also

- `Registry rename(self, std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const`
- `Registry rename(self, std::string_view from, std::string_view to) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::unordered_map<std::string, std::string> renames{{"ids", "ids_new"}};
auto out = rg.rename(renames);
```

#### Output

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
Registry rename(self, std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const
```

#### Parameters

`mapping` : `std::initializer_list<std::pair<std::string_view, std::string_view>>`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Renames one or more slots without changing their stored values.

#### See also

- `Registry rename(self, const std::unordered_map<std::string, std::string>& mapping) const`
- `Registry rename(self, std::string_view from, std::string_view to) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.rename({{"ids", "ids_new"}});
```

#### Output

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
Registry rename(self, std::string_view from, std::string_view to) const
```

#### Parameters

`from` : `std::string_view`
    See the signature for the supported argument form.

`to` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Renames one or more slots without changing their stored values.

#### See also

- `Registry rename(self, std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const`
- `Registry rename(self, const std::unordered_map<std::string, std::string>& mapping) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.rename("ids", "ids_new");
```

#### Output

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
Registry& rename_(self, std::string_view from, std::string_view to)
```

#### Parameters

`from` : `std::string_view`
    See the signature for the supported argument form.

`to` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Renames one or more slots in place without changing their stored values.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.rename_("ids", "ids_new");
```

### `sample`

#### Signature

```cpp
Registry sample(self, std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`random_state` : `std::optional<std::uint64_t>`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Returns a random sample of positions across every slot.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.sample(2, 7);
```

#### Output

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
void save_as(self, std::string_view target_type, std::string_view filepath, const pluss::loader::Config& config =
```

#### Parameters

`target_type` : `std::string_view`
    See the signature for the supported argument form.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the registry using the explicitly supplied output format selector.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_as("json", "path/to/output.json");
```

#### Output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
void save_csv(self, std::string_view filepath) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the registry to CSV after materializing each slot into a tabular export form.

#### See also

- `void save_csv(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_csv("path/to/output.csv");
```

#### Output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
void save_csv(self, std::string_view filepath, const pluss::loader::Config& config) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the registry to CSV after materializing each slot into a tabular export form.

#### See also

- `void save_csv(self, std::string_view filepath) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_csv("path/to/output.csv");
```

#### Output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
void save_excel(self, std::string_view filepath) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the registry to an Excel workbook after materializing each slot into a tabular export form.

#### See also

- `void save_excel(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_excel("path/to/output.xlsx");
```

#### Output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
void save_excel(self, std::string_view filepath, const pluss::loader::Config& config) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the registry to an Excel workbook after materializing each slot into a tabular export form.

#### See also

- `void save_excel(self, std::string_view filepath) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_excel("path/to/output.xlsx");
```

#### Output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
void save_json(self, std::string_view filepath) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the registry to JSON for interchange or later reload.

#### See also

- `void save_json(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_json("path/to/output.json");
```

#### Output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
void save_json(self, std::string_view filepath, const pluss::loader::Config& config) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the registry to JSON for interchange or later reload.

#### See also

- `void save_json(self, std::string_view filepath) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_json("path/to/output.json");
```

#### Output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
void save_parquet(self, std::string_view filepath) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the registry to Parquet for compact typed storage.

#### See also

- `void save_parquet(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_parquet("path/to/output.parquet");
```

#### Output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
void save_parquet(self, std::string_view filepath, const pluss::loader::Config& config) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the registry to Parquet for compact typed storage.

#### See also

- `void save_parquet(self, std::string_view filepath) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_parquet("path/to/output.parquet");
```

#### Output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
void save_txt(self, std::string_view filepath) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes a text rendering of the registry for inspection or logging.

#### See also

- `void save_txt(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_txt("path/to/output.txt");
```

#### Output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
void save_txt(self, std::string_view filepath, const pluss::loader::Config& config) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes a text rendering of the registry for inspection or logging.

#### See also

- `void save_txt(self, std::string_view filepath) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_txt("path/to/output.txt");
```

#### Output

```text
saved: out.*
```

### `select`

#### Signature

```cpp
Registry select(self, const std::vector<std::string>& keys) const
```

#### Parameters

`keys` : `const std::vector<std::string>&`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Keeps only the named slots and preserves their shared positional alignment.

#### See also

- `Registry select(self, std::initializer_list<std::string_view> keys) const`
- `Registry select(self, std::string_view key) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string> names{"ids"};
auto out = rg.select(names);
```

#### Output

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
Registry select(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Keeps only the named slots and preserves their shared positional alignment.

#### See also

- `Registry select(self, const std::vector<std::string>& keys) const`
- `Registry select(self, std::string_view key) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.select({"ids"});
```

#### Output

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
Registry select(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Keeps only the named slots and preserves their shared positional alignment.

#### See also

- `Registry select(self, std::initializer_list<std::string_view> keys) const`
- `Registry select(self, const std::vector<std::string>& keys) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.select("ids");
```

#### Output

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
Registry set_slot(self, std::string_view key, const Slot& value) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Slot&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Adds or replaces one named slot and returns the updated registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.set_slot("score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)}));
```

#### Output

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
Registry& set_slot_(self, std::string_view key, const Slot& value)
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Slot&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Adds or replaces one named slot in place.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.set_slot_("score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)}));
```

### `set_slots`

#### Signature

```cpp
Registry set_slots(self, std::initializer_list<std::pair<std::string, Slot>> items) const
```

#### Parameters

`items` : `std::initializer_list<std::pair<std::string, Slot>>`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Adds or replaces several named slots in one call and returns the updated registry.

#### See also

- `Registry set_slots(self, std::initializer_list<std::pair<std::string_view, Slot>> items) const`

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

#### Output

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
Registry set_slots(self, std::initializer_list<std::pair<std::string_view, Slot>> items) const
```

#### Parameters

`items` : `std::initializer_list<std::pair<std::string_view, Slot>>`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Adds or replaces several named slots in one call and returns the updated registry.

#### See also

- `Registry set_slots(self, std::initializer_list<std::pair<std::string, Slot>> items) const`

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

#### Output

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
Registry& set_slots_(self, std::initializer_list<std::pair<std::string, Slot>> items)
```

#### Parameters

`items` : `std::initializer_list<std::pair<std::string, Slot>>`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Adds or replaces several named slots in place.

#### See also

- `Registry& set_slots_(self, std::initializer_list<std::pair<std::string_view, Slot>> items)`

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

### `set_slots_`

#### Signature

```cpp
Registry& set_slots_(self, std::initializer_list<std::pair<std::string_view, Slot>> items)
```

#### Parameters

`items` : `std::initializer_list<std::pair<std::string_view, Slot>>`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Adds or replaces several named slots in place.

#### See also

- `Registry& set_slots_(self, std::initializer_list<std::pair<std::string, Slot>> items)`

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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.show(std::cout);
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `slice`

#### Signature

```cpp
Registry slice(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Returns a contiguous positional slice `[start, stop)` across every slot.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.slice(0, 2);
```

#### Output

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
Slot slot(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.slot("ids");
```

#### Output

```text
Slot([Series(1), Series(2), Series(3)])
```

### `slot_sizes`

#### Signature

```cpp
std::unordered_map<std::string, std::int64_t> slot_sizes(self) const
```

#### Parameters

`None`

#### Returns

`std::unordered_map<std::string, std::int64_t>`
    Value produced by this call.

#### Notes

Returns the current length of each slot.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.slot_sizes();
```

#### Output

```text
{"ids": 3, "city": 3}
```

### `slots`

#### Signature

```cpp
std::vector<Slot> slots(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<Slot>`
    Materialized collection of slot objects.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.slots();
```

#### Output

```text
[Slot(ids), Slot(city)]
```

### `tail`

#### Signature

```cpp
Registry tail(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Returns the last `n` values from every slot in the registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.tail(2);
```

#### Output

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
Registry take(self, const std::vector<std::int64_t>& indices) const
```

#### Parameters

`indices` : `const std::vector<std::int64_t>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Returns the values at the requested integer positions across every slot.

#### See also

- `Registry take(self, std::span<const std::int64_t> indices) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = rg.take(picks);
```

#### Output

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
Registry take(self, std::span<const std::int64_t> indices) const
```

#### Parameters

`indices` : `std::span<const std::int64_t>`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Returns the values at the requested integer positions across every slot.

#### See also

- `Registry take(self, const std::vector<std::int64_t>& indices) const`

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = rg.take(picks);
```

#### Output

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
Registry take_values(self, std::string_view key, std::span<const std::int64_t> indices) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`indices` : `std::span<const std::int64_t>`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Selects values from one named slot by integer position and returns the updated registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = rg.take_values("ids", picks);
```

#### Output

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
Registry& take_values_(self, std::string_view key, std::span<const std::int64_t> indices)
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`indices` : `std::span<const std::int64_t>`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Selects values from one named slot by integer position in place.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
rg.take_values_("ids", picks);
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.to_json();
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `to_kindle`

#### Signature

```cpp
pluss::kindle::Catalog to_kindle(self) const
```

#### Parameters

`None`

#### Returns

`pluss::kindle::Catalog`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.to_kindle();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.to_string();
```

#### Output

```text
rg.head():
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `transform_values`

#### Signature

```cpp
Registry transform_values(self, std::string_view key, const std::function<Slot::Value(const Slot::Value&)>& fn) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`fn` : `const std::function<Slot::Value(const Slot::Value&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Rewrites the values inside one named slot with a user function and returns the updated registry.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.transform_values("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
```

#### Output

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
Registry& transform_values_(self, std::string_view key, const std::function<Slot::Value(const Slot::Value&)>& fn)
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`fn` : `const std::function<Slot::Value(const Slot::Value&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

Rewrites the values inside one named slot in place.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.transform_values_("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
```

### `type_profiles`

#### Signature

```cpp
std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>> type_profiles(self) const
```

#### Parameters

`None`

#### Returns

`std::unordered_map<std::string, std::unordered_map<std::string, std::int64_t>>`
    Value produced by this call.

#### Notes

Summarizes the value types stored inside each slot.

#### Examples

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.type_profiles();
```

#### Output

```text
{"ids": {"Series": 3}, "city": {"Note": 3}}
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
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = rg.unique();
```

#### Output

```text
true
```
