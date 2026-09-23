<!-- pluss-hand-authored -->

# Nexus::neighbors

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::neighbors(std::string_view table) const -> DataFrame
```

## What It Does

Lists every relationship directly touching a table — both relationships declared *from*
it and relationships declared *to* it — as a `DataFrame`, one row per edge, from that
table's point of view. This is a one-hop view into the same relationship graph
[`path()`](path.md) searches across multiple hops.

## Parameters

**table** (`std::string_view`)
    The table to inspect. Must already exist in the workspace.

## Returns

`DataFrame`
    One row per relationship touching `table`, with columns:

    - `table` — the anchor table (repeated on every row; always equal to the `table`
      argument).
    - `relationship` — the relationship's declared name.
    - `other` — the table on the far end of the edge.
    - `cardinality` — the relationship's cardinality, reoriented if needed so it reads
      correctly from `table`'s perspective (see `reversed` below).
    - `keys` — the join key pairs, as `"left -> right"` strings, also reoriented.
    - `reversed` — `true` if this relationship was declared with `table` as
      `right_table` rather than `left_table`. When `reversed` is `true`, `cardinality` and
      `keys` are flipped so they still read correctly as "from `table`'s perspective",
      rather than forcing you to mentally invert a `left_table`-oriented declaration.
    - `from_optional`/`to_optional` — whether the `table` side, and the `other` side, are
      each allowed to go unmatched (reoriented the same way as `cardinality`).
    - `confidence`, `status`, `note` — echoed from the declaration.

    Empty (but defined) if no relationship touches `table` at all.

## Examples

Example 1. A table with two relationships — one declared from it, one declared to it.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 2.0}))},
});
auto payments = DataFrame({
    {"payment_id", Thread(pluss::make_series({9001.0, 9002.0}))},
    {"order_id",   Thread(pluss::make_series({101.0, 102.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
    {"payments", payments},
});

Relationship orders_to_customers;
orders_to_customers.name = "orders_to_customers";
orders_to_customers.left_table = "orders";
orders_to_customers.right_table = "customers";
orders_to_customers.keys = {{"customer_id", "customer_id"}};

Relationship payments_to_orders;
payments_to_orders.name = "payments_to_orders";
payments_to_orders.left_table = "payments";
payments_to_orders.right_table = "orders";
payments_to_orders.keys = {{"order_id", "order_id"}};

auto linked = biz.add_relationships({orders_to_customers, payments_to_orders});

auto n = linked.neighbors("orders");
std::cout << n.features({"table", "relationship", "other", "reversed"});
```

### Output

```text
=======================================================
     | table   relationship         other      reversed
-------------------------------------------------------
0    | orders  orders_to_customers  customers  false
1    | orders  payments_to_orders   payments   true
-------------------------------------------------------
[2 cs x 4 feat]
```

`orders` is `orders_to_customers`'s own `left_table`, so that edge shows `reversed = false`.
It's `payments_to_orders`'s `right_table` instead, so that edge shows `reversed = true` —
the relationship still reads correctly (`orders` connects to `payments`), just noting the
declaration ran the other way.

Example 2. A table with no relationships declared to or from it returns an empty frame.

```cpp
auto standalone = DataFrame({{"note", Thread(pluss::make_note({"unrelated"}))}});
auto with_standalone = linked.inject("standalone", standalone);

std::cout << with_standalone.neighbors("standalone").ncases();
```

### Output

```text
0
```

## Notes

- Reorients `cardinality`, `keys`, and the optional flags so every row reads correctly
  from `table`'s own perspective, regardless of whether the underlying relationship was
  declared with `table` as `left_table` or `right_table`.
- Results are cached internally per table.
- One row per direct edge — does not traverse multiple hops. For that, see
  [`path()`](path.md).

## See Also

- [`Nexus::path`](path.md): find a multi-hop route between two tables, built from the same
  underlying adjacency this method exposes one hop at a time.
- [`Nexus::relationships`](relationships.md): every declared relationship in the workspace,
  unfiltered by table.
- [`Nexus::graph`](graph.md): the full relationship graph as a `DataFrame`, one row per
  relationship rather than one row per edge-from-a-given-table.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="names.html">
    <span class="label">Previous</span>
    <span class="title">names</span>
  </a>
  <a class="next-link" href="nrelationships.html">
    <span class="label">Next</span>
    <span class="title">nrelationships</span>
  </a>
</div>
