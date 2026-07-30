<!-- pluss-hand-authored -->

# Nexus::validate_keys

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
Nexus::validate_keys(std::string_view table) const -> bool

Nexus::validate_keys(std::string_view table, std::initializer_list<std::string_view> keys) const -> bool
```

## What It Does

Checks whether a candidate key set is both **complete** (no row has a missing value in any
of the key features) and **unique** (no two rows share the same combination of key values)
for a table. This is the underlying check every other key-auditing method on this class
([`detect_duplicates()`](detect_duplicates.md), [`grain_audit()`](grain_audit.md)) builds
on, collapsed down to a single `bool`.

Two overloads, differing in where the key set comes from:

- The one-argument overload uses the table's declared
  [`TableMeta::primary_key`](table_meta.md) — the table must already have metadata
  declared, with a non-empty `primary_key`, or the call throws.
- The two-argument overload takes an explicit `keys` list instead, ignoring whatever
  metadata (if any) is declared for the table.

## Parameters

**table** (`std::string_view`)
    The table to check. Must already exist in the workspace.

**keys** (`std::initializer_list<std::string_view>`, two-argument overload)
    The candidate key features to check for completeness and uniqueness. Each name must be
    non-empty, unique within the request, and must name a real feature on `table`.

## Returns

`bool`
    `true` if every row has a non-missing value for every key feature *and* no two rows
    share the same key combination; `false` if either condition fails.

## Examples

Example 1. Using the declared `primary_key`, on a table with a duplicate key value.

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy", "Cy2"}))},
});

auto biz = Nexus({{"customers", customers}});

TableMeta meta;
meta.primary_key = {"customer_id"};
auto tagged = biz.set_table_meta("customers", meta);

std::cout << std::boolalpha << tagged.validate_keys("customers");
```

### Output

```text
false
```

`customer_id = 3.0` appears twice, so the declared primary key is not unique.

Example 2. Calling the one-argument overload without declared metadata throws.

```cpp
try {
  auto bad = biz.validate_keys("customers");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::validate_keys: no metadata declared for table 'customers'
```

Example 3. The two-argument overload checks an explicit key set instead, independent of
any declared metadata.

```cpp
std::cout << std::boolalpha << biz.validate_keys("customers", {"customer_id"}) << "\n";

auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
});
auto with_orders = biz.inject("orders", orders);
std::cout << std::boolalpha << with_orders.validate_keys("orders", {"order_id"});
```

### Output

```text
false
true
```

`orders.order_id` has no duplicates and no missing values, so it validates as a key even
though `orders` never had `TableMeta` declared at all.

## Notes

- The one-argument overload requires declared `TableMeta` with a non-empty `primary_key`;
  the two-argument overload needs neither.
- Both completeness (no missing values) and uniqueness (no duplicate combinations) must
  hold for the result to be `true`.
- Results are cached internally per `(table, key set)` — repeated calls with the same
  arguments are cheap.

## See Also

- [`Nexus::detect_duplicates`](detect_duplicates.md): get the actual duplicate rows instead
  of a single `bool`.
- [`Nexus::grain_audit`](grain_audit.md): run this same check against every declared key
  set (primary and candidate) on a table at once, as a `DataFrame`.
- [`Nexus::table_meta`](table_meta.md): declare the `primary_key` this method's
  one-argument overload reads.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unique.html">
    <span class="label">Previous</span>
    <span class="title">unique</span>
  </a>
  <a class="next-link" href="validate_relationship.html">
    <span class="label">Next</span>
    <span class="title">validate_relationship</span>
  </a>
</div>
