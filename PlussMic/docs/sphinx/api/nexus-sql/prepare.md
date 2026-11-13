<!-- pluss-hand-authored -->

# NexusQuery::prepare

[Back to NexusQuery API Reference](../../nexus_query_api_reference)

## Signature

```cpp
NexusQueryPrepared NexusQuery::prepare(std::string_view statement) const
```

## What It Does

Builds a reusable prepared workspace query from a SQL statement template,
returned as a `NexusQueryPrepared`. The statement is parsed once; its own
[`.execute(...)`](prepared-execute.md) overloads then bind runtime values
(and, separately, runtime identifiers) into the same template on every
call, instead of re-interpolating a new statement string by hand each time
— see [`NexusQueryPrepared::execute`](prepared-execute.md) for the full
placeholder grammar (`{name}` value placeholders, `{@name}` identifier
placeholders, `{{` for a literal brace).

`prepare()` itself does not touch the workspace or validate the statement
against it — statement text and brace-balance are checked here, but
whether a named table/column actually exists is only checked once
`.execute(...)` runs.

## Parameters

**statement** (`std::string_view`)
    A SQL statement template, with `{name}`/`{@name}` placeholders anywhere
    a bound value or identifier belongs.

## Returns

`NexusQueryPrepared`
    A reusable prepared query bound to this workspace and statement
    template.

## Examples

Example 1. Prepare once, then execute with a bound threshold.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto orders = DataFrame({
    {"order_id", pluss::make_series({100.0, 101.0, 102.0})},
    {"customer_id", pluss::make_series({1.0, 2.0, 2.0})},
    {"amount", pluss::make_series({50.0, 75.0, 20.0})},
});
auto nx = Nexus({{"orders", orders}});

auto stmt = nx.query().prepare(
    "SELECT orders.amount "
    "FROM orders "
    "WHERE orders.amount >= {min_amount} "
    "ORDER BY orders.amount DESC");

auto out = stmt.execute({{"min_amount", 50.0}});
std::cout << out.to_string();
```

### Output

```text
     | orders.amount
--------------------
0    | 75
1    | 50
--------------------
[2 cs x 1 feat]
```

## Notes

- `prepare()` is cheap and side-effect-free relative to `.execute(...)`:
  build the `NexusQueryPrepared` once, near where the statement text is
  authored, and keep it around for as long as you intend to bind and
  re-execute it.
- See [`NexusQueryPrepared::execute`](prepared-execute.md) for the full
  placeholder grammar and worked examples of both value and identifier
  binding.

## See Also

- [`NexusQueryPrepared::execute`](prepared-execute.md): run the prepared
  statement, with or without bound parameters.
- [`NexusQuery::execute`](execute.md): the one-shot equivalent, for a
  statement that only runs once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="execute.html">
    <span class="label">Previous</span>
    <span class="title">execute</span>
  </a>
  <a class="next-link" href="prepared-execute.html">
    <span class="label">Next</span>
    <span class="title">prepared-execute</span>
  </a>
</div>
