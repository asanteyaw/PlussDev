# DataFrameSqlAccessor API Reference

# Practical DataFrameSqlAccessor API Reference

## API Reference Style

This page documents the public SQL accessor returned by `DataFrame::sql()`.

### `execute`

#### Signature

```cpp
DataFrame execute(self, std::string_view statement) const
```

#### Parameters

`statement` : `std::string_view`
    SQL statement text to prepare against the current frame.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

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

#### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

### `prepare`

#### Signature

```cpp
DataFrameSqlPrepared prepare(self, std::string_view statement) const
```

#### Parameters

`statement` : `std::string_view`
    SQL statement text to prepare against the current frame.

#### Returns

`DataFrameSqlPrepared`
    Value produced by this call.

#### Notes

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

#### Output

```text
Prepared statement ready
```
