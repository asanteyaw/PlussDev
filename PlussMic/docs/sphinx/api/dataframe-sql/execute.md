<!-- pluss-hand-authored -->

# DataFrameSqlAccessor::execute

[Back to DataFrameSqlAccessor API Reference](../../dataframe_sql_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameSqlAccessor::execute(std::string_view statement) const
```

## What It Does

Runs one SQL statement against the frame and returns the result as a new `DataFrame`. The frame is exposed to
the statement as the table `df`, and the statement runs in a fresh in-memory SQLite workspace each time, so
nothing is kept between calls.

## Parameters

**statement** (`std::string_view`)

The SQL text. Refer to the frame as `df`. The statement may not contain `{name}` placeholders. Use
[`prepare`](prepare.md) for those.

## Returns

`DataFrame`
    The rows the statement produced, with the column names the statement gave them.

## Examples

Example 1. Filter rows and choose columns.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Shelf(pluss::make_series({1, 2, 3}))},
    {"sales", Shelf(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Shelf(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::cout << orders.sql().execute("SELECT id, sales FROM df WHERE sales > 95");
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

Example 2. Aggregate.

```cpp
std::cout << orders.sql().execute("SELECT COUNT(*) AS n, SUM(sales) AS total FROM df");
```

### Output

```text
==================
     | n     total
------------------
0    | 3     310  
------------------
[1 cs x 2 feat]
```

Example 3. Sort.

```cpp
std::cout << orders.sql().execute("SELECT city, sales FROM df ORDER BY sales DESC");
```

### Output

```text
====================
     | city    sales
--------------------
0    | Tema    120  
1    | Accra   100  
2    | Kumasi  90   
--------------------
[3 cs x 2 feat]
```

Example 4. A statement that names a column that does not exist throws.

```cpp
try {
  auto out = orders.sql().execute("SELECT nope FROM df");
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
DataFrame::sql().execute: sqlite prepare failed: no such column: nope
```

## Notes

- Use `execute` for a statement you run once. When the same statement shape runs many times with different
  values, [`prepare`](prepare.md) keeps the call site small.
- [`DataFrame::sql`](../dataframe/sql.md) takes `SqlOptions` for extra joinable sources, default parameters and
  limits.

## See Also

- [`DataFrame::sql`](../dataframe/sql.md): enters the accessor.
- [`DataFrameSqlAccessor::prepare`](prepare.md): a reusable statement with `{name}` placeholders.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="prepare.html">
    <span class="label">Next</span>
    <span class="title">prepare</span>
  </a>
</div>
