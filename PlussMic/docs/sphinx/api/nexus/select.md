<!-- pluss-hand-authored -->

# Nexus::select

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::select(std::initializer_list<std::string_view> keys) const -> Nexus
Nexus::select(pluss::msharp::RangeView::Of<std::string_view> keys) const -> Nexus
template <detail::StringViewNameRange NameRange>
Nexus::select(const NameRange& keys) const -> Nexus
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

**keys** (a range of `std::string_view` — a brace literal, a named
container such as `std::vector<std::string_view>`/`std::vector<std::string>`,
or any other range whose elements convert to `std::string_view`)

The table names to keep, in iteration order.

- Each key must be non-empty.
- Each key must be unique within the request itself.
- Each key must already exist in the source workspace.
- **Iteration order becomes output order** — not necessarily *your* intended
  order. A brace literal or `std::vector` iterates in the order you wrote it;
  an *ordered* container like `std::set` iterates in *its own* sorted order
  regardless of how you inserted into it — see Example 4.

An empty range is allowed and produces a defined but empty workspace.

```{note} `pluss::msharp::RangeView` is never spelled out in these examples
`RangeView::Of<std::string_view>` has implicit constructors from a brace
list, a `std::vector<std::string_view>`, and other contiguous containers, so
passing any of those at a call site works without ever naming `RangeView`
yourself. Example 1 below uses a brace literal (the dedicated
`initializer_list` overload exists specifically to keep that call
unambiguous); Examples 2 and 3 pass a named container instead, to exercise
the `RangeView` and templated overloads respectively without naming either.
```

## Returns

`Nexus`
    A new workspace containing exactly the requested tables, in request order.

    The source `Nexus` is unchanged.

## Examples

Example 1. Keep two named tables and reorder them deliberately.

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

Example 2. Continuing from Example 1: the same request via a named
`std::vector<std::string_view>` — the `RangeView` overload, reached without
naming `RangeView` anywhere.

```cpp
std::vector<std::string_view> keep = {"payments", "customers"};
auto out = biz.select(keep);
for (std::int64_t i = 0; i < out.ntables(); ++i) {
  if (i > 0) std::cout << ", ";
  std::cout << out.keys()[static_cast<std::size_t>(i)];
}
```

### Output

```text
payments, customers
```

Example 3. Continuing from Example 1: a named `std::vector<std::string>`
(owning strings, not views) reaches the templated overload instead, since
`RangeView`'s vector constructor requires the exact element type
`std::string_view` — `std::vector<std::string>` doesn't match it, but does
satisfy the broader `StringViewNameRange` concept. Same result either way.

```cpp
std::vector<std::string> keep = {"payments", "customers"};
auto out = biz.select(keep);
for (std::int64_t i = 0; i < out.ntables(); ++i) {
  if (i > 0) std::cout << ", ";
  std::cout << out.keys()[static_cast<std::size_t>(i)];
}
```

### Output

```text
payments, customers
```

Example 4. The iteration-order gotcha: a `std::set<std::string_view>` also
reaches the templated overload, but a `std::set` iterates in *sorted* order —
not insertion order — so the request `{"payments", "customers"}` produces
`customers, payments`, not `payments, customers`.

```cpp
std::set<std::string_view> keep = {"payments", "customers"};
auto out = biz.select(keep);
for (std::int64_t i = 0; i < out.ntables(); ++i) {
  if (i > 0) std::cout << ", ";
  std::cout << out.keys()[static_cast<std::size_t>(i)];
}
```

### Output

```text
customers, payments
```

Example 5. Continuing from Example 1, selecting an empty key list produces an
empty but still defined workspace.

```cpp
std::cout << biz.select({}).ntables();
```

### Output

```text
0
```

Example 6. Continuing from Example 1, validation is strict: empty request keys,
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
  <a class="prev-link" href="schema.html">
    <span class="label">Previous</span>
    <span class="title">schema</span>
  </a>
  <a class="next-link" href="set_table_meta.html">
    <span class="label">Next</span>
    <span class="title">set_table_meta</span>
  </a>
</div>
