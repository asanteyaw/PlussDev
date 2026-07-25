<!-- pluss-hand-authored -->

# DataFrameSqlPrepared API Reference

This page documents the reusable prepared-statement handle returned by
[`DataFrameSqlAccessor::prepare()`](dataframe_sql_accessor_api_reference.md#prepare)
and `DataFrame::prep_sql()`.

```{important} The table is named df, not self
Exactly like `DataFrameSqlAccessor::execute()`, a prepared statement addresses
its bound frame as **`df`** — `SELECT ... FROM df ...`. There is no `self`.
```

A `DataFrameSqlPrepared` bundles three things captured once, at `prepare()`
time: the statement **text** (with any `{name}`/`{@name}` placeholders left
unresolved), a copy of the source **frame**, and a copy of the accessor's
**`SqlOptions`** at that moment (its `sources`, `collision_policy`,
`missing_param_policy`, resource limits, and so on). Every `execute(...)`
call layers fresh `params`/`identifier_params` on top of that captured base —
it never mutates the prepared value itself, so the same `DataFrameSqlPrepared`
can be executed any number of times, with different bindings each time,
without rebuilding anything.

## How To Use This Page

- The three `execute()` overloads are grouped together below since they're
  all one conceptual operation with progressively more binding power.
- Examples reuse the same running dataset as the
  [`DataFrameSqlAccessor` reference](dataframe_sql_accessor_api_reference.md)
  (`orders`, plus `managers` for the JOIN example) so results are directly
  comparable.
- Each example is followed immediately by verified output hand-traced against
  `DataFrameSql.cpp`.

## E

### `execute`

#### Signatures

```cpp
DataFrameSqlPrepared::execute() const -> DataFrame
DataFrameSqlPrepared::execute(std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params) const -> DataFrame
DataFrameSqlPrepared::execute(std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam> params, std::unordered_map<std::string, std::string> identifier_params) const -> DataFrame
```

#### What It Does

Renders the captured statement text (substituting any supplied `params`/
`identifier_params` into its `{name}`/`{@name}` placeholders, on top of
whatever `params`/`identifier_params` were already present in the base
`SqlOptions` from `prepare()` time), parses it, and runs it exactly like
`DataFrameSqlAccessor::execute()` would — same pipeline, same resource
limits, same plan cache. The zero-argument overload is for statements with no
placeholders (or ones fully satisfied by base-`SqlOptions` params already).
The one-map overload adds scalar bindings for `{name}` placeholders. The
two-map overload adds both scalar and identifier (`{@name}`) bindings in the
same call.

#### Parameters

**params** (`std::unordered_map<std::string, DataFrame::SqlOptions::SqlParam>`)

Scalar values merged into the base `SqlOptions::params` for this call only —
`SqlParam` is `std::variant<std::nullptr_t, bool, std::int64_t, double,
std::string>`. Each entry overrides any base param of the same key.

**identifier_params** (`std::unordered_map<std::string, std::string>`)

Column/identifier names merged into the base `SqlOptions::identifier_params`
for this call only, resolved into `{@name}` placeholders as backtick-quoted
identifiers.

#### Returns

`DataFrame`
    The same result `DataFrameSqlAccessor::execute(rendered_statement)` would
    produce, where `rendered_statement` is the prepared text after this
    call's placeholder substitution.

#### Examples

The dataset (identical to the accessor page):

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"id",     Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}))},
     {"region", Thread(pluss::make_note({"east", "west", "east", "north", "west"}))},
     {"sales",  Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0, 250.0}))}});
```

Example 1. `execute()` with no placeholders at all — the simplest case, no
bindings needed.

```cpp
auto stmt = orders.sql().prepare("SELECT id, region, sales FROM df WHERE sales >= 100");
auto out = stmt.execute();
std::cout << out;
```

##### Output

```text
=========================
      | id  region  sales
-------------------------
0     | 1   east    100  
1     | 2   west    200  
2     | 3   east    150  
4     | 5   west    250  
-------------------------
[4 cs x 3 feat]
```

`id 4 / 90` fails `sales >= 100` and drops, exactly as a direct `execute()`
call on the accessor would behave — `prepare()` changed nothing about the
query semantics, only when the text gets typed.

Example 2 & 3. The real point of `prepare()`: the **same** `stmt` executed
twice with different scalar bindings, no re-templating.

```cpp
auto region_stmt = orders.sql().prepare("SELECT id, region, sales FROM df WHERE region = {region}");

auto east = region_stmt.execute({{"region", std::string("east")}});
std::cout << east;

auto west = region_stmt.execute({{"region", std::string("west")}});
std::cout << west;
```

##### Output (`region = "east"`)

```text
=========================
      | id  region  sales
-------------------------
0     | 1   east    100  
2     | 3   east    150  
-------------------------
[2 cs x 3 feat]
```

##### Output (`region = "west"`, same `region_stmt`)

```text
=========================
      | id  region  sales
-------------------------
1     | 2   west    200  
4     | 5   west    250  
-------------------------
[2 cs x 3 feat]
```

`region_stmt` itself never changes between the two calls — each `execute(...)`
takes a private copy of the base `SqlOptions`, merges in that call's `params`,
and discards the copy afterward. This is the pattern `prepare()` is for: build
the statement shape once, vary only the bound values across calls (a loop
over regions, a repeated per-request query with a user-supplied filter value,
and so on).

Example 4. Both maps together — a scalar `{min_sales}` and an identifier
`{@col}` bound in the same call.

```cpp
auto flex_stmt = orders.sql().prepare(
    "SELECT id, {@col} FROM df WHERE sales >= {min_sales} ORDER BY {@col} DESC");
auto out4 = flex_stmt.execute({{"min_sales", 100.0}}, {{"col", "sales"}});
std::cout << out4;
```

##### Output

```text
=================
      | id  sales
-----------------
4     | 5   250  
1     | 2   200  
2     | 3   150  
0     | 1   100  
-----------------
[4 cs x 2 feat]
```

The rendered statement becomes
``SELECT id, `sales` FROM df WHERE sales >= 100 ORDER BY `sales` DESC``. Both
placeholders are re-resolved fresh from `flex_stmt`'s captured text on this
one call; a later call could bind `col` to a completely different column
name (say, `region`) with no re-`prepare()` needed.

Example 5. The base `SqlOptions` a prepared statement carries — including
`sources` for a `JOIN` — come from **`prepare()` time**, not from the later
`execute(...)` call.

```cpp
auto managers = DataFrame(
    {{"region",  Thread(pluss::make_note({"east", "west", "north"}))},
     {"manager", Thread(pluss::make_note({"Frey", "Okafor", "Lund"}))}});

auto joined_stmt = orders.sql(DataFrame::SqlOptions{.sources = {{"managers", &managers}}})
                       .prepare(
    "SELECT id, region, m.manager FROM df "
    "INNER JOIN managers AS m ON region = m.region "
    "WHERE id > {min_id} ORDER BY id ASC");

auto out5 = joined_stmt.execute({{"min_id", 2.0}});
std::cout << out5;
```

##### Output

```text
=============================
      | id  region  m.manager
-----------------------------
2     | 3   east    Frey     
3     | 4   north   Lund     
4     | 5   west    Okafor   
-----------------------------
[3 cs x 3 feat]
```

`managers` only had to be bound once, when `.sql(...)` was called to build
`joined_stmt` — every subsequent `execute({...})` call reuses that same
binding automatically; there is no way to swap in a different `sources` map
per call, only `params`/`identifier_params` can vary per execution.

Example 6. `reject_unused_params` (captured at `prepare()` time, same as any
other option) still applies to each `execute(params)` call — an unused key is
caught per call, not just once.

```cpp
auto strict_stmt = orders.sql(DataFrame::SqlOptions{.reject_unused_params = true})
                       .prepare("SELECT id, sales FROM df WHERE region = {region}");
try {
  auto bad = strict_stmt.execute({{"region", std::string("east")}, {"typo_key", std::string("x")}});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

##### Output

```text
DataFrame::sql().execute: unused SQL param key 'typo_key'
```

Nothing about `typo_key` is special-cased by `execute()`'s overload — it's
simply a `params` entry that never matched a `{typo_key}` placeholder in
`strict_stmt`'s text, and `reject_unused_params` (inherited from the base
options) rejects that outright. This is a useful guard specifically because
the same prepared statement tends to get called from several places; a typo'd
binding key anywhere gets caught immediately instead of silently doing
nothing.

Example 7. `prepare()` performs no validation — a required placeholder left
unbound is only ever caught inside `execute()`, at the point it's actually
rendered.

```cpp
auto needs_param = orders.sql().prepare("SELECT id FROM df WHERE region = {region}");
// needs_param exists fine here -- nothing has been parsed or checked yet.

try {
  auto bad2 = needs_param.execute();  // no {region} binding supplied
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

##### Output

```text
DataFrame::sql().execute: SQL params missing key 'region'
```

`prepare()` happily stored `"SELECT id FROM df WHERE region = {region}"`
verbatim with no complaint — the placeholder is only discovered, and only
enforced against `MissingParamPolicy` (`Error` by default, the throw seen
here), the moment some `execute(...)` call actually tries to render it.

#### Notes

- Every `execute(...)` overload takes a private copy of the base `SqlOptions`
  before merging in that call's `params`/`identifier_params` — the
  `DataFrameSqlPrepared` value itself is never mutated, so it's always safe to
  call repeatedly, from multiple places, with different bindings each time.
- The automatic SQL plan cache (`SqlOptions::enable_plan_cache`, default
  `true`, keyed by exact rendered statement text) already avoids re-parsing
  identical statement text on repeated calls, with no `prepare()` involved at
  all. `DataFrameSqlPrepared`'s real value is therefore ergonomic — reusing a
  statement's *text* without retyping/re-templating it — not a parsing
  speed-up the plan cache wasn't already giving you.
- `missing_param_policy`, `reject_unused_params`, `collision_policy`,
  resource limits, and `sources` all come from the base `SqlOptions` captured
  at `prepare()` time (i.e. whatever `DataFrame::sql(options)` was called
  with before `.prepare(...)`). None of them can be changed per `execute(...)`
  call — only `params` and `identifier_params` can.
- See the
  [`DataFrameSqlAccessor` reference](dataframe_sql_accessor_api_reference.md)
  for the full SQL grammar, the `case` pseudo-column, `LEFT JOIN`'s rejection,
  and the `ResultMode::Chunked` no-op gotcha — everything documented there for
  `execute(statement)` applies identically once the statement is rendered
  here.

<div class="pluss-inline-prev-next">
</div>
