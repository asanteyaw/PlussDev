# DataFrameSqlAccessor API Reference

This page documents the public SQL accessor returned by `DataFrame::sql()`.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

## E

### `execute`

#### Signature

```cpp
DataFrameSqlAccessor::execute(std::string_view statement) -> DataFrame
```

#### What It Does

Runs a SQL query against the current frame and returns the result as a new `DataFrame`.

#### Parameters

**statement**
    SQL query text run against the current frame. The frame is exposed to the statement as `self`.

#### Returns

`DataFrame`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto sql = df.sql();

auto out = sql.execute("SELECT id, sales FROM self WHERE sales > 95");
```

##### Output

```text
id  sales
1   100.0
2   120.0
```

#### Practical Behavior

- `execute(statement)` runs the SQL immediately against the current frame.
- The current frame is exposed inside the statement as `self`.
- Use this for one-off queries. Use `prepare(...)` when the same statement shape will be executed repeatedly with different parameter values.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="#prepare">
    <span class="label">Next</span>
    <span class="title">prepare</span>
  </a>
</div>

## P

### `prepare`

#### Signature

```cpp
DataFrameSqlAccessor::prepare(std::string_view statement) -> DataFrameSqlPrepared
```

#### What It Does

Compiles a parameterized SQL statement that can be executed repeatedly with different bindings.

#### Parameters

**statement**
    Parameterized SQL statement template. Use placeholders such as `{min_sales}` when you plan to bind values later.

#### Returns

`DataFrameSqlPrepared`
    Value produced by this call.

#### Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto sql = df.sql();

auto out = sql.prepare("SELECT id, sales FROM self WHERE sales > {min_sales}");
```

##### Output

```text
Prepared statement ready
```

#### Practical Behavior

- `prepare(statement)` captures a parameterized SQL template and returns a reusable prepared helper.
- Use placeholders such as `{min_sales}` for value parameters you will bind later.
- Prepared execution is the better path when you want repeated queries without rebuilding the statement each time.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#execute">
    <span class="label">Previous</span>
    <span class="title">execute</span>
  </a>
</div>
