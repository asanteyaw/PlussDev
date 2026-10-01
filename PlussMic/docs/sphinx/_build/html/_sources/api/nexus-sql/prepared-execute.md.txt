<!-- pluss-hand-authored -->

# NexusQueryPrepared::execute

[Back to NexusQuery API Reference](../../nexus_query_api_reference)

## Signature

```cpp
NexusQueryPrepared::execute() const -> DataFrame
NexusQueryPrepared::execute(
    std::unordered_map<std::string, pluss::table::DataFrame::SqlOptions::SqlParam> params) const -> DataFrame
NexusQueryPrepared::execute(
    std::unordered_map<std::string, pluss::table::DataFrame::SqlOptions::SqlParam> params,
    std::unordered_map<std::string, std::string> identifier_params) const -> DataFrame
```

## What It Does

Executes the statement template a `NexusQueryPrepared` was built with (via
[`NexusQuery::prepare(statement)`](prepare.md)), substituting placeholders
into it first when params are supplied. The whole point of preparing a
statement rather than calling [`NexusQuery::execute`](execute.md) directly
is to run the **same** statement text repeatedly against **different**
bound values — a filter threshold that changes per call, a column name
chosen at runtime — without rebuilding the SQL string by hand each time.

There are two independent kinds of placeholder in the statement template,
and two matching parameter maps:

- **`{name}`** — a **value** placeholder, bound from `params`. The bound
  value is substituted as a literal (quoted for strings, unquoted for
  numbers/booleans, `null` for `SqlParam`'s `std::nullptr_t` alternative) —
  never string-concatenated into the statement, so there is no SQL
  injection surface through this path.
- **`{@name}`** — an **identifier** placeholder, bound from
  `identifier_params`, substituted as a backtick-quoted identifier (a
  column or table name) rather than a value. Column names can't be bound as
  ordinary SQL parameters (`WHERE {col} = 1` isn't valid SQL for a bound
  value), so identifier substitution is a separate, explicit mechanism —
  see [`Notes`](#notes) for why this still isn't a free-text injection
  hole.
- **`{{`** — an escaped literal `{`, for a statement that needs a literal
  brace character.

The no-argument overload re-executes the template with no substitution at
all — useful when the template has no placeholders, or when every
placeholder was already baked in at `prepare()` time via
[`NexusQuery::query(options)`](../nexus_api_reference.md)'s own options.

## Parameters

**params** (`std::unordered_map<std::string, SqlParam>`, `SqlParam` =
`std::variant<std::nullptr_t, bool, std::int64_t, double, std::string>`)
    Value bindings for every `{name}` placeholder in the template. A
    placeholder with no matching key throws (unless the statement's
    `SqlOptions::missing_param_policy` was set to substitute `null`).

**identifier_params** (`std::unordered_map<std::string, std::string>`)
    Identifier bindings for every `{@name}` placeholder in the template. A
    placeholder with no matching key throws under the same policy rule as
    `params`.

## Returns

`DataFrame`
    The query result, same as [`NexusQuery::execute`](execute.md) — one row
    per matched record, one column per selected expression.

## Examples

Example 1. No placeholders — call `.execute()` as many times as needed.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

// nx: a workspace with a "customers" table (customer_id, region_id).
auto stmt = nx.query().prepare("SELECT customer_id FROM customers ORDER BY customer_id");
auto out = stmt.execute();
std::cout << out.to_string();
```

### Output

```text
     | customer_id
------------------
0    | 1
1    | 2
2    | 3
------------------
[3 cs x 1 feat]
```

Example 2. The same template, run twice with different value bindings — the
statement text is parsed and prepared once, then reused.

```cpp
auto stmt = nx.query().prepare(
    "SELECT customer_id, region_id FROM customers WHERE region_id = {rid} ORDER BY customer_id");

auto west = stmt.execute({{"rid", 10.0}});
auto east = stmt.execute({{"rid", 20.0}});

std::cout << "region 10: " << west.ncases() << " rows\n";
std::cout << "region 20: " << east.ncases() << " rows\n";
std::cout << west.to_string();
```

### Output

```text
region 10: 2 rows
region 20: 1 rows
     | customer_id  region_id
-----------------------------
0    | 1            10
1    | 3            10
-----------------------------
[2 cs x 2 feat]
```

Example 3. An identifier placeholder — the column to select is itself a
runtime choice, bound via `identifier_params` rather than `params`.

```cpp
auto stmt = nx.query().prepare("SELECT {@col} FROM customers ORDER BY customer_id");
auto out = stmt.execute({}, {{"col", "customer_id"}});
std::cout << out.features();
```

### Output

```text
customer_id
```

## Notes

- `{name}` value substitution is never plain string interpolation — it
  binds through the same parameterization `DataFrame::sql()` itself uses,
  so a `params` value containing SQL metacharacters is treated as inert
  literal data, not executable SQL.
- `{@name}` identifier substitution is validated before use (rejecting
  characters that aren't valid in a bare SQL identifier) and always
  emitted backtick-quoted — it is a controlled identifier allow-list
  mechanism, not free-text splicing, even though its output position in the
  statement (a column/table name slot) can't go through the same
  value-parameterization path `{name}` uses.
- `.prepare(...)`'s own template parsing already validates brace balance
  and rejects an empty `{}`/`{@}` placeholder — see
  [`NexusQuery::prepare`](prepare.md).

## See Also

- [`NexusQuery::prepare`](prepare.md): builds the `NexusQueryPrepared` this
  method executes.
- [`NexusQuery::execute`](execute.md): the one-shot equivalent, for a
  statement that only runs once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prepare.html">
    <span class="label">Previous</span>
    <span class="title">prepare</span>
  </a>
  <a class="next-link" href="../../nexus_query_api_reference.html">
    <span class="label">Next</span>
    <span class="title">NexusQuery API Reference</span>
  </a>
</div>
