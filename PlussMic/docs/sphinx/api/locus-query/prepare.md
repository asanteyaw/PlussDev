<!-- pluss-hand-authored -->

# LocusQuery::prepare

[Back to LocusQuery API Reference](../../locus_query_api_reference)

## Signature

```cpp
LocusQueryPrepared LocusQuery::prepare(std::string_view statement) const
```

## What It Does

`prepare()` packages statement text, the current aligned workspace handle, and a
copy of the accessor's SQL options into a reusable `LocusQueryPrepared`. It is a
binding and ownership abstraction: runtime values can be supplied without
rebuilding SQL strings, and the prepared object remains valid independently of
the local `LocusQuery` variable.

The name does not imply an open database cursor. Locus creates an in-memory SQL
workspace for each execution, so this call does not compile the statement or
resolve table and stream names. Those checks occur in
[`LocusQueryPrepared::execute()`](../locus-query-prepared/execute.md). This lazy
boundary lets a template be stored before all runtime values are known, but it
also means malformed SQL and unknown catalogs are execution errors.

Templates recognize `{name}` for values and `{@name}` for identifiers. Literal
opening or closing braces are written as `{{` and `}}`. Placeholder parsing also
happens at execution, alongside binding.

## Parameter

**statement** (`std::string_view`)
: Owned statement-template text. The caller's string storage need not outlive
  this call.

## Returns

`LocusQueryPrepared`
: A reusable, workspace-owning template. Continue in the
  [LocusQueryPrepared API Reference](../../locus_query_prepared_api_reference.md).

## Examples

Example 1. Store a value template and execute it repeatedly with different
thresholds.

```cpp
const auto threshold = data.query().prepare(
    "SELECT order_id FROM orders WHERE amount >= {floor} ORDER BY order_id");
print_numbers(threshold.execute({{"floor", 50.0}}), "order_id");
print_numbers(threshold.execute({{"floor", 80.0}}), "order_id");
```

### Output

```text
order_id=[100, 101, 103]
order_id=[103]
```

Example 2. Preparation is lazy: storing a template that names an absent table
succeeds, and execution reports the schema failure.

```cpp
const auto lazy = data.query().prepare("SELECT id FROM absent_table");
std::cout << "prepared\n";
print_error("on execute", [&] { (void)lazy.execute(); });
```

### Output

```text
prepared
on execute: DataFrame::sql().execute: sqlite prepare failed: no such table: absent_table
```

Example 3. A prepared object created from a temporary workspace owns everything
except any external `SourceBinding` pointers supplied in options.

```cpp
const auto fixed = shop().query().prepare(
    "SELECT order_id FROM orders ORDER BY order_id DESC LIMIT 2");
print_numbers(fixed.execute(), "order_id");
```

### Output

```text
order_id=[103, 102]
```

## When To Use It

Use `prepare()` to centralize authored template text, enforce typed value versus
identifier bindings, and retain query options across calls. Do not choose it on
the assumption that a native SQL statement object is compiled once and reused;
the current Locus execution model rebuilds the transient workspace per call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="execute.html"><span class="label">Previous</span><span class="title">execute</span></a>
  <a class="next-link" href="../../locus_query_api_reference.html"><span class="label">Next</span><span class="title">LocusQuery API Reference</span></a>
</div>

