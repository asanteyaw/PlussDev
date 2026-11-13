<!-- pluss-hand-authored -->

# Nexus::get

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
std::optional<DataFrame> Nexus::get(std::string_view key) const
```

## What It Does

Fetches a table by name, the same as [`frame(key)`](frame.md), but returns
`std::nullopt` for an unknown key instead of throwing. This is the non-throwing
counterpart to `frame()` — reach for `get()` when a missing table is an expected,
handled case rather than a programming error.

## Parameters

**key** (`std::string_view`)

The table name to look up.

## Returns

`std::optional<DataFrame>`
    The table if `key` exists; `std::nullopt` otherwise.

## Examples

Example 1. A present key returns the table wrapped in an engaged `optional`.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
    {"name",        pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto orders = DataFrame({
    {"order_id",    pluss::make_series({100.0, 101.0, 102.0})},
    {"customer_id", pluss::make_series({1.0, 2.0, 2.0})},
    {"amount",      pluss::make_series({50.0, 75.0, 20.0})},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

auto found = biz.get("customers");
std::cout << std::boolalpha << found.has_value() << "\n";
std::cout << *found;
```

### Output

```text
true
=========================
      | customer_id  name
-------------------------
0     | 1            Ada 
1     | 2            Bola
2     | 3            Chen
-------------------------
[3 cs x 2 feat]
```

Example 2. An absent key returns `std::nullopt`, not a throw.

```cpp
auto missing = biz.get("products");
std::cout << std::boolalpha << missing.has_value();
```

### Output

```text
false
```

Compare this with [`frame("products")`](frame.md), which would throw
`"Nexus::frame: unknown key 'products'"` for the same input instead of returning a
disengaged `optional`.

## Notes

- `get()` only has a name-based overload — there's no positional `get(int64_t)`.
  Use [`frame(index)`](frame.md) or [`dloc(index)`](dloc.md) for positional access.
- [`has(key)`](has.md) answers the same presence question without materializing the
  `DataFrame` at all, when you don't need the table itself.

## See Also

- [`Nexus::frame`](frame.md): the throwing counterpart, plus positional access.
- [`Nexus::has`](has.md): a presence-only check, without fetching the table.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_atlas.html">
    <span class="label">Previous</span>
    <span class="title">from_atlas</span>
  </a>
  <a class="next-link" href="graph.html">
    <span class="label">Next</span>
    <span class="title">graph</span>
  </a>
</div>
