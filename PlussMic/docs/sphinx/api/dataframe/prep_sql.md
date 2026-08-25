<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b66f6384bebd -->

# DataFrame::prep_sql

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::prep_sql(std::string_view statement) const -> DataFrameSqlPrepared
DataFrame::prep_sql(std::string_view statement, SqlOptions options) const -> DataFrameSqlPrepared
```

## What It Does

Stores a SQL statement template — one that may contain `{param_name}` placeholders —
together with the current frame and `options`, returning a `DataFrameSqlPrepared`
object you can `.execute(...)` repeatedly with different parameter bindings, without
re-typing (or re-templating) the statement text each time. This is the reusable-query
counterpart to [`sql().execute(statement)`](sql.md), which prepares a fresh in-memory
SQLite workspace and runs the statement in one shot.

```{note} A pure convenience wrapper, nothing more
`prep_sql(statement)` is exactly equivalent to `sql().prepare(statement)`, and
`prep_sql(statement, options)` is exactly equivalent to `sql(options).prepare(statement)` —
both overloads just construct a `DataFrameSqlAccessor` and immediately call
`.prepare(statement)` on it. There is no behavior here beyond skipping that
intermediate `.sql()` call; every parameter, option, and gotcha documented on
[`DataFrameSqlAccessor::prepare`](../../dataframe_sql_accessor_api_reference)
and [`DataFrameSqlPrepared::execute`](../../dataframe_sql_prepared_api_reference)
applies identically here.
```

```{note}
`prep_sql()` itself does not validate the statement — no SQL preparation happens until
the returned value's `.execute(...)` is actually called. What it saves you is repeated
statement templating at the call site, not execution work: every `.execute(...)` still
builds a fresh in-memory SQLite workspace for the current frame and options.
```

## Parameters

**statement** (`std::string_view`)

The SQL statement template, using `{param_name}` for value placeholders (and
`{@ident_name}` for identifier placeholders) that get bound at `.execute(...)` time.

**options** (`SqlOptions`, optional)

The same execution options [`sql()`](sql.md) accepts — additional joinable sources,
default parameter bindings, missing-parameter policy, row/memory/timeout limits, and
diagnostics toggles. Omitted params can still be supplied later at `.execute(...)`
time.

## Returns

`DataFrameSqlPrepared`
    A prepared statement bound to the current frame, `options`, and `statement`.
    Call `.execute()` (using only whatever params were already in `options`) or
    `.execute(params)` / `.execute(params, identifier_params)` to bind values and
    run the query, returning a fresh `DataFrame` result each time.

## Examples

Example 1. Prepare a parameterized statement once, then execute it with two
different parameter bindings.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto stmt = orders.prep_sql("SELECT id, sales FROM df WHERE sales > {min_sales}");

auto low_bar = stmt.execute({{"min_sales", 95.0}});
std::cout << low_bar;
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

```cpp
auto high_bar = stmt.execute({{"min_sales", 110.0}});
std::cout << high_bar;
```

### Output

```text
==================
     | id    sales
------------------
1    | 2     120  
------------------
[1 cs x 2 feat]
```

`stmt`'s text is fixed once `prep_sql()` is called — only the bound `min_sales`
*value* changes between the two `.execute({...})` calls, each of which renders the
template fresh and re-runs the `WHERE` filter. Case `2`'s row survives both times
(`120` is greater than both `95` and `110`), while case `0`'s row (`sales = 100`) only
survives the looser `95` threshold.

## Notes

- The statement text itself is fixed once `prep_sql()` is called; only the bound
  parameter *values* can change between `.execute(...)` calls, not the SQL text.
- `.execute()` (no arguments) re-runs the statement using only whatever `params`
  were already set in `options` at `prep_sql()` time.
- Prefer `prep_sql()` over `sql().execute(statement)` when the same query *text*
  needs to be reused across several call sites with different bindings — it saves
  repeated statement templating and keeps the binding site smaller (see the
  [`DataFrameSqlPrepared` reference](../../dataframe_sql_prepared_api_reference)
  for the full explanation).

## See Also

- [`DataFrame::sql`](sql.md): parse and execute a SQL statement in one call, for
  one-off queries.
- [`DataFrame::query`](query.md): a lighter-weight boolean-expression filter, for
  when a full SQL statement is more machinery than the task needs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pow.html">
    <span class="label">Previous</span>
    <span class="title">pow</span>
  </a>
  <a class="next-link" href="prod.html">
    <span class="label">Next</span>
    <span class="title">prod</span>
  </a>
</div>
