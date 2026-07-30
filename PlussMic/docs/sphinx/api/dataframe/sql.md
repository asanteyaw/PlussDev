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

Enters the frame's built-in SQL accessor — a small, self-contained SQL engine that
runs `SELECT` statements directly against the frame (referenced in the statement as
the source table `df`), returning an ordinary `DataFrame` result. Use this when a
transformation reads more naturally as a `SELECT`/`WHERE`/`JOIN`/`ORDER BY` statement
than as a chained sequence of table verbs. `sql()` itself does no work beyond binding
the frame (and, optionally, execution `options`); the real work happens on
`.execute(statement)`.

[`prep_sql(statement)`](prep_sql.md) is the sibling entry point for a statement
that will run many times with different bound parameter values — it parses the
statement once up front, rather than on every `.execute()` call the way
`sql().execute(statement)` does each time it's called fresh.

See the [DataFrameSqlAccessor API Reference](../../dataframe_sql_accessor_api_reference)
for `.execute()`'s full parameter and behavior details.

## Parameters

**options** (`SqlOptions`, optional)

Execution options for the returned accessor: additional joinable `sources`, default
`params`/`identifier_params` for `{...}`-style placeholders, missing-parameter and
name-collision policies, row/memory/timeout limits, and diagnostics toggles. Omit it
entirely to use every default.

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

The `WHERE` clause behaves like [`query()`](query.md)'s filtering — case `2`
(`sales = 90`) doesn't satisfy `sales > 95`, so it's dropped, while cases `0` and `1`
survive with their original case labels intact. The `SELECT id, sales` projection
means `city` never appears in the result at all.

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
1    | 2     120    Tema
------------------------
[1 cs x 3 feat]
```

`{target_city}` is substituted with the quoted string `'Tema'` before the statement
runs, matching only case `1`. `SELECT *` keeps every column this time, unlike Example
1's explicit projection.

## Notes

- `sql()` returns the accessor only; it never executes a statement by itself — you
  must call `.execute(statement)` to get a result.
- The calling frame is always available in the statement as the source table
  `df`; `options.sources` can bind additional named frames for joins.
- Use [`prep_sql(statement)`](prep_sql.md) instead when the same statement text runs
  repeatedly with different parameter bindings, to avoid re-parsing it every time.

## See Also

- [`DataFrame::prep_sql`](prep_sql.md): the reusable, parse-once counterpart for a
  statement executed many times with changing parameters.
- [`DataFrame::query`](query.md): a lighter-weight boolean-expression filter, for
  when a full SQL statement is more machinery than the task needs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort_values.html">
    <span class="label">Previous</span>
    <span class="title">sort_values</span>
  </a>
  <a class="next-link" href="stack.html">
    <span class="label">Next</span>
    <span class="title">stack</span>
  </a>
</div>
