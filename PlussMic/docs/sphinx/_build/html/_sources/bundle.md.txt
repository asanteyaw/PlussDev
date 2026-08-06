# Bundle Model Guide

## When To Use Bundle Types

The bundle model is the heterogeneous record layer:

- `Slot`: sequence of heterogeneous Pluss values
- `Registry`: named set of slots
- `Axel`: depth stack of registries

Use it when one field can hold different domain types, or when you want a lighter record/batch abstraction than `DataFrame`.

## Slot

```cpp
auto ids = pluss::bundle::Slot({
    pluss::make_series(1, pluss::codem::ScalarType::Int64),
    pluss::make_series(2, pluss::codem::ScalarType::Int64),
    pluss::make_series(3, pluss::codem::ScalarType::Int64),
});

ids.push_(pluss::make_series(4, pluss::codem::ScalarType::Int64));
auto even = ids.filter([](const pluss::bundle::Slot::Value& v) {
  return std::get<pluss::Series>(v).to_vec<std::int64_t>()[0] % 2 == 0;
});
```

## Registry

```cpp
auto cities = pluss::bundle::Slot({
    pluss::make_note("Accra"),
    pluss::make_note("Tema"),
    pluss::make_note("Kumasi"),
});

auto registry = pluss::bundle::Registry({
    {"id", ids},
    {"city", cities},
});

auto selected = registry.fields({"id"});
auto renamed = registry.rename({{"city", "location"}});
auto profiled = registry.profiles();
```

## Axel

```cpp
auto train = registry;
auto test = registry.rename({{"city", "city_test"}});
auto labels = pluss::bundle::Slot({pluss::make_note("train"), pluss::make_note("test")});

auto axel = pluss::bundle::Axel({train, test}, labels);

auto first = axel.head(1);
auto combined = axel.concat_depth(first);
```

## Kindle Bridge

```cpp
auto stream = ids.to_kindle();
auto catalog = registry.to_kindle();
auto archive = axel.to_kindle();

auto back_slot = pluss::bundle::Slot::from_kindle(stream);
auto back_registry = pluss::bundle::Registry::from_kindle(catalog);
auto back_axel = pluss::bundle::Axel::from_kindle(archive);
```
