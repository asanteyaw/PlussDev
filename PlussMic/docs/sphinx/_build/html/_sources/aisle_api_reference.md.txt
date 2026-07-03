# Aisle API Reference

# Practical Aisle API Reference

## API Reference Style

This page documents the full public `Aisle` surface for depth-stacked registries.

### `apply_slot`

#### Signature

```cpp
Aisle::apply_slot(key: std::string_view, fn: const std::function<Slot(const Slot&)>&) -> Aisle
```

#### Notes

Applies a slot transform to one named key inside every registry and returns the updated aisle.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`fn` : `const std::function<Slot(const Slot&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Aisle`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

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
Aisle::as_kindle() -> pluss::kindle::Archive
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`None`

#### Output

`pluss::kindle::Archive`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

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
Aisle::concat_depth(other: const Aisle&) -> Aisle
```

#### Notes

Appends the registries from another aisle onto the current depth axis.

#### Inputs

`other` : `const Aisle&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`Aisle`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

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
Aisle::defined() -> bool
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.defined();
```

#### Expected output

```text
true
```

### `depth`

#### Signature

```cpp
Aisle::depth() -> std::int64_t
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.depth();
```

#### Expected output

```text
2
```

### `empty`

#### Signature

```cpp
Aisle::empty() -> bool
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.empty();
```

#### Expected output

```text
false
```

### `from_kindle`

#### Signature

```cpp
Aisle::from_kindle(archive: const pluss::kindle::Archive&) -> Aisle
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`archive` : `const pluss::kindle::Archive&`
    Archive payload supplied to this overload.

#### Output

`Aisle`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `head`

#### Signature

```cpp
Aisle::head(n: std::int64_t) -> Aisle
```

#### Notes

Returns the first `n` registries from the aisle depth axis.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Aisle`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `kindelize`

#### Signature

```cpp
Aisle::kindelize() -> pluss::kindle::Archive
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`None`

#### Output

`pluss::kindle::Archive`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `labels`

#### Signature

```cpp
Aisle::labels() -> const std::optional<Slot>&
```

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

#### Inputs

`None`

#### Output

`const std::optional<Slot>&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

```text
["train", "test"]
```

### `map`

#### Signature

```cpp
Aisle::map(fn: const std::function<Registry(const Registry&)>&) -> Aisle
```

#### Notes

Applies the same function to each registry and returns the transformed aisle.

#### Inputs

`fn` : `const std::function<Registry(const Registry&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Aisle`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `registries`

#### Signature

```cpp
Aisle::registries() -> const std::vector<Registry>&
```

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Inputs

`None`

#### Output

`const std::vector<Registry>&`
    Borrowed collection of registries.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

```text
[Registry(depth0), Registry(depth1)]
```

### `registry`

#### Signature

```cpp
Aisle::registry(i: std::int64_t) -> Registry
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`i` : `std::int64_t`
    Zero-based positional index used by this overload.

#### Output

`Registry`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

```text
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```

### `select`

#### Signature

```cpp
Aisle::select(keys: std::initializer_list<std::string_view>) -> Aisle
```

#### Notes

Keeps only the named keys inside every registry in the aisle.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Aisle`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `show`

#### Signature

```cpp
Aisle::show(os: std::ostream&) -> void
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

aisle.show(std::cout);
```

#### Expected output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `slice`

#### Signature

```cpp
Aisle::slice(start: std::int64_t, stop: std::int64_t) -> Aisle
```

#### Notes

Returns a contiguous depth slice `[start, stop)` from the aisle.

#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Output

`Aisle`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `tail`

#### Signature

```cpp
Aisle::tail(n: std::int64_t) -> Aisle
```

#### Notes

Returns the last `n` registries from the aisle depth axis.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Aisle`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `to_json`

#### Signature

```cpp
Aisle::to_json() -> std::string
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.to_json();
```

#### Expected output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `to_kindle`

#### Signature

```cpp
Aisle::to_kindle() -> pluss::kindle::Archive
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`None`

#### Output

`pluss::kindle::Archive`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

#### Expected output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `to_string`

#### Signature

```cpp
Aisle::to_string() -> std::string
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.to_string();
```

#### Expected output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```

### `unique`

#### Signature

```cpp
Aisle::unique() -> bool
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto aisle = pluss::bundle::Aisle({r0, r1});

auto out = aisle.unique();
```

#### Expected output

```text
true
```
