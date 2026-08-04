# Part XII: Bundle: Slot, Registry, and Axel

Bundle is PlussMic's heterogeneous record model. It is useful when values are
not naturally represented as one typed `Thread`.

## Slot

A `Slot` is a vector of values. Values can represent multiple scalar/value
families depending on the Bundle API surface.

Common Slot operations:

```cpp
slot.defined();
slot.size();
slot.empty();
slot.values();
slot.at(0);

slot.head(10);
slot.tail(10);
slot.slice(10, 20);
slot.take(indices);
slot.filter(predicate);
slot.transform(fn);
```

Slot mutation style follows the Pluss convention:

```cpp
auto next = slot.push(value);
slot.push_(value);
```

Use non-mutating operations in pipelines and mutating operations when updating
owned state.

## Registry

A `Registry` is a named collection of slots.

```cpp
auto registry = pluss::bundle::Registry::from_kindle(catalog);
auto catalog2 = registry.to_kindle();
```

Load and save registry data:

```cpp
auto reg = pluss::bundle::Registry::load_csv("data.csv");
reg.save_json("registry.json");
```

Inspect:

```cpp
registry.nfields();
registry.fields();
registry.has("field");
registry.get("field");
registry.sizes();
registry.profiles();
```

Select and reshape:

```cpp
auto first = registry.head(10);
auto rows = registry.slice(10, 20);
auto one_row = registry.cloc(0);
auto selected = registry.select({"name", "score"});
auto dropped = registry.drop("temporary");
auto renamed = registry.rename("old", "new");
```

Transform:

```cpp
auto changed = registry.apply("score", [](const pluss::bundle::Slot& slot) {
  return slot;
});

auto all = registry.map([](const pluss::bundle::Slot& slot) {
  return slot;
});
```

Update fields:

```cpp
auto with_slot = registry.set("new_field", slot);
auto pushed = registry.push("events", value);
```

Combine:

```cpp
auto merged = registry.combine(other, "_other");
```

## Kindle bridge

Some Registry operations require slots that can map cleanly to streams. The public
surface no longer exposes separate compatibility checkers; `to_kindle()` is the
single contract point.

```cpp
try {
  auto catalog = registry.to_kindle();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

## Axel

`Axel` is the higher collection-oriented bundle type. Use it when you need to
organize registries/slots into a broader bundle workflow. See the
[Axel API Reference](../axel_api_reference.md) for exact constructors and
operations.

## Bundle conversion strategy

DataFrame path:

```cpp
auto catalog = df.kindelize();
auto registry = pluss::bundle::Registry::from_kindle(catalog);
```

Back to table:

```cpp
auto catalog = registry.to_kindle();
auto df = catalog.tabelize();
```

Use Bundle for heterogeneous data, but return to DataFrame when you need
rectangular analysis, group-by, plotting, or file export.
