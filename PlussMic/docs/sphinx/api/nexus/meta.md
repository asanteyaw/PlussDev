<!-- pluss-hand-authored -->

# Nexus::meta

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
Nexus::meta(std::string_view key) const -> std::optional<TableMeta>

Nexus::meta(std::string_view key, const TableMeta& value) const -> Nexus

Nexus& Nexus::meta_(std::string_view key, const TableMeta& value)
```

## What It Does

A trio of exact aliases: the one-argument `meta(key)` calls
[`table_meta(key)`](table_meta.md) internally, `meta(key, value)` calls
[`set_table_meta(key, value)`](set_table_meta.md), and `meta_(key, value)` calls
[`set_table_meta_(key, value)`](set_table_meta.md). All three behave identically to their
canonical counterparts — same throws, same non-mutating/mutating split, same return types.
`meta` exists purely as a shorter, more conversational spelling for the same three calls.

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
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
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

## Notes

- Purely a naming alternative — see [`table_meta()`](table_meta.md) and
  [`set_table_meta()`](set_table_meta.md) for the full documentation, including throw
  conditions and the read-modify-write pattern for partial updates.

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
