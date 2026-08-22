<!-- pluss-hand-authored -->

# NexusQuery::prepare

[Back to NexusQuery API Reference](../../nexus_query_api_reference)

## Signature

```cpp
NexusQuery::prepare(std::string_view statement) const -> NexusQueryPrepared
```

## What It Does

Builds a reusable prepared workspace query from a SQL statement template. The
returned `NexusQueryPrepared` can later be executed with runtime SQL parameter
bindings.

## Example

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto orders = DataFrame({
    {"order_id", pluss::make_series({100.0, 101.0, 102.0})},
    {"customer_id", pluss::make_series({1.0, 2.0, 2.0})},
    {"amount", pluss::make_series({50.0, 75.0, 20.0})},
});

auto nx = Nexus({{"orders", orders}});
auto stmt = nx.query().prepare(
    "SELECT orders.amount "
    "FROM orders "
    "WHERE orders.amount >= {min_amount} "
    "ORDER BY orders.amount DESC");

auto out = stmt.execute({{"min_amount", 50.0}});
std::cout << out.to_string();
```

### Output

```text
 shape: (2, 1)
┌────────┐
│ amount │
├────────┤
│ 75     │
│ 50     │
└────────┘
```
