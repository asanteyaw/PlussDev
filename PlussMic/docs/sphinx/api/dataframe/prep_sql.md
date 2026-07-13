# DataFrame::prep_sql

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::prep_sql(std::string_view statement) -> DataFrameSqlPrepared

DataFrame::prep_sql(std::string_view statement, SqlOptions options) -> DataFrameSqlPrepared
```

## What It Does

Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.

## Parameters

**statement**
    SQL statement text to prepare against the current frame.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`DataFrameSqlPrepared`
    Value produced by this call.

## Examples

Example 1. Prepare a parameterized SQL statement with default SQL options.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto stmt = df.prep_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");
```

### Output

```text
SQL accessor / prepared statement created
```
Example 2. Prepare a parameterized SQL statement with an explicit SQL options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::table::DataFrame::SqlOptions options;
auto stmt = df.prep_sql("SELECT id, sales FROM self WHERE sales > {min_sales}", options);
```

### Output

```text
SQL accessor / prepared statement created
```


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
