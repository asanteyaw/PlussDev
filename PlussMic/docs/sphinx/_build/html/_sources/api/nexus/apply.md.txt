<!-- pluss-hand-authored -->

# Nexus::apply

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::apply(std::string_view key, const std::function<DataFrame(const DataFrame&)>& fn) const -> Nexus
```

## What It Does

Rewrites exactly one named table inside the workspace by passing that table
through a callback and replacing only that table with the callback result.

This is the targeted transformation surface for `Nexus`: when you know *which*
table should change and want every other table left alone, `apply()` is more
precise than [`apply_all()`](apply_all.md) or [`map_frames()`](map_frames.md).

The method is non-mutating. It returns a new `Nexus` whose table keys are the
same as the source workspace, with only the named table's `DataFrame` payload
replaced by the callback result.

## Parameters

**key** (`std::string_view`)

Name of the table to transform.

- Must not be empty.
- Must already exist in the workspace.
- Determines *which* table is rewritten; it does not affect ordering.

**fn** (`const std::function<DataFrame(const DataFrame&)>&`)

Callback that receives the current table and returns the replacement table.

- Must be non-empty.
- Receives the current `DataFrame` by `const` reference.
- Must return a **defined** `DataFrame`.
- May change the table's row count, feature set, feature order, labels, and
  values; `apply()` does not impose shape preservation.

## Returns

`Nexus`
    A new workspace with the named table replaced by the callback result and all
    other tables preserved as they were.

    The source `Nexus` is unchanged.

## Examples

Example 1. Rewrite one named table and leave every other table alone.

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

auto out = biz.apply("orders", [](const DataFrame& frame) {
  return frame.head(1, false);
});

print_keys(out);
std::cout << biz.frame("orders").ncases() << " " << out.frame("orders").ncases();
```

### Output

```text
customers, orders, payments, refunds
2 1
```

Example 2. Continuing from Example 1, the callback can also change the target
table's schema, not just its values.

```cpp
auto out = biz.apply("customers", [](const DataFrame& frame) {
  return frame.features({"name"});
});

std::cout << out.frame("customers").nfeatures() << " "
          << out.frame("orders").nfeatures();
```

### Output

```text
1 2
```

Example 3. Continuing from Example 1, validation is strict: empty callbacks,
unknown keys, and undefined callback results all throw.

```cpp
try {
  std::function<DataFrame(const DataFrame&)> empty;
  auto bad = biz.apply("orders", empty);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto bad = biz.apply("missing", [](const DataFrame& frame) {
    return frame;
  });
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto bad = biz.apply("orders", [](const DataFrame&) {
    return DataFrame();
  });
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Nexus::apply: callback is empty
Nexus::apply: unknown key 'missing'
Nexus::apply: callback returned undefined DataFrame
```

## Notes

- `apply()` is non-mutating.
- Table keys and table order are preserved; only one table payload changes.
- The callback is free to change the target table's shape. `apply()` does not
  require row-count or feature-count stability.
- Workspace metadata is preserved by table key, not by column signature. If the
  callback removes columns that existing relationships, saved views, or declared
  key metadata depend on, those higher-level structures may need manual review
  afterward.
- For the "same callback on every table" variant, use
  [`apply_all()`](apply_all.md) or [`map_frames()`](map_frames.md).

## See Also

- [`Nexus::apply_all`](apply_all.md): run one callback across every table.
- [`Nexus::map_frames`](map_frames.md): the same broad idea with more
  functional-programming-oriented naming.
- [`Nexus::flow`](flow.md): transform the whole workspace through one chainable
  callback.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../nexus_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Nexus API</span>
  </a>
  <a class="next-link" href="apply_all.html">
    <span class="label">Next</span>
    <span class="title">apply_all</span>
  </a>
</div>
