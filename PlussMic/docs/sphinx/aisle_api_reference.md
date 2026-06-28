# Aisle API Reference

# Practical Aisle API Reference

## API Reference Style

This page documents the full public `Aisle` surface for depth-stacked registries.

### `apply_slot`

#### Signature

```cpp
Aisle apply_slot(self, std::string_view key, const std::function<Slot(const Slot&)>& fn) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`fn` : `const std::function<Slot(const Slot&)>&`
    See the signature for the supported argument form.

#### Returns

`Aisle`
    Value produced by this call.

#### Notes

Applies a slot transform to one named key inside every registry and returns the updated aisle.

#### Examples

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

#### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

#### Practical Behavior

- `apply_slot(key, fn)` walks every registry in the aisle and rewrites that slot only where it exists.
- Registries that do not have the requested key are left unchanged.


### `as_kindle`

#### Signature

```cpp
pluss::kindle::Archive as_kindle(self) const
```

#### Parameters

`None`

#### Returns

`pluss::kindle::Archive`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

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

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

#### Bridge Behavior

- `as_kindle()` converts the aisle into the Archive model.
- Use this when the next workflow is archive/catalog based rather than bundle based.


### `concat_depth`

#### Signature

```cpp
Aisle concat_depth(self, const Aisle& other) const
```

#### Parameters

`other` : `const Aisle&`
    See the signature for the supported argument form.

#### Returns

`Aisle`
    Value produced by this call.

#### Notes

Appends the registries from another aisle onto the current depth axis.

#### Examples

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

#### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
depth2 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth3 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

#### Practical Behavior

- `concat_depth(other)` appends the other aisle’s registries onto the current depth axis.
- If both aisles have labels, the labels are concatenated too.


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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.defined();
```

#### Output

```text
true
```

### `depth`

#### Signature

```cpp
std::int64_t depth(self) const
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.depth();
```

#### Output

```text
2
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.empty();
```

#### Output

```text
false
```

### `from_kindle`

#### Signature

```cpp
static Aisle from_kindle(const pluss::kindle::Archive& archive)
```

#### Parameters

`archive` : `const pluss::kindle::Archive&`
    See the signature for the supported argument form.

#### Returns

`Aisle`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

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

#### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `head`

#### Signature

```cpp
Aisle head(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Aisle`
    Value produced by this call.

#### Notes

Returns the first `n` registries from the aisle depth axis.

#### Examples

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

#### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `kindelize`

#### Signature

```cpp
pluss::kindle::Archive kindelize(self) const
```

#### Parameters

`None`

#### Returns

`pluss::kindle::Archive`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

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

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `labels`

#### Signature

```cpp
const std::optional<Slot>& labels(self) const
```

#### Parameters

`None`

#### Returns

`const std::optional<Slot>&`
    Value produced by this call.

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

#### Examples

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

#### Output

```text
["train", "test"]
```

### `map`

#### Signature

```cpp
Aisle map(self, const std::function<Registry(const Registry&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Registry(const Registry&)>&`
    See the signature for the supported argument form.

#### Returns

`Aisle`
    Value produced by this call.

#### Notes

Applies the same function to each registry and returns the transformed aisle.

#### Examples

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

#### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `registries`

#### Signature

```cpp
const std::vector<Registry>& registries(self) const
```

#### Parameters

`None`

#### Returns

`const std::vector<Registry>&`
    Borrowed collection of registries.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Examples

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

#### Output

```text
[Registry(depth0), Registry(depth1)]
```

### `registry`

#### Signature

```cpp
Registry registry(self, std::int64_t i) const
```

#### Parameters

`i` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Registry`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

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

#### Output

```text
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `select`

#### Signature

```cpp
Aisle select(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Aisle`
    Value produced by this call.

#### Notes

Keeps only the named keys inside every registry in the aisle.

#### Examples

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

#### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

aisle.show(std::cout);
```

#### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `slice`

#### Signature

```cpp
Aisle slice(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Returns

`Aisle`
    Value produced by this call.

#### Notes

Returns a contiguous depth slice `[start, stop)` from the aisle.

#### Examples

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

#### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `tail`

#### Signature

```cpp
Aisle tail(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Aisle`
    Value produced by this call.

#### Notes

Returns the last `n` registries from the aisle depth axis.

#### Examples

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

#### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.to_json();
```

#### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `to_kindle`

#### Signature

```cpp
pluss::kindle::Archive to_kindle(self) const
```

#### Parameters

`None`

#### Returns

`pluss::kindle::Archive`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

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

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.to_string();
```

#### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.unique();
```

#### Output

```text
true
```
