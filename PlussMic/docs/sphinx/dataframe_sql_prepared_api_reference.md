# DataFrameSqlPrepared API Reference

# Practical DataFrameSqlPrepared API Reference

## API Reference Style

This page documents the public prepared-statement helper returned by `DataFrame::prepare_sql()` or `DataFrameSqlAccessor::prepare()`.

### `execute`

#### Signature

```cpp
DataFrame execute(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This call is part of the public `DataFrameSqlPrepared` API and is documented here using a concrete usage example.

#### See also

- `DataFrame execute(self, std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params) const`
- `DataFrame execute(self, std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params, std::unordered_map<std::string, std::string> identifier_params) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto stmt = df.prepare_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");

auto out = stmt.execute();
```

#### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

### `execute`

#### Signature

```cpp
DataFrame execute(self, std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params) const
```

#### Parameters

`params` : `std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This call is part of the public `DataFrameSqlPrepared` API and is documented here using a concrete usage example.

#### See also

- `DataFrame execute(self) const`
- `DataFrame execute(self, std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params, std::unordered_map<std::string, std::string> identifier_params) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto stmt = df.prepare_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");

auto out = stmt.execute({{"min_sales", 95.0}});
```

#### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

### `execute`

#### Signature

```cpp
DataFrame execute(self, std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params, std::unordered_map<std::string, std::string> identifier_params) const
```

#### Parameters

`params` : `std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam>`
    See the signature for the supported argument form.

`identifier_params` : `std::unordered_map<std::string, std::string>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This call is part of the public `DataFrameSqlPrepared` API and is documented here using a concrete usage example.

#### See also

- `DataFrame execute(self) const`
- `DataFrame execute(self, std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto stmt = df.prepare_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");

auto out = stmt.execute({{"min_sales", 95.0}}, {{"table_name", "self"}});
```

#### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
