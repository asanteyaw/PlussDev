<!-- pluss-hand-authored -->

# Nexus::tail

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::tail(std::int64_t n = 5) const -> Nexus
```

## What It Does

Returns a new workspace containing only the last `n` tables of the current
`Nexus`, preserving their existing relative order.

`tail()` is the mirror image of [`head()`](head.md): where `head()` keeps a
prefix, `tail()` keeps a suffix. It is still positional, still non-mutating, and
still simple to reason about: table names and contents do not affect which
tables survive; only the current workspace order does.

Like `head()`, this method carries forward surviving schema attachments for the
kept tables. Per-table metadata is preserved for tables that remain, while
relationships and saved views survive only when every table they depend on is
still present in the suffix.

## Parameters

**n** (`std::int64_t`, default `5`)

Number of trailing tables to keep.

- `n > ntables()`: clamped down to the full workspace size.
- `n == 0`: returns a defined but empty `Nexus`.
- `n < 0`: clamps to `0`, again yielding an empty workspace.

There is no out-of-range exception here.

## Returns

`Nexus`
    A new workspace containing the last `n` tables from the source workspace, in
    the same order they originally appeared.

    The source `Nexus` is unchanged.

## Examples

Example 1. Keep the last two tables.

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

auto out = biz.tail(2);
print_keys(out);
std::cout << biz.ntables() << " " << out.ntables();
```

### Output

```text
payments, refunds
4 2
```

Example 2. Continuing from Example 1, zero and negative counts yield an empty
workspace instead of throwing.

```cpp
std::cout << biz.tail(0).ntables() << " " << biz.tail(-4).ntables();
```

### Output

```text
0 0
```

Example 3. Continuing from Example 1, asking for more tables than exist returns
the whole workspace.

```cpp
auto out = biz.tail(99);
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

- `tail()` is non-mutating; the source workspace keeps all of its tables.
- The result keeps the original relative order of the retained suffix. For
  example, the last two tables are returned as `"payments", "refunds"`, not in
  reverse.
- Per-table metadata survives for retained tables.
- Relationships and saved views are kept only when their required tables are all
  still present after the suffix slice.
- For a prefix slice instead, use [`head(n)`](head.md).

## See Also

- [`Nexus::head`](head.md): keep the first `n` tables instead of the last.
- [`Nexus::drop`](drop.md): remove explicitly named tables instead of slicing by
  position.
- [`Nexus::keys`](keys.md): inspect the order `tail()` slices from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="table_meta.html">
    <span class="label">Previous</span>
    <span class="title">table_meta</span>
  </a>
  <a class="next-link" href="to_atlas.html">
    <span class="label">Next</span>
    <span class="title">to_atlas</span>
  </a>
</div>
