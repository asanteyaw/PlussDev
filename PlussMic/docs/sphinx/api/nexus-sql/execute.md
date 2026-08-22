<!-- pluss-hand-authored -->

# NexusQuery::execute

[Back to NexusQuery API Reference](../../nexus_query_api_reference)

## Signature

```cpp
NexusQuery::execute(std::string_view statement) const -> DataFrame
```

## What It Does

Executes a SQL `SELECT` statement against the tables loaded into the `Nexus`
workspace and returns the materialized result as a `DataFrame`.

The `FROM` clause must name a real workspace table such as `customers`,
`orders`, or `trans`. `FROM df` is intentionally rejected, because there is no
single implicit calling frame on a multi-table workspace.

## Example

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
    {"name", pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto orders = DataFrame({
    {"order_id", pluss::make_series({100.0, 101.0, 102.0})},
    {"customer_id", pluss::make_series({1.0, 2.0, 2.0})},
    {"amount", pluss::make_series({50.0, 75.0, 20.0})},
});

auto nx = Nexus({
    {"customers", customers},
    {"orders", orders},
});

auto out = nx.query().execute(
    "SELECT customers.customer_id, orders.amount "
    "FROM customers "
    "JOIN orders ON customers.customer_id = orders.customer_id "
    "ORDER BY orders.amount DESC");

std::cout << out.to_string();
```

### Output

```text
 shape: (3, 2)
┌─────────────┬────────┐
│ customer_id │ amount │
├─────────────┼────────┤
│ 2           │ 75     │
│ 1           │ 50     │
│ 2           │ 20     │
└─────────────┴────────┘
```
