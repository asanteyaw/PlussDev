<!-- pluss-hand-authored -->

# Nexus::has

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::has(std::string_view key) const -> bool
```

## What It Does

Reports whether a table with the given name exists in the workspace, in constant
time (a hash-map lookup, not a linear scan of [`keys()`](keys.md)). This is the
non-throwing way to check for a table's presence before calling
[`frame(key)`](frame.md), which throws for an unknown key.

## Parameters

**key** (`std::string_view`)

The table name to look up.

## Returns

`bool`
    `true` if a table with this name exists; `false` otherwise.

## Examples

Example 1. Check both a present and an absent key.

```cpp
using namespace pluss::table;

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

std::cout << std::boolalpha << biz.has("customers") << " " << biz.has("products");
```

### Output

```text
true false
```

## Notes

- Lookup is by exact name match — no pattern matching or case-insensitivity.
- Prefer `has(key)` over a `try`/`catch` around [`frame(key)`](frame.md) when you
  only need to know presence, not fetch the table itself.
- [`get(key)`](get.md) combines both concerns in one call — it returns
  `std::optional<DataFrame>`, so a single call answers "does it exist" and "here it
  is" together.

## See Also

- [`Nexus::get`](get.md): fetch the table itself as an `optional`, folding this
  presence check into the fetch.
- [`Nexus::frame`](frame.md): fetch the table directly, throwing on an unknown key.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
