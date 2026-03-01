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
    See the signature for the supported argument form.

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Registry apply(self, std::string_view key, const std::function<Slot(const Slot&)>& fn) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.apply({"ids"}, [](const pluss::bundle::Slot& slot) { return slot; });
```

#### Output

```text
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
    See the signature for the supported argument form.

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Registry apply(self, std::initializer_list<std::string_view> keys, const std::function<Slot(const Slot&)>& fn) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.apply("ids", [](const pluss::bundle::Slot& slot) { return slot; });
```

#### Output

```text
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
    See the signature for the supported argument form.

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Registry& apply_(self, std::string_view key, const std::function<Slot(const Slot&)>& fn)`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.apply_({"ids"}, [](const pluss::bundle::Slot& slot) { return slot; });
```

### `apply_`

#### Signature

```cpp
Registry& apply_(self, std::string_view key, const std::function<Slot(const Slot&)>& fn)
```

#### Parameters

`key` : `std::string_view`
    See the signature for the supported argument form.

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Registry& apply_(self, std::initializer_list<std::string_view> keys, const std::function<Slot(const Slot&)>& fn)`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.apply_("ids", [](const pluss::bundle::Slot& slot) { return slot; });
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

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.apply_all([](const pluss::bundle::Slot& slot) { return slot; });
```

#### Output

```text
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
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.as_kindle();
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
    See the signature for the supported argument form.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.at("ids");
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

for (const auto& key : registry) {
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.clear();
```

#### Output

```text
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.clear_();
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

Use this when you need to merge compatible objects or extend the current one with another input.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto other = pluss::bundle::Registry({{"flag", pluss::bundle::Slot({pluss::make_series(true, pluss::codem::ScalarType::Bool), pluss::make_series(false, pluss::codem::ScalarType::Bool), pluss::make_series(true, pluss::codem::ScalarType::Bool)})}});
auto out = registry.combine(other, "_rhs");
```

#### Output

```text
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
    See the signature for the supported argument form.

`other` : `const Slot&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this when you need to merge compatible objects or extend the current one with another input.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
auto out = registry.concat_values("ids", extra);
```

#### Output

```text
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
    See the signature for the supported argument form.

`other` : `const Slot&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto extra = pluss::bundle::Slot({pluss::make_series(4), pluss::make_series(5)});
registry.concat_values_("ids", extra);
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.defined();
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
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### See also

- `Registry drop(self, std::initializer_list<std::string_view> keys) const`
- `Registry drop(self, std::string_view key) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string> names{"city"};
auto out = registry.drop(names);
```

#### Output

```text
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
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### See also

- `Registry drop(self, const std::vector<std::string>& keys) const`
- `Registry drop(self, std::string_view key) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.drop({"city"});
```

#### Output

```text
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
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### See also

- `Registry drop(self, std::initializer_list<std::string_view> keys) const`
- `Registry drop(self, const std::vector<std::string>& keys) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.drop("city");
```

#### Output

```text
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.drop_empty_slots();
```

#### Output

```text
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.drop_empty_slots_();
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.empty();
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

for (const auto& key : registry) {
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.ensure_stream_compatible("demo");
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.field_names();
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
    See the signature for the supported argument form.

`predicate` : `const std::function<bool(const Slot::Value&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.filter_values("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
```

#### Output

```text
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
    See the signature for the supported argument form.

`predicate` : `const std::function<bool(const Slot::Value&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.filter_values_("ids", [](const pluss::bundle::Slot::Value& value) { return std::holds_alternative<pluss::Series>(value); });
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

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = pluss::bundle::Registry::from_kindle(catalog);
```

#### Output

```text
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
    See the signature for the supported argument form.

#### Returns

`std::optional<Slot>`
    Value produced by this call.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.get("ids");
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
    See the signature for the supported argument form.

#### Returns

`std::optional<Slot>`
    Value produced by this call.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.get_slot("ids");
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
    See the signature for the supported argument form.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.has("ids");
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
    See the signature for the supported argument form.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.has_all({"ids", "city"});
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
    See the signature for the supported argument form.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.has_any({"ids", "city"});
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
    See the signature for the supported argument form.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.has_slot("ids");
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
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.head(2);
```

#### Output

```text
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

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### See also

- `Registry iloc(self, std::int64_t start, std::int64_t stop) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.iloc(1);
```

#### Output

```text
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
    See the signature for the supported argument form.

`stop` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### See also

- `Registry iloc(self, std::int64_t row) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.iloc(0, 2);
```

#### Output

```text
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.is_stream_compatible();
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.keys();
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
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.kindelize();
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
    See the signature for the supported argument form.

`filepath` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load("csv", "PlussMic/data/messy_data.csv");
```

#### Output

```text
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
    See the signature for the supported argument form.

`filepath` : `std::string_view`
    See the signature for the supported argument form.

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
auto out = pluss::bundle::Registry::load("csv", "PlussMic/data/messy_data.csv");
```

#### Output

```text
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
    See the signature for the supported argument form.

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
auto out = pluss::bundle::Registry::load_csv("PlussMic/data/messy_data.csv");
```

#### Output

```text
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
    See the signature for the supported argument form.

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
auto out = pluss::bundle::Registry::load_csv("PlussMic/data/messy_data.csv", {"nrows", 16});
```

#### Output

```text
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
    See the signature for the supported argument form.

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
auto out = pluss::bundle::Registry::load_csv("PlussMic/data/messy_data.csv", options);
```

#### Output

```text
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
    See the signature for the supported argument form.

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
auto out = pluss::bundle::Registry::load_excel("PlussMic/data/messy_data.csv");
```

#### Output

```text
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
    See the signature for the supported argument form.

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
auto out = pluss::bundle::Registry::load_excel("PlussMic/data/messy_data.csv", {"nrows", 16});
```

#### Output

```text
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
    See the signature for the supported argument form.

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
auto out = pluss::bundle::Registry::load_excel("PlussMic/data/report.xlsx", options);
```

#### Output

```text
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
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_json(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_json("PlussMic/data/messy_data.csv");
```

#### Output

```text
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
    See the signature for the supported argument form.

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
auto out = pluss::bundle::Registry::load_json("PlussMic/data/messy_data.csv", {"nrows", 16});
```

#### Output

```text
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
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_parquet(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_parquet("PlussMic/data/messy_data.csv");
```

#### Output

```text
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
    See the signature for the supported argument form.

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
auto out = pluss::bundle::Registry::load_parquet("PlussMic/data/messy_data.csv", {"nrows", 16});
```

#### Output

```text
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
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Registry load_txt(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::bundle::Registry::load_txt("PlussMic/data/messy_data.csv");
```

#### Output

```text
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
    See the signature for the supported argument form.

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
auto out = pluss::bundle::Registry::load_txt("PlussMic/data/messy_data.csv", {"nrows", 16});
```

#### Output

```text
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

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.map([](const pluss::bundle::Slot& slot) { return slot; });
```

#### Output

```text
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

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.map_([](const pluss::bundle::Slot& slot) { return slot; });
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.nfields();
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.nslots();
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
    See the signature for the supported argument form.

`value` : `const Slot::Value&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.push("ids", pluss::make_series(4));
```

#### Output

```text
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
    See the signature for the supported argument form.

`value` : `const Slot::Value&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.push_("ids", pluss::make_series(4));
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

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.push_all(pluss::make_note("ok"));
```

#### Output

```text
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

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.push_all_(pluss::make_note("ok"));
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

This adjusts labels without changing the underlying payload values.

#### See also

- `Registry rename(self, std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const`
- `Registry rename(self, std::string_view from, std::string_view to) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::unordered_map<std::string, std::string> renames{{"ids", "ids_new"}};
auto out = registry.rename(renames);
```

#### Output

```text
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

This adjusts labels without changing the underlying payload values.

#### See also

- `Registry rename(self, const std::unordered_map<std::string, std::string>& mapping) const`
- `Registry rename(self, std::string_view from, std::string_view to) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.rename({{"ids", "ids_new"}});
```

#### Output

```text
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

This adjusts labels without changing the underlying payload values.

#### See also

- `Registry rename(self, std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const`
- `Registry rename(self, const std::unordered_map<std::string, std::string>& mapping) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.rename("ids", "ids_new");
```

#### Output

```text
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

This adjusts labels without changing the underlying payload values.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.rename_("ids", "ids_new");
```

### `sample`

#### Signature

```cpp
Registry sample(self, std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) const
```

#### Parameters

`n` : `std::int64_t`
    See the signature for the supported argument form.

`random_state` : `std::optional<std::uint64_t>`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.sample(2, 7);
```

#### Output

```text
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
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.save_as("json", "out.json");
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
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_csv(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.save_csv("out.csv");
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
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_csv(self, std::string_view filepath) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.save_csv("out.csv");
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
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_excel(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.save_excel("out.xlsx");
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
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_excel(self, std::string_view filepath) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.save_excel("out.xlsx");
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
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_json(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.save_json("out.json");
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
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_json(self, std::string_view filepath) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.save_json("out.json");
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
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_parquet(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.save_parquet("out.parquet");
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
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_parquet(self, std::string_view filepath) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.save_parquet("out.parquet");
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
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_txt(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.save_txt("out.txt");
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
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_txt(self, std::string_view filepath) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.save_txt("out.txt");
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
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### See also

- `Registry select(self, std::initializer_list<std::string_view> keys) const`
- `Registry select(self, std::string_view key) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::string> names{"ids"};
auto out = registry.select(names);
```

#### Output

```text
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
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### See also

- `Registry select(self, const std::vector<std::string>& keys) const`
- `Registry select(self, std::string_view key) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.select({"ids"});
```

#### Output

```text
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
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### See also

- `Registry select(self, std::initializer_list<std::string_view> keys) const`
- `Registry select(self, const std::vector<std::string>& keys) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.select("ids");
```

#### Output

```text
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
    See the signature for the supported argument form.

`value` : `const Slot&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.set_slot("score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)}));
```

#### Output

```text
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
    See the signature for the supported argument form.

`value` : `const Slot&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.set_slot_("score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)}));
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

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Registry set_slots(self, std::initializer_list<std::pair<std::string_view, Slot>> items) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.set_slots({
    {"score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)})},
});
```

#### Output

```text
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

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Registry set_slots(self, std::initializer_list<std::pair<std::string, Slot>> items) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.set_slots({
    {"score", pluss::bundle::Slot({pluss::make_series(0.9), pluss::make_series(0.7), pluss::make_series(0.8)})},
});
```

#### Output

```text
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

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Registry& set_slots_(self, std::initializer_list<std::pair<std::string_view, Slot>> items)`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.set_slots_({
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

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Registry& set_slots_(self, std::initializer_list<std::pair<std::string, Slot>> items)`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.set_slots_({
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
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.show(std::cout);
```

#### Output

```text
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
    See the signature for the supported argument form.

`stop` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.slice(0, 2);
```

#### Output

```text
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
    See the signature for the supported argument form.

#### Returns

`Slot`
    Value produced by this call.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.slot("ids");
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

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.slot_sizes();
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.slots();
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
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.tail(2);
```

#### Output

```text
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

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### See also

- `Registry take(self, std::span<const std::int64_t> indices) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = registry.take(picks);
```

#### Output

```text
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

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### See also

- `Registry take(self, const std::vector<std::int64_t>& indices) const`

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = registry.take(picks);
```

#### Output

```text
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
    See the signature for the supported argument form.

`indices` : `std::span<const std::int64_t>`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
auto out = registry.take_values("ids", picks);
```

#### Output

```text
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
    See the signature for the supported argument form.

`indices` : `std::span<const std::int64_t>`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::vector<std::int64_t> picks{0, 2};
registry.take_values_("ids", picks);
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.to_json();
```

#### Output

```text
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
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.to_kindle();
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.to_string();
```

#### Output

```text
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
    See the signature for the supported argument form.

`fn` : `const std::function<Slot::Value(const Slot::Value&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.transform_values("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
```

#### Output

```text
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
    See the signature for the supported argument form.

`fn` : `const std::function<Slot::Value(const Slot::Value&)>&`
    See the signature for the supported argument form.

#### Returns

`Registry&`
    Value produced by this call.

#### Notes

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

registry.transform_values_("ids", [](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value { if (std::holds_alternative<pluss::Series>(value)) return std::get<pluss::Series>(value).add(10); return value; });
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

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.type_profiles();
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

This call is part of the public `Registry` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto registry = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto out = registry.unique();
```

#### Output

```text
true
```
