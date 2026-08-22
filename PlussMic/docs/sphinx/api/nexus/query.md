<!-- pluss-hand-authored -->

# Nexus::query

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::query() const -> NexusQuery
Nexus::query(DataFrame::SqlOptions options) const -> NexusQuery
```

## What It Does

Returns the workspace SQL accessor. This is the bundle-side counterpart to
[`DataFrame::sql()`](../dataframe/sql.md): instead of querying one calling
frame, it queries over the named tables already loaded into the `Nexus`
workspace.

Use `query()` when you want SQL-style `SELECT ... FROM ... JOIN ...` over the
workspace. Use [`from()`](from.md) when you want relationship-graph traversal
without SQL.

## Example

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
    {"name", pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto nx = Nexus({{"customers", customers}});
auto out = nx.query().execute("SELECT name FROM customers");
std::cout << out.ncases() << " " << out.nfeatures();
```

### Output

```text
3 1
```
