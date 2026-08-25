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

Unlike the older root-table-proxy behavior, `query()` no longer requires you to
anchor the statement to one special workspace table first. Any named Nexus table can
appear in `FROM`, `JOIN`, CTEs, or subqueries, and the whole statement runs through
the same in-memory SQLite backend that powers [`DataFrame::sql()`](../dataframe/sql.md).

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

Because this is now a genuine workspace SQL surface, richer statements are also
valid:

```cpp
auto out = nx.query().execute(
    "WITH named AS (SELECT customer_id, name FROM customers) "
    "SELECT DISTINCT name FROM named ORDER BY name");
```
