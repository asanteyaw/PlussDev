# DataFrameSqlAccessor API Reference

# Practical DataFrameSqlAccessor API Reference

## API Reference Style

This page documents the public SQL accessor returned by `DataFrame::sql()`.

## E

### `execute`

#### API Signature

```cpp
DataFrame execute(self, std::string_view statement) const
```

#### Parameters

`statement` : `std::string_view`
    SQL query text run against the current frame. The frame is exposed to the statement as `self`.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Runs a SQL query against the current frame and returns the result as a new `DataFrame`.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto sql = df.sql();

auto out = sql.execute("SELECT id, sales FROM self WHERE sales > 95");
```

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
DataFrameSqlPrepared prepare(self, std::string_view statement) const
```

#### Parameters

`statement` : `std::string_view`
    Parameterized SQL statement template. Use placeholders such as `{min_sales}` when you plan to bind values later.

#### Returns

`DataFrameSqlPrepared`
    Value produced by this call.

#### What It Does

Compiles a parameterized SQL statement that can be executed repeatedly with different bindings.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto sql = df.sql();

auto out = sql.prepare("SELECT id, sales FROM self WHERE sales > {min_sales}");
```

```text
Prepared statement ready
```

#### Practical Behavior

- `prepare(statement)` captures a parameterized SQL template and returns a reusable prepared helper.
- Use placeholders such as `{min_sales}` for value parameters you will bind later.
- Prepared execution is the better path when you want repeated queries without rebuilding the statement each time.

