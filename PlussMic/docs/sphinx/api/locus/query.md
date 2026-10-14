<!-- pluss-hand-authored -->

# Locus::query

[Back to Locus API Reference](../../locus_api_reference)

## Signatures

```cpp
LocusQuery Locus::query() const
LocusQuery Locus::query(pluss::table::DataFrame::SqlOptions options) const
```

## What It Establishes

`query()` exposes every named catalog as a table in one in-memory SQL workspace
and returns a `LocusQuery`. The call obtains Locus's cached aligned `Nexus` view,
then the accessor owns that workspace handle and a copy of its options. No SQL is
parsed and no result is allocated at this point.

Continue in the [LocusQuery API Reference](../../locus_query_api_reference.md):
use `execute()` for a one-shot statement or `prepare()` to retain statement text
and bind values repeatedly. Results return as `Catalog`, not `DataFrame`, so the
query boundary stays inside the ragged data model even though SQL execution uses
rectangular alignment internally.

The no-argument overload uses default SQL policy. The options overload captures
limits, parameter defaults, diagnostics, collision policy, and execution policy
for every later call made through that accessor or a prepared query derived from
it.

## SQL Options

| Field | Meaning | Default |
|---|---|---|
| `primary_source_names` | Additional names for the seed table. Locus catalog keys remain available independently. | empty |
| `sources` | Additional named frames added beside the Locus catalogs. | empty |
| `params` | Default `{name}` value bindings. Runtime prepared bindings replace entries with the same key. | empty |
| `identifier_params` | Default `{@name}` identifier bindings. | empty |
| `missing_param_policy` | Either reject a missing binding or substitute SQL `NULL`. | `Error` |
| `reject_unused_params` | Reject supplied bindings not consumed by the template. | `false` |
| `collision_policy` | Policy for duplicate output names: error, suffix, or overwrite. | `Error` |
| `max_intermediate_rows` | Optional cap used while constructing intermediate SQL data. | unset |
| `max_result_rows` | Maximum rows retained in the returned result. | unset |
| `memory_budget_mb` | Optional positive memory budget for SQL execution. | unset |
| `timeout_ms` | Optional non-negative execution deadline in milliseconds. | unset |
| `diagnostics_explain` | Emit the query plan. | `false` |
| `diagnostics_profile` | Emit elapsed time and result-row count. | `false` |
| `diagnostics_trace_tokens` | Emit lexical SQL tokens. | `false` |
| `diagnostics_trace_plan` | Emit the query plan through tracing. | `false` |
| `enable_plan_cache` | Permit SQL plan caching. | `true` |
| `plan_cache_capacity` | Maximum cached plans. | `256` |
| `result_mode` | Request materialized or chunk-oriented execution. `execute()` still returns a materialized `Catalog`. | `Materialized` |
| `chunk_rows` | Requested chunk size when chunk mode is selected. | `1024` |
| `case_axis_aliases` | Names that may refer to the aligned case axis. | `{"case"}` |

## Returns

`LocusQuery`
: A workspace-owning accessor with copied options. It remains valid when created
  from a temporary Locus. `SqlOptions::sources` is the exception: each
  `SourceBinding` contains a borrowed `DataFrame*`, so externally added frames
  must outlive execution through the accessor.

## Examples

Example 1. Default options expose all catalog keys as SQL table names.

```cpp
auto default_query = data.query();
const auto all = default_query.execute("SELECT order_id FROM orders ORDER BY order_id");
print_numbers(all, "order_id");
```

### Output

```text
order_id=[100, 101, 102, 103]
```

Example 2. Options are captured by value. Here the result-row cap retains the
first two rows after SQL ordering.

```cpp
SqlOptions limited;
limited.max_result_rows = 2;
const auto first_two = data.query(limited).execute(
    "SELECT order_id FROM orders ORDER BY order_id");
print_numbers(first_two, "order_id");
```

### Output

```text
order_id=[100, 101]
```

Example 3. The accessor owns the workspace needed for later execution.

```cpp
const auto retained = shop().query();
print_words(retained.execute(
    "SELECT name FROM customers ORDER BY customer_id"), "name");
```

### Output

```text
name=[Ada, Bola, Chen]
```

## Cost And Ownership

Creating the accessor copies options and a reference-counted aligned workspace
handle. Locus caches that alignment by catalog and policy, so unchanged catalogs
are not padded again for each accessor. Building and loading the transient SQL
database remains deferred to execution. Keep the accessor when several related
statements share the same options; use a prepared query when the statement text
itself also repeats.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pull.html"><span class="label">Previous</span><span class="title">pull</span></a>
  <a class="next-link" href="../../locus_api_reference.html"><span class="label">Next</span><span class="title">Locus API Reference</span></a>
</div>
