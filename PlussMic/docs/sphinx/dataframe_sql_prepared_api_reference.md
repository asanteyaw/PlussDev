# DataFrameSqlPrepared API Reference

This page documents the public prepared-statement helper returned by `DataFrame::prep_sql()` or `DataFrameSqlAccessor::prepare()`.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

## E

### `execute`

#### Signatures

```cpp
DataFrame DataFrameSqlPrepared::execute()

DataFrame DataFrameSqlPrepared::execute(std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params)

DataFrame DataFrameSqlPrepared::execute(std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params, std::unordered_map<std::string, std::string> identifier_params)
```

#### What It Does

Runs the prepared SQL statement with the supplied parameters and returns the result as a new `DataFrame`.

#### Parameters

**params**
    Named scalar bindings supplied to placeholders such as `{min_sales}` inside the prepared statement template.

**identifier_params**
    Named identifier bindings used for placeholder substitution when a statement needs a dynamic SQL identifier such as a table alias.

#### Returns

`DataFrame`
    Value produced by this call.

#### Examples

Example 1. Run the prepared SQL statement with no external parameter bindings.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Slate(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto stmt = df.prep_sql("SELECT id, sales FROM self WHERE sales > 95");

auto out = stmt.execute();
```

##### Output

```text
id  sales
1   100.0
2   120.0
```
Example 2. Run the prepared SQL statement while binding named scalar placeholders.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Slate(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto stmt = df.prep_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");

auto out = stmt.execute({{"min_sales", 95.0}});
```

##### Output

```text
id  sales
1   100.0
2   120.0
```
Example 3. Run the prepared SQL statement while binding both scalar placeholders and SQL identifier placeholders.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Slate(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto stmt = df.prep_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");

auto out = stmt.execute({{"min_sales", 95.0}}, {{"table_name", "self"}});
```

##### Output

```text
id  sales
1   100.0
2   120.0
```