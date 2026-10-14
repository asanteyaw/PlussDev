<!-- pluss-hand-authored -->

# LocusQuery::execute

[Back to LocusQuery API Reference](../../locus_query_api_reference)

## Signature

```cpp
pluss::kindle::Catalog LocusQuery::execute(std::string_view statement) const
```

## What It Does

`execute()` runs one SQL statement against the named catalogs captured by the
accessor. Catalog keys are SQL table names. Selected expressions become streams
in a new `Catalog`, and SQL row order becomes the common record order of those
streams.

This is the direct path for statement text used once. Every call renders option
parameters, creates an in-memory SQL workspace from the aligned frames, compiles
the statement, consumes its rows, and converts the rectangular result back to a
Catalog. For repeated template text, [`prepare()`](prepare.md) centralizes the
template and bindings, although execution still compiles against a fresh
in-memory workspace.

SQL may join any exposed catalog; it is not constrained by Locus's declared link
graph. A SQL join therefore follows the `ON` clause written in the statement and
does not enforce `RelationshipCardinality`. Use Locus relational methods when
declared-link validation and ragged join reporting are part of the contract.

## Parameter

**statement** (`std::string_view`)
: A result-producing SQL statement. `SELECT` and `WITH ... SELECT` are the usual
  forms. Workspace catalog keys and stream keys are identifiers; use `AS` to
  control result stream names.

## Returns

`pluss::kindle::Catalog`
: A defined catalog with one stream per projected expression. A zero-row result
  remains defined and retains its projected stream keys.

## Examples

All outputs below are emitted by
`PlussMic/examples/bundle/locus_query_reference_demo.cpp`.

Example 1. Filter and order one catalog. The result order is the SQL `ORDER BY`
order, not the source record order.

```cpp
const auto filtered = data.query().execute(
    "SELECT order_id, amount FROM orders WHERE amount >= 50 ORDER BY amount DESC");
print_numbers(filtered, "order_id");
print_numbers(filtered, "amount");
```

### Output

```text
order_id=[103, 101, 100]
amount=[90, 75, 50]
```

Example 2. Join catalog tables directly and alias projected names.

```cpp
const auto joined = data.query().execute(
    "SELECT customers.name AS customer, orders.amount AS amount "
    "FROM customers JOIN orders ON customers.customer_id = orders.customer_id "
    "WHERE orders.state = 'paid' ORDER BY orders.amount DESC");
print_words(joined, "customer");
print_numbers(joined, "amount");
```

### Output

```text
customer=[Bola, Ada]
amount=[75, 50]
```

Example 3. No matches produce a zero-row Catalog with the projected key intact.

```cpp
print_keys(data.query().execute(
    "SELECT order_id FROM orders WHERE amount > 1000"));
```

### Output

```text
keys=[order_id] rows=0
```

Example 4. SQL syntax errors surface at execution with the engine diagnostic.

```cpp
print_error("malformed", [&] {
  (void)data.query().execute("SELEKT order_id FROM orders");
});
```

### Output

```text
malformed: DataFrame::sql().execute: sqlite prepare failed: near "SELEKT": syntax error
```

## Failure Modes

- Empty or malformed SQL fails before a result Catalog is built.
- Unknown table or stream identifiers fail during SQL compilation.
- Missing template parameters, unsafe identifier bindings, negative limits, and
  invalid memory or timeout options fail before or during execution.
- A borrowed `SqlOptions::SourceBinding::frame` must remain alive through the call.
- Duplicate output names follow `collision_policy`; the default is to reject them.

## Cost

Execution is eager. It loads each exposed aligned frame into a transient in-memory
database, runs the statement, and allocates a fresh Catalog. Use relational Locus
operations for graph-aware joins that can preserve ragged structure without
materializing every table into SQL.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../locus_query_api_reference.html"><span class="label">Previous</span><span class="title">LocusQuery API Reference</span></a>
  <a class="next-link" href="prepare.html"><span class="label">Next</span><span class="title">prepare</span></a>
</div>

