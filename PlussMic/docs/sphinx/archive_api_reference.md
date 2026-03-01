# Archive API Reference

# Practical Archive API Reference

## API Reference Style

This page documents the full public `Archive` surface for depth-stacked catalogs.

### `aggregate_over_catalogs`

#### Signature

```cpp
Catalog aggregate_over_catalogs(self, const std::function<Catalog(const std::vector<Catalog>&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Catalog(const std::vector<Catalog>&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

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

auto out = archive.aggregate_over_catalogs([](const std::vector<pluss::kindle::Catalog>& catalogs) { return catalogs.front(); });
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `align_depth`

#### Signature

```cpp
Archive align_depth(self, const Stream& labels) const
```

#### Parameters

`labels` : `const Stream&`
    See the signature for the supported argument form.

#### Returns

`Archive`
    Value produced by this call.

#### Notes

This call is part of the public `Archive` API and is documented here using a concrete usage example.

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

auto labels = pluss::kindle::Stream(pluss::make_note({"baseline", "stress"}));
auto out = archive.align_depth(labels);
```

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `catalog`

#### Signature

```cpp
Catalog catalog(self, std::int64_t i) const
```

#### Parameters

`i` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This call is part of the public `Archive` API and is documented here using a concrete usage example.

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

auto out = archive.catalog(0);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `catalogs`

#### Signature

```cpp
const std::vector<Catalog>& catalogs(self) const
```

#### Parameters

`None`

#### Returns

`const std::vector<Catalog>&`
    Borrowed collection of catalogs.

#### Notes

This call is part of the public `Archive` API and is documented here using a concrete usage example.

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

auto out = archive.catalogs();
```

#### Output

```text
[Catalog(train), Catalog(test)]
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

This call is part of the public `Archive` API and is documented here using a concrete usage example.

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

auto out = archive.defined();
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

This call is part of the public `Archive` API and is documented here using a concrete usage example.

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

auto out = archive.depth();
```

#### Output

```text
2
```

### `depth_labels`

#### Signature

```cpp
const std::optional<Stream>& depth_labels(self) const
```

#### Parameters

`None`

#### Returns

`const std::optional<Stream>&`
    Value produced by this call.

#### Notes

This call is part of the public `Archive` API and is documented here using a concrete usage example.

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

auto out = archive.depth_labels();
```

#### Output

```text
["train", "test"]
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

This call is part of the public `Archive` API and is documented here using a concrete usage example.

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

auto out = archive.empty();
```

#### Output

```text
false
```

### `head`

#### Signature

```cpp
Archive head(self, std::int64_t n = 5) const
```

#### Parameters

`n` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Archive`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

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

auto out = archive.head(1);
```

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `iloc`

#### Signature

```cpp
Catalog iloc(self, std::int64_t i) const
```

#### Parameters

`i` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

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

auto out = archive.iloc(0);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `kindelize`

#### Signature

```cpp
static Archive kindelize(const pluss::table::Atlas& atlas, const KindelizePolicy& policy =
```

#### Parameters

`atlas` : `const pluss::table::Atlas&`
    See the signature for the supported argument form.

`policy` : `const KindelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`Archive`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = pluss::kindle::Archive::kindelize(atlas);
```

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `loc`

#### Signature

```cpp
Catalog loc(self, const pluss::Timeline& label) const
```

#### Parameters

`label` : `const pluss::Timeline&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### See also

- `Catalog loc(self, std::string_view label) const`
- `Catalog loc(self, std::int64_t label) const`

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

auto out = archive.loc("test");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `loc`

#### Signature

```cpp
Catalog loc(self, std::int64_t label) const
```

#### Parameters

`label` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### See also

- `Catalog loc(self, std::string_view label) const`
- `Catalog loc(self, const pluss::Timeline& label) const`

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

auto out = archive.loc("test");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `loc`

#### Signature

```cpp
Catalog loc(self, std::string_view label) const
```

#### Parameters

`label` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### See also

- `Catalog loc(self, std::int64_t label) const`
- `Catalog loc(self, const pluss::Timeline& label) const`

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

auto out = archive.loc("test");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `map_catalogs`

#### Signature

```cpp
Archive map_catalogs(self, const std::function<Catalog(const Catalog&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Catalog(const Catalog&)>&`
    See the signature for the supported argument form.

#### Returns

`Archive`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

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

auto out = archive.map_catalogs([](const pluss::kindle::Catalog& catalog) { return catalog.head(1); });
```

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `relabel_depth`

#### Signature

```cpp
Archive relabel_depth(self, const Stream& labels) const
```

#### Parameters

`labels` : `const Stream&`
    See the signature for the supported argument form.

#### Returns

`Archive`
    Value produced by this call.

#### Notes

This adjusts labels without changing the underlying payload values.

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

auto labels = pluss::kindle::Stream(pluss::make_note({"baseline", "stress"}));
auto out = archive.relabel_depth(labels);
```

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
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
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

archive.show(std::cout);
```

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `slice_depth`

#### Signature

```cpp
Archive slice_depth(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    See the signature for the supported argument form.

`stop` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Archive`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

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

auto out = archive.slice_depth(0, 1);
```

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `sort_depth`

#### Signature

```cpp
Archive sort_depth(self, bool ascending = true, bool nulls_last = true) const
```

#### Parameters

`ascending` : `bool`
    See the signature for the supported argument form.

`nulls_last` : `bool`
    See the signature for the supported argument form.

#### Returns

`Archive`
    Value produced by this call.

#### Notes

This call is part of the public `Archive` API and is documented here using a concrete usage example.

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

auto out = archive.sort_depth(true, true);
```

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `tabelize`

#### Signature

```cpp
pluss::table::Atlas tabelize(self, const TabelizePolicy& policy =
```

#### Parameters

`policy` : `const TabelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`pluss::table::Atlas`
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

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Inner;
auto out = archive.tabelize(policy);
```

#### Output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `tail`

#### Signature

```cpp
Archive tail(self, std::int64_t n = 5) const
```

#### Parameters

`n` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Archive`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

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

auto out = archive.tail(1);
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

This call is part of the public `Archive` API and is documented here using a concrete usage example.

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

auto out = archive.to_string();
```

#### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```

### `to_table`

#### Signature

```cpp
pluss::table::Atlas to_table(self, const TabelizePolicy& policy =
```

#### Parameters

`policy` : `const TabelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`pluss::table::Atlas`
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

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Inner;
auto out = archive.to_table(policy, true);
```

#### Output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
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

This call is part of the public `Archive` API and is documented here using a concrete usage example.

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

auto out = archive.unique();
```

#### Output

```text
true
```
