# Thread And Atlas Guide

## Thread

`Thread` is a typed column wrapper around `Series`, `Note`, `Timeline`, or `TimeIndex`.
Use it when you want column operations without carrying the full `DataFrame` around.

```cpp
auto sales = pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}));
auto cities = pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}));

auto cleaned = sales.fillna(0.0).clip(0.0, 150.0);
auto avg = cleaned.mean();
auto city_mask = cities.contains("Accra");
```

Notes:

- `Thread` is the easiest bridge point between the Pluss domain types and table verbs
- when you want the original typed payload back, use `as_series()`, `as_note()`, `as_timeline()`, or `as_timeindex()`

## Row-Space Helpers

```cpp
auto head = sales.head(2);
auto tail = sales.tail(2);
auto slice = sales.slice(0, 2);
auto taken = sales.take(std::array<std::int64_t, 2>{0, 2});
auto filtered = sales.filter(city_mask);
```

## Atlas

`Atlas` is the frame-stack model in PlussMic. It is useful when you want a small set of aligned `DataFrame` objects with an optional depth axis.

```cpp
auto f0 = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))},
});

auto f1 = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema"}))},
    {"sales", pluss::table::Thread(pluss::make_series({130.0, 125.0}))},
});

auto labels = pluss::table::Thread(pluss::make_note({"train", "test"}));
auto atlas = pluss::table::Atlas({f0, f1}, labels);

atlas.head(1).show();
auto sorted = atlas.sort_depth();
auto first = atlas.frame(0);
```

## Atlas To Kindle

```cpp
auto archive = atlas.kindelize();
auto roundtrip = archive.tabelize();
```
