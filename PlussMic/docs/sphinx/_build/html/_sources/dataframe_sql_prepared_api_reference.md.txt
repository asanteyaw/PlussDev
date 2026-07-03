# DataFrameSqlPrepared API Reference

# Practical DataFrameSqlPrepared API Reference

## API Reference Style

This page documents the public prepared-statement helper returned by `DataFrame::prepare_sql()` or `DataFrameSqlAccessor::prepare()`.

## E

### `execute`

#### API Signatures

```cpp
DataFrameSqlPrepared::execute() -> DataFrame

DataFrameSqlPrepared::execute(params: std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam>) -> DataFrame

DataFrameSqlPrepared::execute(params: std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam>, identifier_params: std::unordered_map<std::string, std::string>) -> DataFrame
```

#### Notes

Runs the prepared SQL statement with the supplied parameters and returns the result as a new `DataFrame`.

#### Inputs

`params` : `std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam>`
    Named scalar bindings supplied to placeholders such as `{min_sales}` inside the prepared statement template.

`identifier_params` : `std::unordered_map<std::string, std::string>`
    Named identifier bindings used for placeholder substitution when a statement needs a dynamic SQL identifier such as a table alias.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Run the prepared SQL statement with no external parameter bindings.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto stmt = df.prepare_sql("SELECT id, sales FROM self WHERE sales > 95");

auto out = stmt.execute();
```

##### Expected output

```text
id  sales
1   100.0
2   120.0
```
Example 2. Run the prepared SQL statement while binding named scalar placeholders.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto stmt = df.prepare_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");

auto out = stmt.execute({{"min_sales", 95.0}});
```

##### Expected output

```text
id  sales
1   100.0
2   120.0
```
Example 3. Run the prepared SQL statement while binding both scalar placeholders and SQL identifier placeholders.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto stmt = df.prepare_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");

auto out = stmt.execute({{"min_sales", 95.0}}, {{"table_name", "self"}});
```

##### Expected output

```text
id  sales
1   100.0
2   120.0
```
