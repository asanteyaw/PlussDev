# Catalog API Reference

# Practical Catalog API Reference

## API Reference Style

This page documents the full public `Catalog` surface for named stream collections.

### `add_text_features`

#### Signature

```cpp
Catalog add_text_features(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = catalog.add_text_features("text");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `aggregate_text_stats`

#### Signature

```cpp
Catalog aggregate_text_stats(self, std::string_view field, std::string_view stat_type) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

`stat_type` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = catalog.aggregate_text_stats("text", "mean");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `apply`

#### Signature

```cpp
Catalog apply(self, std::string_view field, const std::function<Stream(const Stream&)>& func) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

`func` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.apply("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `apply_`

#### Signature

```cpp
Catalog& apply_(self, std::string_view field, const std::function<Stream(const Stream&)>& func)
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

`func` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.apply_("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });
```

### `apply_all`

#### Signature

```cpp
Catalog apply_all(self, const std::function<Stream(const Stream&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.apply_all([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `apply_all_`

#### Signature

```cpp
Catalog& apply_all_(self, const std::function<Stream(const Stream&)>& fn)
```

#### Parameters

`fn` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.apply_all_([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });
```

### `astype`

#### Signature

```cpp
Catalog astype(self, std::initializer_list<std::string_view> keys, pluss::codem::ScalarType dtype) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`dtype` : `pluss::codem::ScalarType`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.astype({"metric"}, pluss::codem::ScalarType::Float64);
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
Stream at(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.at("metric");
```

#### Output

```text
Series([10.0, 20.0, 30.0])
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

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

for (const auto& key : catalog) {
  (void)key;
}
```

### `bfill`

#### Signature

```cpp
Catalog bfill(self, std::initializer_list<std::string_view> keys =
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.bfill();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `cast_note`

#### Signature

```cpp
Catalog cast_note(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.cast_note({"metric"});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `cast_series`

#### Signature

```cpp
Catalog cast_series(self, std::initializer_list<std::string_view> keys, std::optional<pluss::codem::ScalarType> dtype = std::nullopt) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`dtype` : `std::optional<pluss::codem::ScalarType>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"metric"_k = pluss::kindle::Stream(pluss::make_note({"10.0", "20.0", "30.0"}))});

auto out = catalog.cast_series({"metric"}, pluss::codem::ScalarType::Float64);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `cast_timeindex`

#### Signature

```cpp
Catalog cast_timeindex(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.cast_timeindex({"metric"});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `cast_timeline`

#### Signature

```cpp
Catalog cast_timeline(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.cast_timeline({"metric"});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `compute_similarity`

#### Signature

```cpp
Catalog compute_similarity(self, std::string_view field1, std::string_view field2, std::string_view method = "cosine") const
```

#### Parameters

`field1` : `std::string_view`
    See the signature for the supported argument form.

`field2` : `std::string_view`
    See the signature for the supported argument form.

`method` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = catalog.compute_similarity("text", "text", "cosine");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `count`

#### Signature

```cpp
std::int64_t count(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.count();
```

#### Output

```text
3
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

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.defined();
```

#### Output

```text
true
```

### `display`

#### Signature

```cpp
void display(self) const
```

#### Parameters

`None`

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.display();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `drop`

#### Signature

```cpp
Catalog drop(self, const std::vector<std::string>& field_names) const
```

#### Parameters

`field_names` : `const std::vector<std::string>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### See also

- `Catalog drop(self, std::string_view field_name) const`
- `Catalog drop(self, std::initializer_list<std::string_view> keys) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::vector<std::string> names{"city"};
auto out = catalog.drop(names);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `drop`

#### Signature

```cpp
Catalog drop(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### See also

- `Catalog drop(self, std::string_view field_name) const`
- `Catalog drop(self, const std::vector<std::string>& field_names) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.drop({"city"});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `drop`

#### Signature

```cpp
Catalog drop(self, std::string_view field_name) const
```

#### Parameters

`field_name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### See also

- `Catalog drop(self, const std::vector<std::string>& field_names) const`
- `Catalog drop(self, std::initializer_list<std::string_view> keys) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.drop("city");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `dropna`

#### Signature

```cpp
Catalog dropna(self, std::initializer_list<std::string_view> keys =
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.dropna();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
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

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.empty();
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

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

for (const auto& key : catalog) {
  (void)key;
}
```

### `extract_keywords`

#### Signature

```cpp
Catalog extract_keywords(self, std::string_view field, int top_n = 10) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

`top_n` : `int`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = catalog.extract_keywords("text", 5);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `ffill`

#### Signature

```cpp
Catalog ffill(self, std::initializer_list<std::string_view> keys =
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.ffill();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Catalog fillna(self, const char* value, std::initializer_list<std::string_view> keys =
```

#### Parameters

`value` : `const char*`
    See the signature for the supported argument form.

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Catalog fillna(self, scalar value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, std::string_view value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys =`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = catalog.fillna("Unknown", {"city"});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Catalog fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys =
```

#### Parameters

`value` : `const pluss::TimeIndex&`
    See the signature for the supported argument form.

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Catalog fillna(self, scalar value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, std::string_view value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const char* value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> keys =`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = catalog.fillna("Unknown", {"city"});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Catalog fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> keys =
```

#### Parameters

`value` : `const pluss::Timeline&`
    See the signature for the supported argument form.

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Catalog fillna(self, scalar value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, std::string_view value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const char* value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys =`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = catalog.fillna("Unknown", {"city"});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Catalog fillna(self, scalar value, std::initializer_list<std::string_view> keys =
```

#### Parameters

`value` : `scalar`
    See the signature for the supported argument form.

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Catalog fillna(self, std::string_view value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const char* value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys =`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = catalog.fillna("Unknown", {"city"});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Catalog fillna(self, std::string_view value, std::initializer_list<std::string_view> keys =
```

#### Parameters

`value` : `std::string_view`
    See the signature for the supported argument form.

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Catalog fillna(self, scalar value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const char* value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> keys =`
- `Catalog fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys =`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = catalog.fillna("Unknown", {"city"});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `filter`

#### Signature

```cpp
Catalog filter(self, const pluss::Series& condition) const
```

#### Parameters

`condition` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto out = catalog.filter(mask);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `filter_by_text`

#### Signature

```cpp
Catalog filter_by_text(self, std::string_view field, std::string_view pattern) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

`pattern` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = catalog.filter_by_text("text", "rain");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `from_json`

#### Signature

```cpp
static Catalog from_json(std::string_view json_text)
```

#### Parameters

`json_text` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
auto out = pluss::kindle::Catalog::from_json(R"([{"id":1,"city":"Accra"},{"id":2,"city":"Tema"}])");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `get`

#### Signature

```cpp
std::optional<Stream> get(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`std::optional<Stream>`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.get("metric");
```

#### Output

```text
Series([10.0, 20.0, 30.0])
```

### `get_stream`

#### Signature

```cpp
std::optional<Stream> get_stream(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`std::optional<Stream>`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.get_stream("metric");
```

#### Output

```text
Series([10.0, 20.0, 30.0])
```

### `group_by`

#### Signature

```cpp
std::unordered_map<std::string, Catalog> group_by(self, std::string_view key_field) const
```

#### Parameters

`key_field` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`std::unordered_map<std::string, Catalog>`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.group_by("city");
```

#### Output

```text
{"Accra": Catalog(...), "Tema": Catalog(...)}
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

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.has("metric");
```

#### Output

```text
true
```

### `has_stream`

#### Signature

```cpp
bool has_stream(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.has_stream("metric");
```

#### Output

```text
true
```

### `head`

#### Signature

```cpp
Catalog head(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.head(2);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `iloc`

#### Signature

```cpp
Catalog iloc(self, std::int64_t row) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### See also

- `Catalog iloc(self, std::int64_t start, std::int64_t stop) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.iloc(1);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `iloc`

#### Signature

```cpp
Catalog iloc(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    See the signature for the supported argument form.

`stop` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### See also

- `Catalog iloc(self, std::int64_t row) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.iloc(0, 2);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `info`

#### Signature

```cpp
void info(self) const
```

#### Parameters

`None`

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.info();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `isnull`

#### Signature

```cpp
Catalog isnull(self, std::initializer_list<std::string_view> keys =
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.isnull();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
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

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.keys();
```

#### Output

```text
["id", "metric", "city"]
```

### `kindelize`

#### Signature

```cpp
static Catalog kindelize(const pluss::table::DataFrame& frame, const KindelizePolicy& policy =
```

#### Parameters

`frame` : `const pluss::table::DataFrame&`
    See the signature for the supported argument form.

`policy` : `const KindelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = pluss::kindle::Catalog::kindelize(df);
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
static Catalog load(std::string_view source_type, std::string_view filepath)
```

#### Parameters

`source_type` : `std::string_view`
    See the signature for the supported argument form.

`filepath` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load("csv", "PlussMic/data/messy_data.csv");
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
static Catalog load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`source_type` : `std::string_view`
    See the signature for the supported argument form.

`filepath` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load(std::string_view source_type, std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load("csv", "PlussMic/data/messy_data.csv");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_csv`

#### Signature

```cpp
static Catalog load_csv(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_csv("PlussMic/data/messy_data.csv");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_csv`

#### Signature

```cpp
static Catalog load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)
```

#### Parameters

`filepath` : `std::string_view`
    See the signature for the supported argument form.

`options` : `const pluss::loader::ReadCsvOptions&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_csv(std::string_view filepath)`

#### Examples

```cpp
pluss::loader::ReadCsvOptions options;
auto out = pluss::kindle::Catalog::load_csv("PlussMic/data/messy_data.csv", options);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_csv_with_key`

#### Signature

```cpp
static Catalog load_csv_with_key(std::string_view filepath, std::string_view key_field, const pluss::loader::ReadCsvOptions& options =
```

#### Parameters

`filepath` : `std::string_view`
    See the signature for the supported argument form.

`key_field` : `std::string_view`
    See the signature for the supported argument form.

`options` : `const pluss::loader::ReadCsvOptions&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_csv_with_key("PlussMic/data/messy_data.csv", "id");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_excel`

#### Signature

```cpp
static Catalog load_excel(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_excel("PlussMic/data/messy_data.csv");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_excel`

#### Signature

```cpp
static Catalog load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)
```

#### Parameters

`filepath` : `std::string_view`
    See the signature for the supported argument form.

`options` : `const pluss::loader::ReadExcelOptions&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_excel(std::string_view filepath)`

#### Examples

```cpp
pluss::loader::ReadExcelOptions options;
auto out = pluss::kindle::Catalog::load_excel("PlussMic/data/report.xlsx", options);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_json`

#### Signature

```cpp
static Catalog load_json(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_json(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_json("PlussMic/data/messy_data.csv");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_json`

#### Signature

```cpp
static Catalog load_json(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_json(std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_json("PlussMic/data/messy_data.csv", {"nrows", 16});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_parquet`

#### Signature

```cpp
static Catalog load_parquet(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_parquet(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_parquet("PlussMic/data/messy_data.csv");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_parquet`

#### Signature

```cpp
static Catalog load_parquet(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_parquet(std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_parquet("PlussMic/data/messy_data.csv", {"nrows", 16});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_txt`

#### Signature

```cpp
static Catalog load_txt(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_txt(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_txt("PlussMic/data/messy_data.csv");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_txt`

#### Signature

```cpp
static Catalog load_txt(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_txt(std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_txt("PlussMic/data/messy_data.csv", {"nrows", 16});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `masked_select`

#### Signature

```cpp
Catalog masked_select(self, const pluss::Series& mask) const
```

#### Parameters

`mask` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto out = catalog.masked_select(mask);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `max`

#### Signature

```cpp
Stream max(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.max("metric");
```

#### Output

```text
Series([20.0])
```

### `mean`

#### Signature

```cpp
Stream mean(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.mean("metric");
```

#### Output

```text
Series([20.0])
```

### `merge`

#### Signature

```cpp
Catalog merge(self, const Catalog& other, std::string_view on, std::string_view how = "inner") const
```

#### Parameters

`other` : `const Catalog&`
    See the signature for the supported argument form.

`on` : `std::string_view`
    See the signature for the supported argument form.

`how` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this when you need to merge compatible objects or extend the current one with another input.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

using namespace pluss::kindle::literals;
auto right = pluss::kindle::Catalog({"id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})), "score"_k = pluss::kindle::Stream(pluss::make_series({0.9, 0.7, 0.8}))});
auto out = catalog.merge(right, "id", "inner");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `min`

#### Signature

```cpp
Stream min(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.min("metric");
```

#### Output

```text
Series([20.0])
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

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.nfields();
```

#### Output

```text
3
```

### `note`

#### Signature

```cpp
Catalog note(self) const
```

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.note();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `notnull`

#### Signature

```cpp
Catalog notnull(self, std::initializer_list<std::string_view> keys =
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.notnull();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `nstreams`

#### Signature

```cpp
std::int64_t nstreams(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.nstreams();
```

#### Output

```text
3
```

### `query`

#### Signature

```cpp
Catalog query(self, const QueryFn& fn) const
```

#### Parameters

`fn` : `const QueryFn&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.query([](const pluss::kindle::Catalog& c) { return c.stream("metric").as_series().gt(15.0); });
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `rename`

#### Signature

```cpp
Catalog rename(self, const std::unordered_map<std::string, std::string>& mapping) const
```

#### Parameters

`mapping` : `const std::unordered_map<std::string, std::string>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This adjusts labels without changing the underlying payload values.

#### See also

- `Catalog rename(self, std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const`
- `Catalog rename(self, std::string_view from, std::string_view to) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::unordered_map<std::string, std::string> renames{{"metric", "distance_km"}};
auto out = catalog.rename(renames);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `rename`

#### Signature

```cpp
Catalog rename(self, std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const
```

#### Parameters

`mapping` : `std::initializer_list<std::pair<std::string_view, std::string_view>>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This adjusts labels without changing the underlying payload values.

#### See also

- `Catalog rename(self, std::string_view from, std::string_view to) const`
- `Catalog rename(self, const std::unordered_map<std::string, std::string>& mapping) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.rename({{"metric", "distance_km"}});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `rename`

#### Signature

```cpp
Catalog rename(self, std::string_view from, std::string_view to) const
```

#### Parameters

`from` : `std::string_view`
    See the signature for the supported argument form.

`to` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This adjusts labels without changing the underlying payload values.

#### See also

- `Catalog rename(self, std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const`
- `Catalog rename(self, const std::unordered_map<std::string, std::string>& mapping) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.rename("metric", "distance_km");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `rename_`

#### Signature

```cpp
Catalog& rename_(self, std::string_view from, std::string_view to)
```

#### Parameters

`from` : `std::string_view`
    See the signature for the supported argument form.

`to` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

This adjusts labels without changing the underlying payload values.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.rename_("metric", "distance_km");
```

### `sample`

#### Signature

```cpp
Catalog sample(self, std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) const
```

#### Parameters

`n` : `std::int64_t`
    See the signature for the supported argument form.

`random_state` : `std::optional<std::uint64_t>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.sample(2, 7);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `save_as`

#### Signature

```cpp
void save_as(self, std::string_view target_type, std::string_view filename, const pluss::loader::Config& config =
```

#### Parameters

`target_type` : `std::string_view`
    See the signature for the supported argument form.

`filename` : `std::string_view`
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
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.save_as("json", "out.json");
```

#### Output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
void save_csv(self, std::string_view filename) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_csv(self, std::string_view filename, const pluss::loader::Config& config) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.save_csv("out.csv");
```

#### Output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
void save_csv(self, std::string_view filename, const pluss::loader::Config& config) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_csv(self, std::string_view filename) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.save_csv("out.csv");
```

#### Output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
void save_excel(self, std::string_view filename) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_excel(self, std::string_view filename, const pluss::loader::Config& config) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.save_excel("out.xlsx");
```

#### Output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
void save_excel(self, std::string_view filename, const pluss::loader::Config& config) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_excel(self, std::string_view filename) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.save_excel("out.xlsx");
```

#### Output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
void save_json(self, std::string_view filename) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_json(self, std::string_view filename, const pluss::loader::Config& config) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.save_json("out.json");
```

#### Output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
void save_json(self, std::string_view filename, const pluss::loader::Config& config) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_json(self, std::string_view filename) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.save_json("out.json");
```

#### Output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
void save_parquet(self, std::string_view filename) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_parquet(self, std::string_view filename, const pluss::loader::Config& config) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.save_parquet("out.parquet");
```

#### Output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
void save_parquet(self, std::string_view filename, const pluss::loader::Config& config) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_parquet(self, std::string_view filename) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.save_parquet("out.parquet");
```

#### Output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
void save_txt(self, std::string_view filename) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_txt(self, std::string_view filename, const pluss::loader::Config& config) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.save_txt("out.txt");
```

#### Output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
void save_txt(self, std::string_view filename, const pluss::loader::Config& config) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the current object to disk using the corresponding PlussMic saver route.

#### See also

- `void save_txt(self, std::string_view filename) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.save_txt("out.txt");
```

#### Output

```text
saved: out.*
```

### `select`

#### Signature

```cpp
Catalog select(self, const std::vector<std::string>& field_names) const
```

#### Parameters

`field_names` : `const std::vector<std::string>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### See also

- `Catalog select(self, std::initializer_list<std::string_view> field_names) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::vector<std::string> names{"id", "metric"};
auto out = catalog.select(names);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `select`

#### Signature

```cpp
Catalog select(self, std::initializer_list<std::string_view> field_names) const
```

#### Parameters

`field_names` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### See also

- `Catalog select(self, const std::vector<std::string>& field_names) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.select({"id", "metric"});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `series`

#### Signature

```cpp
Catalog series(self) const
```

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.series();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `set`

#### Signature

```cpp
Catalog set(self, std::string_view key, const Stream& value) const
```

#### Parameters

`key` : `std::string_view`
    See the signature for the supported argument form.

`value` : `const Stream&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.set_stream("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `set_`

#### Signature

```cpp
Catalog& set_(self, std::string_view key, const Stream& value)
```

#### Parameters

`key` : `std::string_view`
    See the signature for the supported argument form.

`value` : `const Stream&`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.set_stream_("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

### `set_stream`

#### Signature

```cpp
Catalog set_stream(self, std::string_view key, const Stream& value) const
```

#### Parameters

`key` : `std::string_view`
    See the signature for the supported argument form.

`value` : `const Stream&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.set_stream("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `set_stream_`

#### Signature

```cpp
Catalog& set_stream_(self, std::string_view key, const Stream& value)
```

#### Parameters

`key` : `std::string_view`
    See the signature for the supported argument form.

`value` : `const Stream&`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.set_stream_("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

### `set_streams`

#### Signature

```cpp
Catalog set_streams(self, std::initializer_list<NamedStream> items) const
```

#### Parameters

`items` : `std::initializer_list<NamedStream>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Catalog set_streams(self, std::initializer_list<std::pair<std::string_view, Stream>> items) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

using namespace pluss::kindle::literals;
auto out = catalog.set_streams({
    "flag"_k = pluss::kindle::Stream(pluss::make_series({1, 0, 1})),
});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `set_streams`

#### Signature

```cpp
Catalog set_streams(self, std::initializer_list<std::pair<std::string_view, Stream>> items) const
```

#### Parameters

`items` : `std::initializer_list<std::pair<std::string_view, Stream>>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Catalog set_streams(self, std::initializer_list<NamedStream> items) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.set_streams({
    {"flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1}))},
});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `set_streams_`

#### Signature

```cpp
Catalog& set_streams_(self, std::initializer_list<NamedStream> items)
```

#### Parameters

`items` : `std::initializer_list<NamedStream>`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Catalog& set_streams_(self, std::initializer_list<std::pair<std::string_view, Stream>> items)`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

using namespace pluss::kindle::literals;
catalog.set_streams_({
    "flag"_k = pluss::kindle::Stream(pluss::make_series({1, 0, 1})),
});
```

### `set_streams_`

#### Signature

```cpp
Catalog& set_streams_(self, std::initializer_list<std::pair<std::string_view, Stream>> items)
```

#### Parameters

`items` : `std::initializer_list<std::pair<std::string_view, Stream>>`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Catalog& set_streams_(self, std::initializer_list<NamedStream> items)`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

catalog.set_streams_({
    {"flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1}))},
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
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.show();
```

### `slice`

#### Signature

```cpp
Catalog slice(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    See the signature for the supported argument form.

`stop` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.slice(0, 2);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `sort`

#### Signature

```cpp
Catalog sort(self, std::string_view by_field, bool ascending = true) const
```

#### Parameters

`by_field` : `std::string_view`
    See the signature for the supported argument form.

`ascending` : `bool`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.sort("metric", false);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `sort_by`

#### Signature

```cpp
Catalog sort_by(self, const std::vector<std::string>& fields, const std::vector<bool>& ascending =
```

#### Parameters

`fields` : `const std::vector<std::string>&`
    See the signature for the supported argument form.

`ascending` : `const std::vector<bool>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.sort_by({"city", "id"}, {true, false});
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `stream`

#### Signature

```cpp
Stream stream(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.stream("metric");
```

#### Output

```text
Series([10.0, 20.0, 30.0])
```

### `streams`

#### Signature

```cpp
std::vector<Stream> streams(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<Stream>`
    Materialized collection of stream objects.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.streams();
```

#### Output

```text
[Stream(id), Stream(metric), Stream(city)]
```

### `sum`

#### Signature

```cpp
Stream sum(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

Use this when you want a summary view instead of carrying the full input forward unchanged.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.sum("metric");
```

#### Output

```text
Series([20.0])
```

### `tabelize`

#### Signature

```cpp
pluss::table::DataFrame tabelize(self, const TabelizePolicy& policy =
```

#### Parameters

`policy` : `const TabelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`pluss::table::DataFrame`
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

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Inner;
auto out = catalog.tabelize(policy);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `tail`

#### Signature

```cpp
Catalog tail(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.tail(2);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `take`

#### Signature

```cpp
Catalog take(self, std::span<const std::int64_t> indices) const
```

#### Parameters

`indices` : `std::span<const std::int64_t>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to narrow the current object to the rows or depth range you want to inspect next.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::vector<std::int64_t> picks{0, 2};
auto out = catalog.take(picks);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `text_preprocess`

#### Signature

```cpp
Catalog text_preprocess(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = catalog.text_preprocess("text");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `tfidf`

#### Signature

```cpp
Catalog tfidf(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = catalog.tfidf("text");
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `timeindex`

#### Signature

```cpp
Catalog timeindex(self) const
```

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.timeindex();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `timeline`

#### Signature

```cpp
Catalog timeline(self) const
```

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.timeline();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
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

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.to_json();
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

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.to_string();
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `to_table`

#### Signature

```cpp
pluss::table::DataFrame to_table(self, const TabelizePolicy& policy =
```

#### Parameters

`policy` : `const TabelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`pluss::table::DataFrame`
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

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Inner;
auto out = catalog.to_table(policy, true);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `transform`

#### Signature

```cpp
Catalog transform(self, const std::function<Stream(const Stream&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Catalog transform(self, std::string_view field, const std::function<Stream(const Stream&)>& fn) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.transform([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `transform`

#### Signature

```cpp
Catalog transform(self, std::string_view field, const std::function<Stream(const Stream&)>& fn) const
```

#### Parameters

`field` : `std::string_view`
    See the signature for the supported argument form.

`fn` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This mutates or derives structure while keeping the surrounding object model intact.

#### See also

- `Catalog transform(self, const std::function<Stream(const Stream&)>& fn) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.transform("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
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

This call is part of the public `Catalog` API and is documented here using a concrete usage example.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = catalog.unique();
```

#### Output

```text
true
```

### `where`

#### Signature

```cpp
Catalog where(self, const pluss::Series& condition) const
```

#### Parameters

`condition` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto catalog = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto out = catalog.where(mask);
```

#### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
