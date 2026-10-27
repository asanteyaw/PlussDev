# Part VII: Query Expressions and SQL

PlussMic gives you three practical ways to choose rows:

- `DataFrame::query(...)` evaluates a text predicate against the current frame.
- `DataFrame::filter(...)` applies an explicit boolean mask that you build in C++.
- `DataFrame::sql(...).execute(...)` parses a small SQL `SELECT` language for projection, joins, parameters, ordering, and limits.

The important design point is that these are not three unrelated systems. The SQL executor parses the SQL statement, builds an intermediate `DataFrame`, and sends the SQL `WHERE` clause to `DataFrame::query`. That means SQL `WHERE` supports the same predicate language as `query`, while SQL itself is responsible for `SELECT`, `FROM`, `JOIN`, `ORDER BY`, `LIMIT`, `OFFSET`, and parameter replacement.

Use `query` when you only need row filtering. Use SQL when you also want column projection, source binding, a join, a limit, or late-bound parameters. Use an explicit mask when your predicate is easier or safer to express as ordinary C++ code.

## Query expressions

`DataFrame::query` takes a predicate string and returns a new frame containing only the cases for which that predicate is true. The result keeps the same feature set as the input frame unless you later select or drop features.

```cpp
auto older = df.query("age >= 45");
auto positive = df.query("balance > 0");
auto known = df.query("job != 'unknown'");
auto band = df.query("age between 45 and 50");
auto selected = df.query("job in ('management', 'technician')");
auto combined = df.query("age >= 45 and balance > 0");
```

A query string names features directly. Numeric comparisons use numeric features. String comparisons use quoted string literals. Boolean combinations use words such as `and` and `or`.

Use query expressions for:

- filtering a frame without changing its columns;
- readable conditions in analysis code;
- direct comparisons against literals;
- membership checks;
- range checks;
- null filtering through helper predicates.

Examples:

```cpp
auto clean_balance = df.query("isnotnull(balance)");
auto clean_job = df.query("isnotnull(job) and job != ''");
auto adults_with_income = df.query("age >= 18 and income > 0");
```

Because `query` works from feature names, treat those names as part of your data contract. If a CSV column is renamed from `balance` to `account_balance`, every query using `balance` must change.

## Query result shape

A query filters cases, not features. If the input frame has 20 features and the predicate matches 5 cases, the output has 5 cases and the same 20 features.

This is why a common workflow is:

```cpp
auto filtered = df.query("age >= 45 and balance > 0");
auto compact = filtered.features({"age", "job", "balance"});
```

The first line chooses rows. The second line chooses columns.

## Explicit masks

Use a boolean mask when the condition is easier to express in C++ than in the text predicate language. A mask must have one boolean value for each case in the frame. `true` keeps the row. `false` removes it.

```cpp
auto values = df.features("age").as_series().host<std::int64_t>();
auto mask = pluss::Series::zeros(
    {static_cast<std::int64_t>(values.size())},
    pluss::codem::ScalarType::Bool);

auto* keep = reinterpret_cast<bool*>(mask.raw_data());
for (std::size_t i = 0; i < values.size(); ++i) {
  keep[i] = values[i] >= 45 && values[i] <= 50;
}

auto filtered = df.filter(mask);
```

This style is more verbose than `df.query("age between 45 and 50")`, but it gives you full control. It is useful when the predicate calls your own C++ functions, depends on external state, or must be debugged step by step.

Use masks when:

- the predicate is a C++ function;
- the predicate depends on a lookup table or an object that is not a feature;
- you already computed a boolean `Series`;
- you want to inspect the mask before filtering;
- you need precise control over exceptional cases.

## SQL is a PlussMic SELECT language

`DataFrame::sql()` returns a `DataFrameSqlAccessor`. The accessor binds the
calling frame and optional extra sources into an in-memory SQLite workspace,
executes the SQL there, and converts the result back into a `DataFrame`.

```cpp
auto result = df.sql().execute(
    "SELECT age, job, balance FROM df WHERE age >= 45");
```

This SQL layer is still query-oriented. Statements that do not produce a result
set, such as `DELETE FROM df`, are rejected. But within result-producing SQL, the
surface is much broader than the old parser-only engine: grouped aggregates,
common table expressions, subqueries, `LEFT JOIN`, `DISTINCT`, and ordinary SQLite
expressions now work.

The calling frame is named `df` by default:

```sql
SELECT age, job FROM df
```

You may alias the calling frame:

```sql
SELECT customers.age FROM df AS customers
```

Pluss-specific conveniences still remain on top of the SQLite backend:

- the calling frame is available as `df`
- scalar parameters such as `{min_age}` still work
- identifier parameters such as `{@feature_name}` still work
- `case_axis_aliases` still expose the case axis as selectable SQL columns
- bare source-name shorthand such as `df.sql().execute("df")` expands to
  `SELECT * FROM df`

## Verified SQL example

The following example is compiled and run by `PlussMic/docs/sphinx/manual/examples/plussmic_manual_examples.cpp`.

```cpp
void example_09_sql() {
  section("09 sql");
  auto sales = sales_frame().assign({{
      "revenue",
      [](const DataFrame& frame) {
        return frame.features("units") * frame.features("price");
      },
  }});

  auto top = sales.sql().execute(
      "SELECT city, revenue FROM df WHERE revenue >= 28 ORDER BY revenue DESC LIMIT 2");
  std::cout << top.to_string() << '\n';

  DataFrame::SqlOptions options;
  options.params["min_revenue"] = std::int64_t{28};
  options.identifier_params["value_col"] = "revenue";
  auto parameterized = sales.sql(std::move(options)).execute(
      "SELECT city, {@value_col} FROM df WHERE revenue >= {min_revenue} ORDER BY revenue DESC LIMIT 3");
  std::cout << parameterized.to_string() << '\n';

  auto prepared = sales.prepare_sql(
      "SELECT city FROM df WHERE revenue >= {min_revenue} ORDER BY revenue DESC LIMIT {limit_n}");
  auto prepared_result = prepared.execute({
      {"min_revenue", std::int64_t{28}},
      {"limit_n", std::int64_t{2}},
  });
  std::cout << prepared_result.to_string() << '\n';
}
```

The `sales_frame()` helper builds this input:

```text
city    region  units  price
Accra   south   10     3
Tema    south   7      4
Kumasi  middle  12     2.5
Tamale  north   5      5
```

The example first derives `revenue = units * price`, then runs three SQL variants over the enriched frame.

Verified output:

```text
=== 09 sql ===
======================
     | city    revenue
----------------------
0    | Accra   30     
2    | Kumasi  30     
----------------------
[2 cs x 2 feat]
======================
     | city    revenue
----------------------
0    | Accra   30     
2    | Kumasi  30     
1    | Tema    28     
----------------------
[3 cs x 2 feat]
=============
     | city  
-------------
0    | Accra 
2    | Kumasi
-------------
[2 cs x 1 feat]
```

The first result has two features, `city` and `revenue`, because the SQL projection is `SELECT city, revenue`. The `WHERE revenue >= 28` predicate keeps Accra, Kumasi, and Tema, then `ORDER BY revenue DESC LIMIT 2` returns only the first two after sorting. Accra and Kumasi both have revenue `30`, so both appear before Tema.

The second result uses two different parameter systems. `{min_revenue}` is a scalar parameter and becomes the numeric value `28`. `{@value_col}` is an identifier parameter and becomes the feature name `revenue`. Identifier parameters are for column names, not values. They are rendered as escaped SQL identifiers by PlussMic before parsing.

The third result uses a prepared SQL statement. The statement template is created once with `prepare_sql`, and the values are supplied when `execute` is called. The projection only asks for `city`, so the output has one feature.

## SQL options

`DataFrame::SqlOptions` configures how SQL statements are rendered and executed.

```cpp
pluss::table::DataFrame::SqlOptions options;
options.params["min_age"] = std::int64_t{45};
options.identifier_params["value_col"] = "balance";
options.max_result_rows = 1000;
options.timeout_ms = 5000;
options.diagnostics_explain = true;

auto result = df.sql(options).execute(
    "SELECT age, job, {@value_col} FROM df WHERE age >= {min_age}");
```

Important options:

| Option | Purpose |
| --- | --- |
| `sources` | Additional named `DataFrame` sources available to joins. |
| `params` | Scalar placeholders like `{min_age}`. Values may be null, bool, `std::int64_t`, double, or string. |
| `identifier_params` | Identifier placeholders like `{@feature}`. Use these for feature or source names. |
| `missing_param_policy` | Choose whether missing parameters are errors or render as SQL null. |
| `reject_unused_params` | Fail if a supplied scalar or identifier parameter is never used. |
| `collision_policy` | Control duplicate projection names: error, suffix, or overwrite. |
| `max_intermediate_rows` | Legacy guard field. The SQLite backend materializes the final query result rather than exposing parser-stage intermediate frames. |
| `max_result_rows` | Cap the returned result. |
| `memory_budget_mb` | Estimate and guard result memory. |
| `timeout_ms` | Check execution time at SQL execution stages. |
| `diagnostics_explain` | Print a statement plan through Pluss diagnostics. |
| `diagnostics_profile` | Print stage timing. |
| `diagnostics_trace_tokens` | Print tokenization output. |
| `diagnostics_trace_plan` | Print the SQLite query plan. |
| `enable_plan_cache` | Legacy field kept for API stability. |
| `plan_cache_capacity` | Legacy field kept for API stability. |
| `result_mode` | Request materialized or chunked behavior. Current `execute()` still returns a materialized `DataFrame`. |
| `chunk_rows` | Chunk size used with chunked result mode diagnostics. |
| `case_axis_aliases` | Names that expose the case axis as a projected output feature. |

## Scalar parameters

Scalar parameters are placeholders wrapped in braces:

```cpp
DataFrame::SqlOptions options;
options.params["min_age"] = std::int64_t{45};
options.params["blocked_job"] = std::string{"unknown"};
options.params["limit_n"] = std::int64_t{4};

auto out = df.sql(std::move(options)).execute(
    "SELECT age, job FROM df "
    "WHERE age >= {min_age} and job != {blocked_job} "
    "ORDER BY age DESC LIMIT {limit_n}");
```

The SQL renderer turns numbers into SQL numeric tokens, strings into quoted SQL string literals, bools into `true` or `false`, and null values into `null`. This happens before parsing, so the parser receives a normal SQL-like statement after parameter substitution.

Use scalar parameters for values only. Do not use them for column names. For column names, use identifier parameters.

## Identifier parameters

Identifier parameters are placeholders with `@` after the opening brace:

```cpp
DataFrame::SqlOptions options;
options.identifier_params["col"] = "age";
options.params["limit_n"] = std::int64_t{2};

auto out = df.sql(std::move(options)).execute(
    "SELECT {@col} FROM df LIMIT {limit_n}");
```

This is safer and clearer than concatenating a feature name into the SQL string yourself. PlussMic validates identifier parameter values before rendering them.

Use identifier parameters for:

- selected feature names;
- order-by feature names;
- source names when they are part of a supported identifier position;
- aliases where supported by the parser.

## Prepared SQL

Use prepared SQL when the statement shape stays the same and only values change.

```cpp
auto prepared = df.prepare_sql(
    "SELECT age FROM df WHERE age >= {min_age} ORDER BY age DESC LIMIT {limit_n}");

auto first = prepared.execute({
    {"min_age", std::int64_t{40}},
    {"limit_n", std::int64_t{3}},
});

auto second = prepared.execute({
    {"min_age", std::int64_t{55}},
    {"limit_n", std::int64_t{10}},
});
```

Prepared execution keeps the statement template separate from runtime parameters. It also lets you bind identifier parameters at execution time:

```cpp
auto prepared = df.prepare_sql(
    "SELECT {@col} FROM df WHERE age >= {min_age} LIMIT {limit_n}");

auto out = prepared.execute(
    {
        {"min_age", std::int64_t{45}},
        {"limit_n", std::int64_t{5}},
    },
    {
        {"col", "balance"},
    });
```

## Case-axis projection

The case axis is not a normal feature unless you ask SQL to expose it. Configure `case_axis_aliases` when you want case labels in the result:

```cpp
auto with_case = df.sql(DataFrame::SqlOptions{
    .case_axis_aliases = {"case", "idx"},
}).execute("SELECT idx, age FROM df ORDER BY age DESC LIMIT 3");
```

In this example, `idx` becomes a projected output feature representing the original case labels. Without `case_axis_aliases`, `SELECT idx` would be treated like a normal feature lookup and would fail unless the frame actually had a feature named `idx`.

## Joins

Additional frames are supplied through `SqlOptions::sources`. The calling frame must still participate in the `FROM` or `JOIN` sources.

```cpp
DataFrame metrics({
    {"age", df.features("age")},
    {"score", df.features("balance")},
});

auto joined = df.sql(DataFrame::SqlOptions{
    .sources = {{"m", &metrics}},
}).execute("SELECT df.age, m.score FROM df JOIN m ON df.age = m.age LIMIT 6");
```

The join source is named `m`, so the SQL statement can refer to `m.age` and `m.score`. Joined right-hand columns are prefixed internally with the joined source alias, which is why qualified projections such as `m.score` are useful.

Keep joins narrow. Set `max_intermediate_rows` when joining data that could expand unexpectedly.

## Collision policy

SQL projection names must be unique by default:

```sql
SELECT age as v, balance as v FROM df LIMIT 3
```

This projection asks for two output features with the same name, `v`, so the default collision policy rejects it.

Use suffixing if you want PlussMic to keep both:

```cpp
auto out = df.sql(DataFrame::SqlOptions{
    .collision_policy = DataFrame::SqlOptions::CollisionPolicy::Suffix,
}).execute("SELECT age as v, balance as v FROM df LIMIT 3");
```

The result has `v` and `v_2`.

Use overwrite only when replacing an earlier duplicate is intentional:

```cpp
auto out = df.sql(DataFrame::SqlOptions{
    .collision_policy = DataFrame::SqlOptions::CollisionPolicy::Overwrite,
}).execute("SELECT age as v, balance as v FROM df LIMIT 3");
```

The result keeps one `v`.

## Choosing the right tool

Use `query` when:

- you only need to filter rows;
- you want the output to keep all features;
- the predicate is short and readable;
- you do not need joins or SQL parameters.

Use SQL when:

- you want projection and filtering in one expression;
- you need `ORDER BY`, `LIMIT`, or `OFFSET`;
- you need to bind scalar or identifier parameters;
- you need to join against additional named frames;
- you want execution guards such as `max_result_rows`, `max_intermediate_rows`, `memory_budget_mb`, or `timeout_ms`.

Use explicit masks when:

- the predicate is ordinary C++ logic;
- you need custom functions;
- you need external state;
- you already have a boolean `Series`;
- you want to debug every intermediate value.

## Common mistakes

Do not use SQL for mutation:

```cpp
df.sql().execute("DELETE FROM df"); // rejected
```

Do not use scalar parameters for feature names:

```cpp
// Wrong: renders the string as a SQL string literal, not an identifier.
options.params["col"] = std::string{"age"};
df.sql(options).execute("SELECT {col} FROM df");
```

Use an identifier parameter instead:

```cpp
options.identifier_params["col"] = "age";
df.sql(options).execute("SELECT {@col} FROM df");
```

Do not expect projection expressions:

```sql
SELECT units * price AS revenue FROM df
```

Derive the feature first, then select it:

```cpp
auto enriched = df.assign({{
    "revenue",
    [](const DataFrame& frame) {
      return frame.features("units") * frame.features("price");
    },
}});

auto result = enriched.sql().execute("SELECT revenue FROM df");
```

Do not assume SQL `WHERE` is an external database dialect. It is PlussMic's DataFrame query predicate language inside a SQL statement.
