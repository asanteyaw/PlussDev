<!-- pluss-hand-authored -->

# Nexus::head

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus Nexus::head(std::int64_t n = 5) const
```

## What It Does

Returns a new workspace containing only the first `n` tables of the current
`Nexus`, in their existing left-to-right order.

`head()` is the simplest way to trim a larger multi-table workspace down to an
exploratory prefix without renaming anything, recomputing anything, or mutating
the source handle. It is purely positional: the method does not inspect table
names or contents; it simply keeps the first `n` entries from
[`keys()`](keys.md)/[`frames()`](frames.md) order.

Two behaviors matter most in practice:

- `n` is **clamped**, not treated as a strict validation boundary. Asking for
  more tables than exist gives you the whole workspace, while `0` or a negative
  count gives you an empty workspace.
- Surviving schema attachments are carried forward for retained tables. Table
  metadata survives for the tables that remain, and relationships or saved views
  survive only if they are still fully supported by the retained key set.

## Parameters

**n** (`std::int64_t`, default `5`)

Number of leading tables to keep.

- `n > ntables()`: clamped to `ntables()`, so the whole workspace is returned.
- `n == 0`: returns a defined but empty `Nexus`.
- `n < 0`: also clamps to `0`, so the result is again a defined but empty
  workspace.

There is no bounds error here.

## Returns

`Nexus`
    A new workspace containing the first `n` tables from the source workspace,
    in the same order they originally appeared.

    The source `Nexus` is unchanged.

## Examples

Example 1. Keep the first two tables.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto print_keys = [](const Nexus& nx) {
  for (std::int64_t i = 0; i < nx.ntables(); ++i) {
    if (i > 0) {
      std::cout << ", ";
    }
    std::cout << nx.keys()[static_cast<std::size_t>(i)];
  }
  std::cout << "\n";
};

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto orders = DataFrame({
    {"order_id",    Slate(pluss::make_series({101.0, 102.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 2.0}))},
});

auto payments = DataFrame({
    {"payment_id", Slate(pluss::make_series({9001.0, 9002.0}))},
    {"order_id",   Slate(pluss::make_series({101.0, 102.0}))},
});

auto refunds = DataFrame({
    {"refund_id", Slate(pluss::make_series({77.0}))},
    {"order_id",  Slate(pluss::make_series({102.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
    {"payments", payments},
    {"refunds", refunds},
});

auto out = biz.head(2);
print_keys(out);
std::cout << biz.ntables() << " " << out.ntables();
```

### Output

```text
customers, orders
4 2
```

Example 2. Continuing from Example 1, zero and negative counts collapse to an
empty workspace rather than throwing.

```cpp
std::cout << biz.head(0).ntables() << " " << biz.head(-4).ntables();
```

### Output

```text
0 0
```

Example 3. Continuing from Example 1, asking for more tables than exist simply
returns the whole workspace.

```cpp
auto out = biz.head(99);
for (std::int64_t i = 0; i < out.ntables(); ++i) {
  if (i > 0) std::cout << ", ";
  std::cout << out.keys()[static_cast<std::size_t>(i)];
}
```

### Output

```text
customers, orders, payments, refunds
```

## Notes

- `head()` is non-mutating.
- Order is preserved exactly. `head(2)` does not sort or otherwise reinterpret
  the workspace; it just keeps the first two current positions.
- Surviving per-table metadata is carried forward for retained tables.
- Relationships and saved views are filtered structurally: only those whose
  referenced tables are all still present survive into the result.
- For the symmetric operation from the end of the workspace, use
  [`tail(n)`](tail.md).

## See Also

- [`Nexus::tail`](tail.md): keep the last `n` tables instead of the first.
- [`Nexus::select`](select.md): keep explicitly named tables, not just a prefix.
- [`Nexus::keys`](keys.md): inspect the current table order that `head()` slices
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_table_meta.html">
    <span class="label">Previous</span>
    <span class="title">has_table_meta</span>
  </a>
  <a class="next-link" href="inject.html">
    <span class="label">Next</span>
    <span class="title">inject</span>
  </a>
</div>
