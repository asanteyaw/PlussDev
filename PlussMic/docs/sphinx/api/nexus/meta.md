<!-- pluss-hand-authored -->

# Nexus::meta

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
std::optional<TableMeta> Nexus::meta(std::string_view key) const

Nexus Nexus::meta(std::string_view key, const TableMeta& value) const
Nexus Nexus::meta(std::initializer_list<std::pair<std::string_view, TableMeta>> values) const

Nexus& Nexus::meta_(std::string_view key, const TableMeta& value)
Nexus& Nexus::meta_(std::initializer_list<std::pair<std::string_view, TableMeta>> values)
```

## What It Does

A trio of exact aliases: the one-argument `meta(key)` calls
[`table_meta(key)`](table_meta.md) internally, `meta(key, value)` calls
[`set_table_meta(key, value)`](set_table_meta.md), and `meta_(key, value)` calls
[`set_table_meta_(key, value)`](set_table_meta.md). All three behave identically to their
canonical counterparts — same throws, same non-mutating/mutating split, same return types.
`meta` exists purely as a shorter, more conversational spelling for the same three calls.
The bulk setter overload mirrors [`inject()`](inject.md): if you already know several
table metadata records at once, pass them as one brace list, `std::map`, or
`std::unordered_map` instead of chaining `.meta(...).meta(...).meta(...)`.

## Parameters

See [`table_meta()`](table_meta.md) and [`set_table_meta()`](set_table_meta.md) for the
full parameter documentation — identical here.

## Returns

Identical to [`table_meta()`](table_meta.md) (getter overload) and
[`set_table_meta()`](set_table_meta.md) (the two setter overloads).

## Examples

Example 1. `meta(key, value)` and `meta(key)` behave exactly like their canonical
counterparts.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto biz = Nexus({{"customers", customers}});

TableMeta info;
info.grain = "one row per customer";
info.primary_key = {"customer_id"};

auto tagged = biz.meta("customers", info);
auto fetched = tagged.meta("customers");

std::cout << std::boolalpha << fetched.has_value() << "\n";
std::cout << *fetched->grain;
```

### Output

```text
true
one row per customer
```

Example 2. Bulk metadata declaration keeps multi-table setup shorter.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0}))},
});
auto orders = DataFrame({
    {"order_id",    Slate(pluss::make_series({10.0, 11.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 2.0}))},
    {"ordered_at",  Slate(pluss::make_timeline({
        "2026-01-01T00:00:00Z",
        "2026-01-03T00:00:00Z",
    }))},
});

TableMeta customers_meta;
customers_meta.primary_key = {"customer_id"};

TableMeta orders_meta;
orders_meta.primary_key = {"order_id"};
orders_meta.time_field = "ordered_at";

auto tagged = Nexus({
    {"customers", customers},
    {"orders", orders},
}).meta({
    {"customers", customers_meta},
    {"orders", orders_meta},
});

std::cout << tagged.meta("customers")->primary_key.front() << "\n";
std::cout << tagged.meta("orders")->time_field.value();
```

### Output

```text
customer_id
ordered_at
```

## Notes

- Purely a naming alternative — see [`table_meta()`](table_meta.md) and
  [`set_table_meta()`](set_table_meta.md) for the full documentation, including throw
  conditions and the read-modify-write pattern for partial updates.
- The bulk setter is still validated one table at a time. An unknown key throws the same
  `"Nexus::set_table_meta: unknown table '<name>'"` error you would get from the
  one-table form.

## See Also

- [`Nexus::table_meta`](table_meta.md): the canonical getter this aliases.
- [`Nexus::set_table_meta`](set_table_meta.md): the canonical setter pair this aliases.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="materialize.html">
    <span class="label">Previous</span>
    <span class="title">materialize</span>
  </a>
  <a class="next-link" href="names.html">
    <span class="label">Next</span>
    <span class="title">names</span>
  </a>
</div>
