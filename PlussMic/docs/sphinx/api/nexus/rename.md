<!-- pluss-hand-authored -->

# Nexus::rename

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::rename(std::string_view from, std::string_view to) const -> Nexus
```

## What It Does

Changes one table key inside the workspace and returns the resulting `Nexus`,
without altering table order or table contents.

This is a schema-level naming operation, not a data transformation. The table
itself stays exactly where it was in the workspace; only its key changes.

`rename()` is deeper than a plain string substitution, though. Internally, it
also updates the schema objects that refer to table names:

- the table key list
- stored table metadata
- declared relationships whose `left_table` or `right_table` matched the old key
- saved join plans that reference the old table name

That means the operation is intended to preserve the workspace's internal
consistency, not just make the visible key list look nicer.

## Parameters

**from** (`std::string_view`)

Existing table key to rename.

- Must not be empty.
- Must already exist in the workspace.

**to** (`std::string_view`)

Replacement key.

- Must not be empty.
- Must differ from `from`.
- Must not already exist in the workspace.
- If it has the auto-key shape `table_N`, it advances the internal auto-name
  counter so later unnamed [`inject()`](inject.md) calls continue from `N + 1`
  rather than reusing lower numbers.

## Returns

`Nexus`
    A new workspace with the renamed table key applied.

    Table order, table contents, and every unaffected table key remain unchanged.
    The source `Nexus` is unchanged.

## Examples

Example 1. Rename one table key while preserving its position.

```cpp
using namespace pluss::table;

auto print_keys = [](const Nexus& nx) {
  for (std::int64_t i = 0; i < nx.ntables(); ++i) {
    if (i > 0) {
      std::cout << ", ";
    }
    std::cout << nx.keys()[static_cast<std::size_t>(i)];
  }
  std::cout << "\n";
};

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
});

auto payments = DataFrame({
    {"payment_id", Thread(pluss::make_series({9001.0, 9002.0}))},
    {"order_id",   Thread(pluss::make_series({101.0, 102.0}))},
});

auto refunds = DataFrame({
    {"refund_id", Thread(pluss::make_series({77.0}))},
    {"order_id",  Thread(pluss::make_series({102.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
    {"payments", payments},
    {"refunds", refunds},
});

auto out = biz.rename("orders", "purchases");
print_keys(out);
print_keys(biz);
```

### Output

```text
customers, purchases, payments, refunds
customers, orders, payments, refunds
```

Example 2. Continuing from Example 1, renaming to an explicit auto-style key
changes the future auto-generated names used by unnamed `inject(...)`.

```cpp
auto shipments = DataFrame({
    {"shipment_id", Thread(pluss::make_series({501.0, 502.0}))},
    {"order_id",    Thread(pluss::make_series({101.0, 102.0}))},
});

auto out = biz.rename("orders", "table_9").inject(shipments);
print_keys(out);
```

### Output

```text
customers, table_9, payments, refunds, table_10
```

Example 3. Continuing from Example 1, validation failures are explicit.

```cpp
try {
  auto bad = biz.rename("", "purchases");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto bad = biz.rename("orders", "");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto bad = biz.rename("orders", "orders");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto bad = biz.rename("missing", "purchases");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto bad = biz.rename("orders", "payments");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Nexus::rename: source key cannot be empty
Nexus::rename: target key cannot be empty
Nexus::rename: source and target keys must differ
Nexus::rename: unknown key 'missing'
Nexus::rename: target key already exists 'payments'
```

## Notes

- `rename()` is non-mutating.
- The renamed table stays in the same position in the workspace; only the key
  changes.
- Table metadata keyed by the old name is moved to the new name.
- Relationships and saved join plans that reference the old table name are
  rewritten to reference the new one.
- If `to` matches the auto-generated naming convention `table_N`, later unnamed
  [`inject()`](inject.md) calls continue from the next number after `N`.
- The mutating sibling is `rename_()`, but this page documents the non-mutating
  pipeline-friendly form.

## See Also

- [`Nexus::inject`](inject.md): add new tables, including unnamed auto-keyed
  tables.
- [`Nexus::keys`](keys.md): inspect the renamed key list.
- [`Nexus::select`](select.md): project and reorder tables without renaming them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="profile.html">
    <span class="label">Previous</span>
    <span class="title">profile</span>
  </a>
  <a class="next-link" href="scan.html">
    <span class="label">Next</span>
    <span class="title">scan</span>
  </a>
</div>
