<!-- pluss-hand-authored -->

# Nexus::select

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::select(std::initializer_list<std::string_view> keys) const -> Nexus
```

## What It Does

Builds a new workspace containing exactly the named tables you request, in the
exact order you request them.

This makes `select()` more than a simple filter. It is both:

- a **projection** operation, because it drops every unmentioned table, and
- a **reordering** operation, because the result follows the request list rather
  than the source workspace's original order.

That second point is the easy one to miss. If the source workspace is
`customers, orders, payments, refunds`, then `select({"payments", "customers"})`
does not preserve source order among the survivors; it intentionally returns
`payments, customers`.

Like the other subset-building methods, `select()` carries forward metadata for
surviving tables and keeps only the relationships and saved views that remain
valid on the selected key set.

## Parameters

**keys** (`std::initializer_list<std::string_view>`)

The table names to keep.

- Each key must be non-empty.
- Each key must be unique within the request itself.
- Each key must already exist in the source workspace.
- Request order becomes output order.

An empty list is allowed and produces a defined but empty workspace.

## Returns

`Nexus`
    A new workspace containing exactly the requested tables, in request order.

    The source `Nexus` is unchanged.

## Examples

Example 1. Keep two named tables and reorder them deliberately.

```cpp
using namespace pluss::table;

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
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
});

auto payments = DataFrame({
    {"payment_id", Thread(pluss::make_series({9001.0, 9002.0}))},
    {"order_id",   Thread(pluss::make_series({101.0, 102.0}))},
});

auto refunds = DataFrame({
    {"refund_id", Thread(pluss::make_series({77.0}))},
    {"order_id",  Thread(pluss::make_series({102.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
    {"payments", payments},
    {"refunds", refunds},
});

auto out = biz.select({"payments", "customers"});
print_keys(out);
print_keys(biz);
```

### Output

```text
payments, customers
customers, orders, payments, refunds
```

Example 2. Continuing from Example 1, selecting an empty key list produces an
empty but still defined workspace.

```cpp
std::cout << biz.select({}).ntables();
```

### Output

```text
0
```

Example 3. Continuing from Example 1, validation is strict: empty request keys,
duplicate request keys, and unknown keys all throw.

```cpp
try {
  auto bad = biz.select({"", "orders"});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto dup = biz.select({"orders", "orders"});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto missing = biz.select({"orders", "missing"});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Nexus::select: key cannot be empty
Nexus::select: duplicate key 'orders'
Nexus::select: unknown key 'missing'
```

## Notes

- `select()` is non-mutating.
- Request order controls output order. This is the key semantic difference from
  [`drop()`](drop.md), which preserves the original order of survivors.
- Surviving table metadata is preserved.
- Relationships and saved views are pruned to the subset that remains fully
  supported by the selected tables.
- If you naturally think in terms of "remove these few tables" rather than "keep
  exactly these tables", [`drop()`](drop.md) is often the shorter expression.

## See Also

- [`Nexus::drop`](drop.md): remove named tables and keep everything else.
- [`Nexus::head`](head.md): keep a positional prefix instead of an explicit key
  list.
- [`Nexus::keys`](keys.md): inspect the final order after projection.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scan.html">
    <span class="label">Previous</span>
    <span class="title">scan</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
