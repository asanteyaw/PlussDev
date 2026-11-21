<!-- pluss-hand-authored -->

# DataFrameSqlPrepared::execute

[Back to DataFrameSqlPrepared API Reference](../../dataframe_sql_prepared_api_reference)

## Signatures

```cpp
DataFrame DataFrameSqlPrepared::execute() const
DataFrame DataFrameSqlPrepared::execute(std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params) const
DataFrame DataFrameSqlPrepared::execute(std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params,
                                        std::unordered_map<std::string, std::string> identifier_params) const
```

## What It Does

Runs the prepared statement and returns the result as a new `DataFrame`. Each call builds a fresh in-memory
workspace for the frame, so a prepared statement can be executed any number of times.

- `execute()` uses only the parameters that were already set in the `SqlOptions` the statement was prepared with.
- `execute(params)` binds `{name}` value placeholders, over any defaults from the options.
- `execute(params, identifier_params)` also binds `{@name}` placeholders, which stand for column or table names.

## Parameters

**params** (`std::unordered_map<std::string, SqlParam>`)

Values for `{name}` placeholders. A placeholder with no value and no default throws.

**identifier_params** (`std::unordered_map<std::string, std::string>`)

Names for `{@name}` placeholders, such as a column to select.

## Returns

`DataFrame`
    The rows the statement produced.

## Examples

Example 1. Bind a value.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Shelf(pluss::make_series({1, 2, 3}))},
    {"sales", Shelf(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Shelf(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto stmt = orders.sql().prepare("SELECT id FROM df WHERE sales > {m}");

std::cout << stmt.execute({{"m", 110.0}});
```

### Output

```text
===========
     | id  
-----------
0    | 2   
-----------
[1 cs x 1 feat]
```

Example 2. Bind a column name as well as a value.

```cpp
auto by_column = orders.sql().prepare("SELECT {@col} FROM df WHERE sales > {m}");

std::cout << by_column.execute({{"m", 95.0}}, {{"col", "city"}});
```

### Output

```text
============
     | city 
------------
0    | Accra
1    | Tema 
------------
[2 cs x 1 feat]
```

Example 3. Defaults from the options, overridden per call.

```cpp
DataFrame::SqlOptions options;
options.params["m"] = 95.0;

auto stmt = orders.sql(options).prepare("SELECT id FROM df WHERE sales > {m}");

std::cout << stmt.execute();
std::cout << stmt.execute({{"m", 110.0}});
```

### Output

```text
===========
     | id  
-----------
0    | 1   
1    | 2   
-----------
[2 cs x 1 feat]===========
     | id  
-----------
0    | 2   
-----------
[1 cs x 1 feat]
```

Example 4. A placeholder with no value throws.

```cpp
auto stmt = orders.sql().prepare("SELECT id FROM df WHERE sales > {m}");

try {
  auto out = stmt.execute();
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
DataFrame::sql().execute: SQL params missing key 'm'
```

## See Also

- [`DataFrameSqlAccessor::prepare`](../dataframe-sql/prepare.md): create a prepared statement.
- [`DataFrame::prep_sql`](../dataframe/prep_sql.md): the shorthand for `sql().prepare(...)`.
