<!-- pluss-hand-authored -->

# Nexus::map_frames

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus Nexus::map_frames(const std::function<DataFrame(const DataFrame&)>& fn) const
```

## What It Does

Builds a new workspace by mapping each table through a callback and storing the
mapped result back under the same key.

Semantically, this is the functional-programming sibling of
[`apply_all()`](apply_all.md). Both methods walk every table in the workspace and
replace each one with the callback result. `map_frames()` simply emphasizes the
"map each frame to a new frame" mental model more explicitly.

Use it when you want the workspace-level equivalent of a table collection map:
same key set, same order, transformed `DataFrame` payloads.

## Parameters

**fn** (`const std::function<DataFrame(const DataFrame&)>&`)

Callback applied to each table.

- Must be non-empty.
- Receives each `DataFrame` by `const` reference.
- Must return a **defined** `DataFrame`.
- May change shape; `map_frames()` does not enforce row-count or feature-count
  stability.

## Returns

`Nexus`
    A new workspace containing one mapped replacement table for every source
    table, under the same keys and in the same order.

    The source `Nexus` is unchanged.

## Examples

Example 1. Map every table down to its first feature while preserving the key
set.

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

auto out = biz.map_frames([](const DataFrame& frame) {
  const auto names = frame.features();
  return frame.features({names.front()});
});

print_keys(out);
std::cout << out.frame("customers").nfeatures() << " "
          << out.frame("orders").nfeatures() << " "
          << out.frame("payments").nfeatures() << " "
          << out.frame("refunds").nfeatures();
```

### Output

```text
customers, orders, payments, refunds
1 1 1 1
```

Example 2. Continuing from Example 1, empty callbacks and undefined callback
results throw.

```cpp
try {
  std::function<DataFrame(const DataFrame&)> empty;
  auto bad = biz.map_frames(empty);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto bad = biz.map_frames([](const DataFrame&) {
    return DataFrame();
  });
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Nexus::map_frames: callback is empty
Nexus::map_frames: callback returned undefined DataFrame
```

## Notes

- `map_frames()` is non-mutating.
- It preserves workspace keys and order exactly.
- Like [`apply_all()`](apply_all.md), it preserves workspace metadata by table
  key rather than by transformed column signature. If the callback removes
  columns used by higher-level schema structures, review those structures after
  the transformation.
- Choose `map_frames()` when "map each frame to a new frame" is the clearest
  mental model. Choose [`apply_all()`](apply_all.md) when "apply one rule
  everywhere" reads more naturally at the call site.

## See Also

- [`Nexus::apply_all`](apply_all.md): workspace-wide table rewriting with the
  same core mechanics.
- [`Nexus::apply`](apply.md): targeted rewrite of one named table.
- [`Nexus::flow`](flow.md): whole-workspace callback composition.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="materialize.html">
    <span class="label">Next</span>
    <span class="title">materialize</span>
  </a>
</div>
