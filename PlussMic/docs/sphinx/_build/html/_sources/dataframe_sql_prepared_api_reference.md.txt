# DataFrameSqlPrepared API Reference

# Practical DataFrameSqlPrepared API Reference

## API Reference Style

This page documents the public prepared-statement helper returned by `DataFrame::prepare_sql()` or `DataFrameSqlAccessor::prepare()`.

## E

### `execute`

#### API Signatures

```cpp
DataFrame execute(self) const

DataFrame execute(self, std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params) const

DataFrame execute(self, std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params, std::unordered_map<std::string, std::string> identifier_params) const
```

#### Parameters

`params` : `std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam>`
    Named scalar bindings supplied to placeholders such as `{min_sales}` inside the prepared statement template.

`identifier_params` : `std::unordered_map<std::string, std::string>`
    Named identifier bindings used for placeholder substitution when a statement needs a dynamic SQL identifier such as a table alias.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Runs the prepared SQL statement with the supplied parameters and returns the result as a new `DataFrame`.

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

```text
id  sales
1   100.0
2   120.0
```

