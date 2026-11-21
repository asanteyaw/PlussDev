<!-- pluss-hand-authored -->

# DataFrameSqlAccessor::prepare

[Back to DataFrameSqlAccessor API Reference](../../dataframe_sql_accessor_api_reference)

## Signature

```cpp
DataFrameSqlPrepared DataFrameSqlAccessor::prepare(std::string_view statement) const
```

## What It Does

Stores a SQL statement that may contain `{name}` value placeholders (and `{@name}` identifier placeholders),
together with the frame and options, and returns a [`DataFrameSqlPrepared`](../../dataframe_sql_prepared_api_reference)
you can run again and again with different bindings.

Preparation does not check the statement. A misspelt column or table is reported by the first
[`execute`](../dataframe-sql-prepared/execute.md), not by `prepare`.

## Parameters

**statement** (`std::string_view`)

The SQL text. Refer to the frame as `df`. Write `{min_sales}` where a value is bound later, and `{@col}` where a
column or table name is bound later.

## Returns

`DataFrameSqlPrepared`
    A reusable statement. Each `execute(...)` builds a fresh in-memory workspace and returns a new `DataFrame`.

## Examples

Example 1. Prepare once, run with two different values.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Shelf(pluss::make_series({1, 2, 3}))},
    {"sales", Shelf(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Shelf(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto stmt = orders.sql().prepare("SELECT id, sales FROM df WHERE sales > {min_sales}");

std::cout << stmt.execute({{"min_sales", 95.0}});
std::cout << stmt.execute({{"min_sales", 110.0}});
```

### Output

```text
==================
     | id    sales
------------------
0    | 1     100  
1    | 2     120  
------------------
[2 cs x 2 feat]==================
     | id    sales
------------------
0    | 2     120  
------------------
[1 cs x 2 feat]
```

Example 2. Preparation does not validate the statement.

```cpp
auto stmt = orders.sql().prepare("SELECT nope FROM df WHERE sales > {m}");
std::cout << "prepared\n";

try {
  auto out = stmt.execute({{"m", 1.0}});
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
prepared
DataFrame::sql().execute: sqlite prepare failed: no such column: nope
```

## Notes

- `DataFrame::prep_sql(statement)` is the same call without the intermediate `sql()`.
- Only the bound values change between executions. The statement text is fixed.

## See Also

- [`DataFrameSqlPrepared::execute`](../dataframe-sql-prepared/execute.md): run the prepared statement.
- [`DataFrameSqlAccessor::execute`](execute.md): run a statement once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="execute.html">
    <span class="label">Previous</span>
    <span class="title">execute</span>
  </a>
</div>
