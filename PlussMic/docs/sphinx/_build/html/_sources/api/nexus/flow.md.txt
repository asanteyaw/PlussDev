<!-- pluss-hand-authored -->

# Nexus::flow

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::flow(const std::function<Nexus(const Nexus&)>& fn) const -> Nexus
```

## What It Does

Passes the entire workspace through a callback and returns whatever valid `Nexus`
the callback produces.

`flow()` is the broadest chainable transformation surface on `Nexus`. Unlike
[`apply()`](apply.md), [`apply_all()`](apply_all.md), or
[`map_frames()`](map_frames.md), it does not operate table-by-table. Instead, it
hands the whole workspace to your callback so the callback can decide whether to
select tables, rename tables, inject tables, drop tables, or compose several of
those operations together before returning the next workspace.

Use it when you want to keep a fluent chain but need a short custom block of
logic in the middle.

## Parameters

**fn** (`const std::function<Nexus(const Nexus&)>&`)

Callback that receives the current workspace and returns the next workspace.

- Must be non-empty.
- Receives the current `Nexus` by `const` reference.
- Must return a **defined** `Nexus`.
- May return the input unchanged, a derived subset, an expanded workspace, or any
  other valid `Nexus`.

## Returns

`Nexus`
    Whatever valid workspace the callback returns.

    The source `Nexus` is unchanged unless the callback explicitly returns a
    modified copy derived from it.

## Examples

Example 1. Use `flow()` to insert a short workspace-level branch in the middle
of a chain.

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

auto out = biz.flow([](const Nexus& value) {
  return value.select({"payments", "customers"}).drop({"customers"});
});

print_keys(out);
```

### Output

```text
payments
```

Example 2. Continuing from Example 1, the callback can compose several ordinary
`Nexus` methods and return the result as one expression.

```cpp
auto out = biz.flow([](const Nexus& value) {
  return value.rename("orders", "purchases").head(3);
});

print_keys(out);
```

### Output

```text
customers, purchases, payments
```

Example 3. Continuing from Example 1, empty callbacks and undefined callback
results both throw.

```cpp
try {
  std::function<Nexus(const Nexus&)> empty;
  auto bad = biz.flow(empty);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto bad = biz.flow([](const Nexus& value) {
    auto copy = value;
    auto moved = std::move(copy);
    (void)moved;
    return copy;
  });
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Nexus::flow: callback is empty
Nexus::flow: callback returned undefined Nexus
```

## Notes

- `flow()` is non-mutating.
- The callback is not constrained to preserve any particular aspect of the
  workspace. It can return a subset, a renamed workspace, an injected workspace,
  or the input unchanged.
- Because the callback works at the workspace level, `flow()` is the right tool
  for logic that depends on multiple tables or on method composition, not just
  on transforming one table body.
- If the callback's logic is really "rewrite one table" or "rewrite every table",
  [`apply()`](apply.md) or [`apply_all()`](apply_all.md) usually communicates
  intent more clearly than `flow()`.

## See Also

- [`Nexus::apply`](apply.md): transform one named table.
- [`Nexus::apply_all`](apply_all.md): transform every table with one callback.
- [`Nexus::map_frames`](map_frames.md): functional table-by-table mapping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="frame.html">
    <span class="label">Next</span>
    <span class="title">frame</span>
  </a>
</div>
