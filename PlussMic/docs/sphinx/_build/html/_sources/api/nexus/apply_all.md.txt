<!-- pluss-hand-authored -->

# Nexus::apply_all

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::apply_all(const std::function<DataFrame(const DataFrame&)>& fn) const -> Nexus
```

## What It Does

Rewrites every table in the workspace by applying the same callback to each
table, returning a new `Nexus` with the transformed tables under the original
keys.

`apply_all()` is the broad "rewrite the whole workspace uniformly" counterpart to
[`apply(key, fn)`](apply.md). The callback sees each table independently; the
method itself preserves the workspace key set and key order.

This method overlaps intentionally with [`map_frames()`](map_frames.md). Their
core transformation behavior is the same, but `apply_all()` reads more naturally
when your mental model is "apply one table-rewrite rule everywhere", while
`map_frames()` reads more naturally when you are thinking in terms of functional
mapping.

## Parameters

**fn** (`const std::function<DataFrame(const DataFrame&)>&`)

Callback applied to each table in the workspace.

- Must be non-empty.
- Receives each `DataFrame` by `const` reference.
- Must return a **defined** `DataFrame` for every table.
- May change table shape; `apply_all()` does not require schema preservation.

## Returns

`Nexus`
    A new workspace containing one transformed replacement table for every source
    table, under the same keys and in the same order.

    The source `Nexus` is unchanged.

## Examples

Example 1. Apply the same row-trimming rule to every table.

```cpp
using namespace pluss::table;

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

auto out = biz.apply_all([](const DataFrame& frame) {
  return frame.head(1, false);
});

std::cout << out.frame("customers").ncases() << " "
          << out.frame("orders").ncases() << " "
          << out.frame("payments").ncases() << " "
          << out.frame("refunds").ncases();
```

### Output

```text
1 1 1 1
```

Example 2. Continuing from Example 1, empty callbacks and undefined callback
results throw with `apply_all`-specific messages.

```cpp
try {
  std::function<DataFrame(const DataFrame&)> empty;
  auto bad = biz.apply_all(empty);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto bad = biz.apply_all([](const DataFrame&) {
    return DataFrame();
  });
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Nexus::apply_all: callback is empty
Nexus::apply_all: callback returned undefined DataFrame
```

## Notes

- `apply_all()` is non-mutating.
- Table keys and their order are preserved exactly.
- The callback may change each table's shape; there is no row-count or
  feature-count contract imposed by `apply_all()`.
- Like [`apply()`](apply.md), this method preserves workspace metadata by table
  key rather than by column signature. If your callback removes columns that
  relationships or saved views depend on, review those structures afterward.
- `apply_all()` and [`map_frames()`](map_frames.md) are behaviorally near-twins;
  the difference is mostly naming intent and, now, their method-specific error
  messages.

## See Also

- [`Nexus::apply`](apply.md): rewrite one named table instead of every table.
- [`Nexus::map_frames`](map_frames.md): the same workspace-wide rewrite idea with
  functional naming.
- [`Nexus::flow`](flow.md): transform the whole workspace with a callback that
  receives the entire `Nexus`, not one table at a time.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply.html">
    <span class="label">Previous</span>
    <span class="title">apply</span>
  </a>
  <a class="next-link" href="deduce.html">
    <span class="label">Next</span>
    <span class="title">deduce</span>
  </a>
</div>
