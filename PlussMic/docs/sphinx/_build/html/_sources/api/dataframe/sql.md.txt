# DataFrame::sql

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::sql() -> DataFrameSqlAccessor

DataFrame::sql(SqlOptions options) -> DataFrameSqlAccessor
```

## What It Does

Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.

## Parameters

**options**
    SQL execution options such as engine or binding policy for the returned SQL accessor.

## Returns

`DataFrameSqlAccessor`
    Value produced by this call.

## Examples

Example 1. Create the SQL accessor with default options and run a query immediately.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sql().execute("SELECT id, sales FROM self WHERE sales > 95");
```

### Output

```text
id  sales
1   100.0
2   120.0
```
Example 2. Create the SQL accessor with an explicit SQL options object before running a query.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::table::DataFrame::SqlOptions options;
auto sql = df.sql(options);
auto out = sql.execute("SELECT id, sales FROM self WHERE sales > 95");
```

### Output

```text
id  sales
1   100.0
2   120.0
```

## Practical Behavior

- `sql()` returns the SQL accessor; it does not execute a statement by itself.
- Call `.execute(...)` on the accessor when you want a result frame immediately.
- Use `prepare_sql(...)` when the same query will be executed repeatedly with changing bindings.



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
