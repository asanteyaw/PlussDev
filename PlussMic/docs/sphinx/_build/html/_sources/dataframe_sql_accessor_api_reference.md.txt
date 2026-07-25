<!-- pluss-hand-authored -->

# DataFrameSqlAccessor API Reference

This page documents the public SQL accessor returned by `DataFrame::sql()` /
`DataFrame::sql(SqlOptions)`.

```{important} The table is named df, not self
Every statement addresses the frame `sql()` was called on as **`df`** —
`SELECT ... FROM df ...`. There is no `self` keyword; if you've seen `FROM self`
in older notes or examples, that name never existed in the implementation and
will fail to resolve.
```

PlussMic's embedded SQL is a small, deliberately limited dialect, not a general
SQL engine: one `SELECT`, an optional single `INNER JOIN`, a `WHERE` clause that
is itself just a [`DataFrame::query`](api/dataframe/query.md) expression, at
most one `ORDER BY` key, and `LIMIT`/`OFFSET`. There is no `GROUP BY`, no
aggregate functions, no subqueries, no `UNION`, no multi-column `ORDER BY`, and
no `LEFT JOIN` (the parser explicitly rejects it — see
[Notes](#notes-execute)). Anything beyond that grammar is a parse error, not a
silent best-effort interpretation.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method section starts with the public signature(s), then explains what
  the method does, how each parameter changes behavior, and what the call
  returns.
- Examples build on one running dataset (`orders`, and a second frame
  `managers` for the JOIN examples) so you can compare outputs directly
  instead of re-reading a new setup every time.
- Each example is followed immediately by verified output hand-traced against
  `DataFrameSql.cpp`, so the result shape is visible before you move to the
  next one.

## E

### `execute`

#### Signature

```cpp
DataFrameSqlAccessor::execute(std::string_view statement) const -> DataFrame
```

#### What It Does

Renders any `{name}`/`{@name}` template placeholders into the statement text,
parses the result as a single `SELECT` statement (or reuses a cached parse —
see the plan-cache note below), executes it against `df` (the frame `sql()`
was called on) and any additional `sources`, and returns the projected rows as
a brand-new `DataFrame`. Execution proceeds in a fixed pipeline: resolve `FROM`
source → apply `JOIN` (if any) → apply `WHERE` (if any) → apply `ORDER BY` (if
any) → apply the `SELECT` projection → apply `OFFSET`/`LIMIT` → apply
`max_result_rows` truncation. Resource limits (`max_intermediate_rows`,
`memory_budget_mb`, `timeout_ms`) are checked at every one of those stages,
not just at the end.

#### Parameters

**statement** (`std::string_view`)

The SQL text. Supported grammar:

```text
SELECT <* | col[, col ...][ AS alias]>
FROM <source>[ AS alias]
[INNER JOIN <source>[ AS alias] ON <left> = <right>]
[WHERE <query()-expression>]
[ORDER BY <one column> [ASC|DESC] [NULLS FIRST|LAST]]
[LIMIT <n>]
[OFFSET <n>]
```

`source` is `df` (the calling frame) or a name bound via
`SqlOptions::sources`. `<left>`/`<right>` in a JOIN's `ON` clause are bare or
dotted (`alias.column`) identifiers separated by `=` with spaces around it —
`region = m.region`, not `region=m.region` (the tokenizer only splits on
whitespace/quotes/punctuation, so `=` glued to its neighbors becomes one
unparsable word). At least one side of `ON` must reference the joined
source's own alias/name.

#### Returns

`DataFrame`
    A new, freshly materialized frame — the projected, filtered, ordered,
    sliced result. Row case labels are whatever the last case-label-affecting
    stage produced (see the per-example notes below; a plain `WHERE` keeps the
    original labels of the surviving rows, a `JOIN` always resets to a fresh
    `0..n-1` index, per [`unify`](api/dataframe/unify.md)).

#### Examples

The dataset used throughout:

```cpp
using namespace pluss::table;

auto orders = DataFrame(
    {{"id",     Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}))},
     {"region", Thread(pluss::make_note({"east", "west", "east", "north", "west"}))},
     {"sales",  Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0, 250.0}))}});
std::cout << orders;
```

```text
=========================
      | id  region  sales
-------------------------
0     | 1   east    100  
1     | 2   west    200  
2     | 3   east    150  
3     | 4   north   90   
4     | 5   west    250  
-------------------------
[5 cs x 3 feat]
```

Example 1. A plain `SELECT` with a `WHERE` clause — the clause body is handed
directly to [`query()`](api/dataframe/query.md), so anything valid there is
valid here.

```cpp
auto out = orders.sql().execute("SELECT id, region, sales FROM df WHERE sales > 95");
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

The `id 4 / 90` row fails `sales > 95` and is dropped. The surviving rows keep
their **original** case labels (`0`, `1`, `2`, `4` — `3` is simply missing,
not renumbered), because a bare `WHERE` with no `JOIN` never touches the case
axis.

Example 2. `AS` renames a projected column; `ORDER BY ... DESC` sorts on it by
its **source** name, not the output alias.

```cpp
auto out2 = orders.sql().execute("SELECT id, sales AS amount FROM df ORDER BY sales DESC");
std::cout << out2;
```

##### Output

```text
==================
      | id  amount
------------------
4     | 5   250   
1     | 2   200   
2     | 3   150   
0     | 1   100   
3     | 4   90    
------------------
[5 cs x 2 feat]
```

`sales` is projected out under the name `amount`, but the `ORDER BY sales`
clause still refers to the underlying column — `ORDER BY amount` would fail
here since the projection hasn't happened yet when `ORDER BY` runs (ordering
happens before the final `SELECT` projection in the pipeline). Case labels
travel with their rows through the sort (`4` now leads, since `id 5` had the
highest `sales`).

Example 3. Scalar template parameters — `{name}` placeholders are substituted
with rendered SQL literals (quoted/escaped for strings, bare for numbers)
**before** parsing, so they behave exactly as if you'd typed the literal
yourself.

```cpp
auto opts = DataFrame::SqlOptions{
    .params = {{"region", std::string("west")}, {"min_sales", 100.0}},
};
auto out3 = orders.sql(opts).execute(
    "SELECT id, region, sales FROM df WHERE region = {region} AND sales >= {min_sales}");
std::cout << out3;
```

##### Output

```text
=========================
      | id  region  sales
-------------------------
1     | 2   west    200  
4     | 5   west    250  
-------------------------
[2 cs x 3 feat]
```

The rendered statement is literally
`SELECT id, region, sales FROM df WHERE region = 'west' AND sales >= 100` —
`{region}` became a quoted string literal, `{min_sales}` a bare number. By
default (`MissingParamPolicy::Error`), leaving out either key from `params`
throws instead of silently proceeding; passing
`MissingParamPolicy::Null` renders the literal token `null` in its place
instead (useful for building templates once and letting some placeholders
resolve to SQL `NULL` in certain calls — but you're then responsible for
writing a `WHERE` expression that tolerates a `NULL` literal on that side).

Example 4. Identifier template parameters — `{@name}` placeholders are
substituted with a backtick-quoted identifier, letting a column name itself be
data-driven (a scalar `{name}` can never do this — it always renders a
*value*, never a bare column reference).

```cpp
auto opts2 = DataFrame::SqlOptions{
    .identifier_params = {{"sort_col", "sales"}},
};
auto out4 = orders.sql(opts2).execute(
    "SELECT id, region, sales FROM df ORDER BY {@sort_col} ASC");
std::cout << out4;
```

##### Output

```text
=========================
      | id  region  sales
-------------------------
3     | 4   north   90   
0     | 1   east    100  
2     | 3   east    150  
1     | 2   west    200  
4     | 5   west    250  
-------------------------
[5 cs x 3 feat]
```

The rendered statement becomes ``ORDER BY `sales` ASC`` — the backtick
quoting is what lets the substituted identifier survive tokenizing even if it
happened to collide with a keyword. Unlike `{name}`, `{@name}` values are
validated to contain only identifier-safe characters (letters, digits, `_`,
`.`) and throw immediately if not.

Example 5. The `case` pseudo-column (`SqlOptions::case_axis_aliases`, default
`{"case"}`) projects the row's own case-label value as an ordinary output
column, combined with `LIMIT`/`OFFSET`.

```cpp
auto out5 = orders.sql().execute(
    "SELECT case, id, sales FROM df ORDER BY id DESC LIMIT 2 OFFSET 1");
std::cout << out5;
```

##### Output

```text
=======================
      | case  id  sales
-----------------------
3     | 3     4   90   
2     | 2     3   150  
-----------------------
[2 cs x 3 feat]
```

`ORDER BY id DESC` puts `id 5` first, `id 1` last; `OFFSET 1` then skips
`id 5`, and `LIMIT 2` keeps the next two (`id 4`, `id 3`). `case` only means
"project the case axis" when no *real* column named `case` exists — if
`orders` had an actual `case` feature, `SELECT case` would return that column
instead, ordinary-column names always take priority over the pseudo-column
alias.

Example 6. `INNER JOIN ... AS ... ON ... = ...` — the only join PlussMic's SQL
supports, built on [`DataFrame::unify`](api/dataframe/unify.md) underneath.

```cpp
auto managers = DataFrame(
    {{"region",  Thread(pluss::make_note({"east", "west", "north"}))},
     {"manager", Thread(pluss::make_note({"Frey", "Okafor", "Lund"}))}});
std::cout << managers;
```

```text
=======================
      | region  manager
-----------------------
0     | east    Frey   
1     | west    Okafor 
2     | north   Lund   
-----------------------
[3 cs x 2 feat]
```

```cpp
auto opts3 = DataFrame::SqlOptions{.sources = {{"managers", &managers}}};
auto out6 = orders.sql(opts3).execute(
    "SELECT id, region, sales, m.manager FROM df "
    "INNER JOIN managers AS m ON region = m.region "
    "ORDER BY id ASC");
std::cout << out6;
```

##### Output

```text
====================================
      | id  region  sales  m.manager
------------------------------------
0     | 1   east    100    Frey     
1     | 2   west    200    Okafor   
2     | 3   east    150    Frey     
3     | 4   north   90     Lund     
4     | 5   west    250    Okafor   
------------------------------------
[5 cs x 4 feat]
```

```{important} A joined-in source is prefixed *before* the key column is deduplicated
`managers`' columns are all renamed with an `m.` prefix — including its own
`region` key — before the join key match happens. Deduplication (dropping the
right side's copy of a shared key) only fires when the left and right key
**names** are exactly equal after that prefixing. Here the left key is
`region` and the right key is `m.region` — different names — so **both**
survive. `SELECT *` after this join would show `region` *and* `m.region` as
two separate, value-identical columns:
```

```cpp
auto out6b = orders.sql(opts3).execute(
    "SELECT * FROM df INNER JOIN managers AS m ON region = m.region ORDER BY id ASC");
std::cout << out6b;
```

```text
==============================================
      | id  region  sales  m.region  m.manager
----------------------------------------------
0     | 1   east    100    east      Frey     
1     | 2   west    200    west      Okafor   
2     | 3   east    150    east      Frey     
3     | 4   north   90     north     Lund     
4     | 5   west    250    west      Okafor   
----------------------------------------------
[5 cs x 5 feat]
```

Every `orders` region has exactly one `managers` match, so no rows fan out and
no rows drop — but note the case labels reset to a fresh `0..4`: `unify()`
never preserves either input's case labels, `JOIN` inherits that unconditionally.

Example 7. `SqlOptions::collision_policy` — what happens when two projected
columns land on the same output name. Same statement, three policies.

```cpp
auto dup_sql = "SELECT id AS x, region AS x FROM df";

try {
  auto bad = orders.sql(DataFrame::SqlOptions{}).execute(dup_sql);  // Error (default)
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

##### Output (`CollisionPolicy::Error`, the default)

```text
DataFrame::sql().execute: duplicate projected column name 'x'
```

```cpp
auto suffix_opts = DataFrame::SqlOptions{
    .collision_policy = DataFrame::SqlOptions::CollisionPolicy::Suffix,
};
auto out_suffix = orders.sql(suffix_opts).execute(dup_sql);
std::cout << out_suffix;
```

##### Output (`CollisionPolicy::Suffix`)

```text
================
      | x  x_2  
----------------
0     | 1  east 
1     | 2  west 
2     | 3  east 
3     | 4  north
4     | 5  west 
----------------
[5 cs x 2 feat]
```

```cpp
auto overwrite_opts = DataFrame::SqlOptions{
    .collision_policy = DataFrame::SqlOptions::CollisionPolicy::Overwrite,
};
auto out_overwrite = orders.sql(overwrite_opts).execute(dup_sql);
std::cout << out_overwrite;
```

##### Output (`CollisionPolicy::Overwrite`)

```text
=============
      | x    
-------------
0     | east 
1     | west 
2     | east 
3     | north
4     | west 
-------------
[5 cs x 1 feat]
```

`Suffix` keeps both, renaming the second to `x_2` (then `x_3`, and so on, if
still taken). `Overwrite` keeps only the *last* projection under that name —
`id`'s values are computed and then thrown away the instant `region AS x` is
projected — so plan the projection order accordingly if you rely on this
mode.

Example 8. Resource limits: `max_result_rows` truncates silently;
`max_intermediate_rows` throws. These are not the same kind of guard.

```cpp
auto trunc_opts = DataFrame::SqlOptions{.max_result_rows = 2};
auto out8 = orders.sql(trunc_opts).execute(
    "SELECT id, sales FROM df ORDER BY sales DESC");
std::cout << out8;
```

##### Output

```text
=================
      | id  sales
-----------------
4     | 5   250  
1     | 2   200  
-----------------
[2 cs x 2 feat]
```

`max_result_rows` slices the *final*, already-ordered result down to its
first N rows — it never throws, it just silently keeps less than you might
expect if you forget it's set.

```cpp
auto strict_opts = DataFrame::SqlOptions{.max_intermediate_rows = 2};
try {
  auto bad2 = orders.sql(strict_opts).execute("SELECT id, sales FROM df");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

##### Output

```text
DataFrame::sql().execute: initial rows exceed configured limit
```

`max_intermediate_rows` is checked at *every* pipeline stage (right after
resolving the source, after the join, after `WHERE`, after `ORDER BY`, after
the projection) — here `orders` itself already has 5 rows before anything
else runs, so it fails immediately at the "initial" checkpoint, before
`WHERE`/`JOIN`/`ORDER BY` ever get a chance to shrink it.

Example 9. `includes_calling_frame` — the statement must actually touch the
frame you called `.sql()` on somewhere in its `FROM`/`JOIN` chain.

```cpp
auto side_opts = DataFrame::SqlOptions{.sources = {{"managers", &managers}}};
try {
  auto bad3 = orders.sql(side_opts).execute("SELECT * FROM managers");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

##### Output

```text
DataFrame::sql().execute: calling dataframe source is not part of FROM/JOIN sources
```

`orders.sql(...)` is only a *handle* — the query still has to reference `df`
(directly, or transitively via a `JOIN`) somewhere. Querying only a bound
`sources` entry with no `df` in sight is rejected outright, even though the
bound frame is perfectly queryable on its own via its *own* `.sql()` call.

(notes-execute)=
#### Notes

- `LEFT JOIN` is parsed but explicitly rejected —
  `"DataFrame::sql().execute: LEFT JOIN is not supported yet (use INNER JOIN)"`.
  There is no working substitute inside SQL text; drop to
  [`DataFrame::unify`](api/dataframe/unify.md) directly (`how = "keep_left"`)
  if you need it, then continue in SQL against the result via a bound source.
- `ORDER BY` accepts exactly one column; there is no multi-column tiebreaker
  and no expression, only a bare (optionally qualified) identifier plus
  optional `ASC`/`DESC` and `NULLS FIRST`/`NULLS LAST`.
- `SqlOptions::result_mode = ResultMode::Chunked` currently has **no
  execution effect** — `execute()` still returns a fully materialized
  `DataFrame` either way, and merely emits a diagnostic warning noting the
  request was ignored. There is no streaming/chunked execution path yet.
- Diagnostics (`diagnostics_explain`, `diagnostics_profile`,
  `diagnostics_trace_tokens`, `diagnostics_trace_plan`) never appear in the
  returned `DataFrame` or on `std::cout` — they are emitted through
  PlussMic's internal warning log (`pluss::codem::warn`), a side channel
  separate from the result value.
- `enable_plan_cache` (default `true`) memoizes the *parsed plan* keyed by the
  exact rendered statement text (after `{}`/`{@}` substitution) — a statement
  re-rendered to the same text skips re-parsing automatically, with no
  `prepare()` required. This is why `prepare()`/`DataFrameSqlPrepared` (see
  [its reference page](dataframe_sql_prepared_api_reference.md)) offers no
  extra parsing-performance win beyond what repeated identical SQL text
  already gets for free — its value is purely the ergonomics of not
  re-typing/re-templating the same statement text at every call site.
- The primary table name is always `df`, case-insensitively. Passing anything
  else as the un-joined `FROM` source only works if it's a name bound via
  `SqlOptions::sources`.

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
DataFrameSqlAccessor::prepare(std::string_view statement) const -> DataFrameSqlPrepared
```

#### What It Does

Stores the given statement text verbatim, together with a copy of this
accessor's current frame and `SqlOptions`, inside a `DataFrameSqlPrepared`
value. **No parsing or validation happens at `prepare()` time** — the text is
only tokenized/parsed the first time it's actually rendered and executed (via
`DataFrameSqlPrepared::execute(...)`). `prepare()` exists purely so a
statement's *text* can be built once and reused across several calls with
different scalar/identifier bindings, without retyping (or re-templating) it
each time.

#### Parameters

**statement** (`std::string_view`)

The (optionally parameterized) SQL text to store. May contain `{name}` /
`{@name}` placeholders exactly like `execute()`'s argument — they're just not
resolved until the prepared value is actually executed.

#### Returns

`DataFrameSqlPrepared`
    A reusable handle bound to this frame, this accessor's `SqlOptions`, and
    the given statement text. See its own
    [API reference](dataframe_sql_prepared_api_reference.md) for the three
    `execute()` overloads it exposes.

#### Examples

Example 1. Prepare a plain, unparameterized statement and run it with
`DataFrameSqlPrepared::execute()` — no placeholders, no bindings, just a
stored statement you can re-run without retyping it.

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

`id 4 / 90` fails `sales >= 100` and drops — identical to what
`orders.sql().execute(...)` would produce with that same text in one shot.
`prepare()` changed nothing about the query itself, only when the text gets
typed.

Example 2. The real reason to reach for `prepare()`: bind a placeholder with
`DataFrameSqlPrepared::execute(params)` and reuse the **same** prepared value
with different values, with no re-templating.

```cpp
auto region_stmt = orders.sql().prepare(
    "SELECT id, region, sales FROM df WHERE region = {region}");

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

`region_stmt` itself is never modified by either call — each `execute(...)`
takes a private copy of the base options, merges in that call's `params`, and
discards the copy afterward. This is what `prepare()` is actually for: build
the statement's shape once (a loop over regions, a repeated per-request query
with a user-supplied filter, and so on), and vary only the bound values.

Example 3. `prepare()` performs no validation up front — a placeholder left
unbound is only ever caught inside `execute()`, at the point it's rendered.

```cpp
auto needs_param = orders.sql().prepare("SELECT id FROM df WHERE region = {region}");
// needs_param exists fine here -- nothing has been parsed or checked yet.

try {
  auto bad = needs_param.execute();  // no {region} binding supplied
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

##### Output

```text
DataFrame::sql().execute: SQL params missing key 'region'
```

`prepare()` happily stored the text verbatim with no complaint; the missing
`{region}` binding is only discovered — and only enforced against
`MissingParamPolicy` (`Error` by default, the throw seen here) — the moment
some `execute(...)` call actually tries to render it.

See the [`DataFrameSqlPrepared` API reference](dataframe_sql_prepared_api_reference.md)
for the full breadth of what `execute(...)` supports beyond these three
examples: combined scalar **and** identifier bindings in one call,
`reject_unused_params` catching a typo'd key, and a prepared statement that
carries a `JOIN`'s bound `sources` from `prepare()` time.

#### Notes

- `prepare()` never throws on a malformed or under-parameterized template by
  itself — every check (unknown placeholder keys, missing required params,
  unused params under `reject_unused_params`, the SQL grammar itself) happens
  inside the `execute()` call that follows, every time it's called.
- The captured `SqlOptions` (`sources`, `collision_policy`,
  `missing_param_policy`, resource limits, and so on) come from **whichever
  accessor `prepare()` was called on** — `frame.sql(options).prepare(stmt)`
  bakes `options` in as the base for every future execution; only `params`
  and `identifier_params` can still be layered on per-call, via
  `DataFrameSqlPrepared::execute(...)`'s arguments.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#execute">
    <span class="label">Previous</span>
    <span class="title">execute</span>
  </a>
</div>
