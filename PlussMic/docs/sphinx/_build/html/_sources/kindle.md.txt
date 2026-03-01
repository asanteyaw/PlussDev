# Kindle Model Guide

## When To Use Kindle Types

The kindle model is stream-first:

- `Stream`: one typed vector plus optional validity mask and case labels
- `Catalog`: named collection of streams
- `Archive`: depth stack of catalogs

This is the right model when case alignment and stream-native transforms matter more than table presentation.

## Stream Basics

```cpp
auto metric = pluss::kindle::Stream(
    pluss::make_series({10.0, 20.0, 30.0}),
    std::nullopt,
    pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"})));

metric = metric.with_meta("source", "demo").with_meta("unit", "km");

auto cleaned = metric.fillna(0.0).clip(0.0, 35.0);
auto filtered = cleaned.filter(cleaned.as_series().gt(15.0));
```

## Catalog Basics

```cpp
using namespace pluss::kindle::literals;

auto id = pluss::kindle::Stream(pluss::make_series({1, 2, 3}));
auto city = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}));
auto value = pluss::kindle::Stream(pluss::make_series({10.0, 15.0, 20.0}));

auto catalog = pluss::kindle::Catalog({
    "id"_k = id,
    "city"_k = city,
    "value"_k = value,
});

catalog.select({"id", "value"}).show();
catalog.sort("value").show();
```

## Text And Similarity Helpers

```cpp
auto text_catalog = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto clean = text_catalog.text_preprocess("text");
auto keywords = text_catalog.extract_keywords("text", 5);
auto tfidf = text_catalog.tfidf("text");
```

## Archive Basics

```cpp
auto c0 = catalog;
auto c1 = catalog.rename({{"value", "value_test"}});
auto depth = pluss::kindle::Stream(pluss::make_note({"train", "test"}));

auto archive = pluss::kindle::Archive({c0, c1}, depth);

auto first = archive.catalog(0);
auto mapped = archive.map_catalogs([](const pluss::kindle::Catalog& c) {
  return c.head(2);
});
```

## Convert Back To Table Model

```cpp
pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Outer;
policy.index_mode = pluss::kindle::IndexMode::Positional;

auto df = catalog.tabelize(policy);
auto atlas = archive.tabelize(policy);
```
