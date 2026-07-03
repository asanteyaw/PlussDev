# DataFrameSqlAccessor API Reference

# Practical DataFrameSqlAccessor API Reference

## API Reference Style

This page documents the public SQL accessor returned by `DataFrame::sql()`.

## E

### `execute`

#### API Signature

```cpp
DataFrameSqlAccessor::execute(statement: std::string_view) -> DataFrame
```

#### Notes

Runs a SQL query against the current frame and returns the result as a new `DataFrame`.

#### Inputs

`statement` : `std::string_view`
    SQL query text run against the current frame. The frame is exposed to the statement as `self`.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto sql = df.sql();

auto out = sql.execute("SELECT id, sales FROM self WHERE sales > 95");
```

##### Expected output

```text
id  sales
1   100.0
2   120.0
```

#### Practical Behavior

- `execute(statement)` runs the SQL immediately against the current frame.
- The current frame is exposed inside the statement as `self`.
- Use this for one-off queries. Use `prepare(...)` when the same statement shape will be executed repeatedly with different parameter values.


## P

### `prepare`

#### API Signature

```cpp
DataFrameSqlAccessor::prepare(statement: std::string_view) -> DataFrameSqlPrepared
```

#### Notes

Compiles a parameterized SQL statement that can be executed repeatedly with different bindings.

#### Inputs

`statement` : `std::string_view`
    Parameterized SQL statement template. Use placeholders such as `{min_sales}` when you plan to bind values later.

#### Output

`DataFrameSqlPrepared`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto sql = df.sql();

auto out = sql.prepare("SELECT id, sales FROM self WHERE sales > {min_sales}");
```

##### Expected output

```text
Prepared statement ready
```

#### Practical Behavior

- `prepare(statement)` captures a parameterized SQL template and returns a reusable prepared helper.
- Use placeholders such as `{min_sales}` for value parameters you will bind later.
- Prepared execution is the better path when you want repeated queries without rebuilding the statement each time.

