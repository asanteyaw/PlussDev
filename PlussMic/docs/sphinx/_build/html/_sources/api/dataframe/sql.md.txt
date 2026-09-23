<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6085bd8c12e2 -->

# DataFrame::sql

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::sql() const -> DataFrameSqlAccessor
DataFrame::sql(SqlOptions options) const -> DataFrameSqlAccessor
```

## What It Does

Enters the frame's built-in SQL accessor. The accessor binds the current frame and
optional extra sources, loads them into an in-memory SQLite workspace, and executes
the SQL statement there before converting the result back into a `DataFrame`.

That makes `sql()` much broader than the old parser-only implementation: ordinary
`SELECT` queries, aliases, expressions, `GROUP BY`, `HAVING`, common table
expressions (`WITH`), subqueries, `LEFT JOIN`, `DISTINCT`, and similar SQLite query
shapes now work without PlussMic having to special-case each clause itself.

`sql()` itself still does no work beyond binding the frame and options; the real work
happens on `.execute(statement)`.

[`prep_sql(statement)`](prep_sql.md) is the sibling entry point for a statement
that will run many times with different bound parameter values.

See the [DataFrameSqlAccessor API Reference](../../dataframe_sql_accessor_api_reference)
for `.execute()`'s full parameter and behavior details.

## Parameters

**options** (`SqlOptions`, optional)

Execution options for the returned accessor: additional joinable `sources`,
`params`/`identifier_params` for `{...}`-style placeholders, missing-parameter and
name-collision policies, result/memory/timeout guards, and diagnostics toggles.
Omit it entirely to use every default.

## Returns

`DataFrameSqlAccessor`
    An accessor bound to the current frame (and, if given, `options`). Call
    `.execute(statement)` on it to parse and run a `SELECT` statement, returning a new
    `DataFrame`.

## Examples

Example 1. Run a filtering `SELECT` statement with the default options.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto high_sales = orders.sql().execute("SELECT id, sales FROM df WHERE sales > 95");
std::cout << high_sales;
```

### Output

```text
==================
     | id    sales
------------------
0    | 1     100  
1    | 2     120  
------------------
[2 cs x 2 feat]
```

The `SELECT id, sales` projection means `city` never appears in the result at all.

Example 2. Pass an `{param}` placeholder value through `SqlOptions`.

```cpp
DataFrame::SqlOptions options;
options.params["target_city"] = std::string("Tema");

auto matched = orders.sql(options).execute("SELECT * FROM df WHERE city = {target_city}");
std::cout << matched;
```

### Output

```text
========================
     | id    sales  city
------------------------
0    | 2     120    Tema
------------------------
[1 cs x 3 feat]
```

`{target_city}` is substituted with the quoted string `'Tema'` before the statement
runs, matching only case `1`. `SELECT *` keeps every column this time, unlike Example
1's explicit projection.

Example 3. Use grouped SQL directly instead of spelling the aggregation as chained
table verbs.

```cpp
auto grouped = orders.sql().execute(
    "SELECT city, COUNT(*) AS n, AVG(sales) AS mean_sales "
    "FROM df "
    "GROUP BY city "
    "ORDER BY mean_sales DESC");
std::cout << grouped;
```

### Output

```text
==============================
     | city    n      mean_sales
------------------------------
0    | Tema    1      120
1    | Accra   1      100
2    | Kumasi  1      90
------------------------------
[3 cs x 3 feat]
```

This is the main reason to use `sql()`: when the operation reads more clearly as SQL
than as a long sequence of C++ table verbs, you can just write the query.

## Notes

- `sql()` returns the accessor only; it never executes a statement by itself — you
  must call `.execute(statement)` to get a result.
- The calling frame is always available in the statement as the source table
  `df`; `options.sources` can bind additional named frames for joins.
- Bare source-name shorthand is accepted: `df.sql().execute("df")` is treated like
  `SELECT * FROM df`, similar in spirit to pandas' `read_sql(table_name, ...)`.
- Statements still need to produce a result set. This surface is query-oriented, not
  a mutation API for arbitrary DDL/DML workflows.

## See Also

- [`DataFrame::prep_sql`](prep_sql.md): the reusable, parse-once counterpart for a
  statement executed many times with changing parameters.
- [`DataFrame::query`](query.md): a lighter-weight boolean-expression filter, for
  when a full SQL statement is more machinery than the task needs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="stack.html">
    <span class="label">Next</span>
    <span class="title">stack</span>
  </a>
</div>
