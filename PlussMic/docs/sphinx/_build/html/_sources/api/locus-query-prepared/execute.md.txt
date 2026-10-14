<!-- pluss-hand-authored -->

# LocusQueryPrepared::execute

[Back to LocusQueryPrepared API Reference](../../locus_query_prepared_api_reference)

## Signatures

```cpp
pluss::kindle::Catalog LocusQueryPrepared::execute() const
pluss::kindle::Catalog LocusQueryPrepared::execute(Params params) const
pluss::kindle::Catalog LocusQueryPrepared::execute(
    Params params,
    std::unordered_map<std::string, std::string> identifier_params) const
```

`Params` is:

```cpp
using Params = std::unordered_map<
    std::string,
    std::variant<std::nullptr_t, bool, std::int64_t, double, std::string>>;
```

## What It Does

`execute()` materializes the statement template retained by `prepare()`. The
no-argument overload is for templates with no placeholders or for placeholders
already supplied through `Locus::query(options)`. The second overload merges
runtime value bindings. The third merges value and identifier bindings.

Runtime maps are copied into the captured options and replace defaults with the
same key; defaults not mentioned by the call remain available. The prepared
object itself is unchanged, so one instance can be executed repeatedly with
different maps.

Value and identifier placeholders are deliberately separate:

- `{name}` accepts `nullptr`, `bool`, `std::int64_t`, finite/non-finite `double`, or `std::string`.
- `{@name}` accepts a simple identifier such as a catalog or stream key and is emitted quoted.
- A string value is escaped as data, not inserted as SQL syntax.
- A non-finite `double` and `nullptr` become SQL `NULL`.

## Parameters

**params** (`Params`)
: Runtime value bindings. Every supplied value is rendered according to its
  variant alternative. When `reject_unused_params` is enabled, extra keys fail.

**identifier_params** (`std::unordered_map<std::string, std::string>`)
: Runtime table or stream identifiers. Values are validated as simple
  identifiers before being quoted; SQL fragments are rejected.

## Returns

`pluss::kindle::Catalog`
: A newly materialized query result. Execution never mutates the prepared object
  or the source Locus.

## Examples

Example 1. The no-argument overload runs a template without placeholders.

```cpp
const auto fixed = data.query().prepare(
    "SELECT order_id FROM orders ORDER BY order_id DESC LIMIT 2");
print_numbers(fixed.execute(), "order_id");
```

### Output

```text
order_id=[103, 102]
```

Example 2. Reuse one value template without changing the prepared object.

```cpp
const auto threshold = data.query().prepare(
    "SELECT order_id FROM orders WHERE amount >= {floor} ORDER BY order_id");
print_numbers(threshold.execute({{"floor", 50.0}}), "order_id");
print_numbers(threshold.execute({{"floor", 80.0}}), "order_id");
```

### Output

```text
order_id=[100, 101, 103]
order_id=[103]
```

Example 3. One call can bind every value alternative. `true` participates as a
SQL predicate, `nullptr` supplies `NULL`, and the integer controls `LIMIT`.

```cpp
const auto kinds = data.query().prepare(
    "SELECT order_id FROM orders "
    "WHERE state = {state} AND amount >= {floor} AND {enabled} "
    "AND {nothing} IS NULL ORDER BY order_id LIMIT {limit}");
SqlOptions::SqlParam null_value = nullptr;
print_numbers(kinds.execute({
    {"state", std::string("paid")},
    {"floor", 40.0},
    {"enabled", true},
    {"nothing", null_value},
    {"limit", std::int64_t{3}},
}), "order_id");
```

### Output

```text
order_id=[100, 101]
```

Example 4. Bind a projected stream name through the identifier map, not the
value map.

```cpp
const auto projected = data.query().prepare(
    "SELECT {@field} FROM orders ORDER BY order_id LIMIT 2");
const auto amounts = projected.execute({}, {{"field", "amount"}});
print_keys(amounts);
print_numbers(amounts, "amount");
```

### Output

```text
keys=[amount] rows=2
amount=[50, 75]
```

Example 5. Runtime values override captured defaults only for that execution.

```cpp
SqlOptions defaults;
defaults.params["floor"] = 80.0;
const auto inherited = data.query(defaults).prepare(
    "SELECT order_id FROM orders WHERE amount >= {floor} ORDER BY order_id");
print_numbers(inherited.execute(), "order_id");
print_numbers(inherited.execute({{"floor", 50.0}}), "order_id");
```

### Output

```text
order_id=[103]
order_id=[100, 101, 103]
```

Example 6. An unresolved value placeholder fails before SQL compilation.

```cpp
print_error("missing", [&] { (void)threshold.execute(); });
```

### Output

```text
missing: DataFrame::sql().execute: SQL params missing key 'floor'
```

Example 7. Identifier bindings reject SQL fragments rather than treating them as
column names.

```cpp
print_error("identifier", [&] {
  (void)projected.execute({}, {{"field", "amount; DROP TABLE orders"}});
});
```

### Output

```text
identifier: DataFrame::sql().execute: identifier param 'field' has invalid character
```

## Validation And Cost

Binding validation precedes database execution. Each call copies the captured
options, merges its maps, builds the transient SQL workspace, compiles the
rendered statement, and allocates a result Catalog. The prepared object is safe
to reuse sequentially; its source workspace is immutable, while external
`SourceBinding` pointers remain the caller's lifetime responsibility.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../locus_query_prepared_api_reference.html"><span class="label">Previous</span><span class="title">LocusQueryPrepared API Reference</span></a>
  <a class="next-link" href="../../locus_query_prepared_api_reference.html"><span class="label">Next</span><span class="title">LocusQueryPrepared API Reference</span></a>
</div>

