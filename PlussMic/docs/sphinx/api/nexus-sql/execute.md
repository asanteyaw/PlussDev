<!-- pluss-hand-authored -->

# NexusQuery::execute

[Back to NexusQuery API Reference](../../nexus_query_api_reference)

## Signature

```cpp
DataFrame NexusQuery::execute(std::string_view statement) const
```

## What It Does

Executes one SQL statement against the tables loaded into the `Nexus`
workspace and returns the materialized result as a `DataFrame`. This is the
one-shot entry point — for a statement that runs more than once with
different bound values, [`.prepare(statement)`](prepare.md) is the better
fit, since it parses the statement once and lets later calls only supply
new parameter values.

`FROM`/`JOIN` must name real workspace tables (`customers`, `orders`, and so
on, matching whatever names the `Nexus` was constructed with), but there is
no special root-table requirement the way [`pull()`](../nexus/pull.md)'s
root-preserving path rule has — any workspace table can appear in `FROM`, a
`JOIN`, a common table expression, or a subquery, and ordinary grouped
queries, CTEs, and multi-way joins are valid as long as the statement
produces a result set.

## Parameters

**statement** (`std::string_view`)
    A SQL `SELECT` statement (or a statement shape that produces a result
    set, such as a `WITH ... SELECT`). Table names inside it refer to the
    workspace's own table names.

## Returns

`DataFrame`
    One row per matched record, one column per selected expression. A
    qualified `SELECT table.feature` keeps the `table.feature` spelling as
    the output column name — see Example 1.

## Examples

Example 1. A join across two workspace tables.

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
auto nx = Nexus({{"customers", customers}, {"orders", orders}});

auto out = nx.query().execute(
    "SELECT customers.customer_id, orders.amount "
    "FROM customers "
    "JOIN orders ON customers.customer_id = orders.customer_id "
    "ORDER BY orders.amount DESC");

std::cout << out.to_string();
```

### Output

```text
     | customers.customer_id  orders.amount
-------------------------------------------
0    | 2                      75
1    | 1                      50
2    | 2                      20
-------------------------------------------
[3 cs x 2 feat]
```

Example 2. A statement that matches nothing still returns a defined,
zero-row `DataFrame` rather than throwing.

```cpp
auto out = nx.query().execute("SELECT customer_id FROM customers WHERE customer_id > 100");
std::cout << out.ncases() << " rows";
```

### Output

```text
0 rows
```

Example 3. Malformed SQL throws with the underlying SQLite error message.

```cpp
try {
  auto out = nx.query().execute("SELEKT customer_id FROM customers");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
DataFrame::sql().execute: sqlite prepare failed: near "SELEKT": syntax error
```

## Notes

- `execute()` parses and runs `statement` fresh every call — for a
  statement reused with different bound values, prefer
  [`.prepare(statement)`](prepare.md) once and call `.execute(params)` on
  the result, rather than re-interpolating a new statement string by hand
  on every call.
- Column names in the result mirror the statement's own `SELECT` list
  verbatim — alias with `AS` in the statement if a table-qualified name
  like `customers.customer_id` isn't the column name you want.

## See Also

- [`NexusQuery::prepare`](prepare.md): build a reusable statement template
  instead of executing once.
- [`Nexus::query`](../../nexus_api_reference.md): the entry point that returns
  this `NexusQuery`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../nexus_query_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">NexusQuery API Reference</span>
  </a>
  <a class="next-link" href="prepare.html">
    <span class="label">Next</span>
    <span class="title">prepare</span>
  </a>
</div>
