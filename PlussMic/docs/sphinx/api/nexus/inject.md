<!-- pluss-hand-authored -->

# Nexus::inject

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
Nexus Nexus::inject(std::string_view key, const DataFrame& frame) const

Nexus Nexus::inject(std::initializer_list<std::pair<std::string_view, DataFrame>> tables) const

Nexus Nexus::inject(const DataFrame& frame) const

Nexus Nexus::inject(std::initializer_list<DataFrame> tables) const
```

## What It Does

Appends one or more tables to a `Nexus` workspace and returns the expanded
workspace as a new `Nexus`.

`inject(...)` is the main "grow this workspace" API when you want to stay
pipeline-friendly and non-mutating. Instead of editing the current workspace in
place, it gives you a new handle with the appended tables while leaving the
source `Nexus` untouched. That makes it a natural fit for expression-style
construction:

```cpp
auto nx = Nexus({{"customers", customers}})
  .inject("orders", orders)
  .inject(payments);
```

The four overloads split cleanly into two naming modes:

- **Named injection**: you supply table names explicitly.
- **Unnamed injection**: `Nexus` manufactures names for you as `table_0`,
  `table_1`, `table_2`, and so on.

Every overload appends. None of them replace an existing table in place. If the
target name already exists, the call throws rather than overwriting anything.

## Parameters

**key** (`std::string_view`)

The explicit name to assign to the injected table in the single-table named
overload.

- Must not be empty.
- Must not already exist in the workspace.
- If it has the auto-key shape `table_N`, it also advances the internal
  auto-naming counter so later unnamed injections continue from there rather than
  reusing an older number.

**frame** (`const DataFrame&`)

The single table to append.

- Must be defined.
- The table is appended at the end of the workspace.
- In the unnamed overload, `Nexus` chooses the new key automatically.

**tables** (`std::initializer_list<std::pair<std::string_view, DataFrame>>`)

Batch named injection. Each pair is `(table_name, frame)`.

- Tables are appended in the exact order you supply them.
- Each name must be non-empty.
- Each name must be unique relative to the existing workspace and the rest of
  the batch.
- Each `DataFrame` must be defined.

**tables** (`std::initializer_list<DataFrame>`)

Batch unnamed injection.

- Each frame is appended in the exact order you supply it.
- Each frame receives the next available auto-generated key.
- Each `DataFrame` must be defined.

## Returns

`Nexus`
    A new workspace containing the original tables followed by the injected
    tables.

    Existing table order is preserved. New tables are always appended, never
    inserted into the middle and never used to overwrite an existing key.

## Examples

Example 1. Inject one named table.

This is the most explicit overload: you choose the key and the frame together.
It is also the clearest way to demonstrate that `inject(...)` is non-mutating.
The original workspace still only knows about `"customers"`; the returned one has
both `"customers"` and `"orders"`.

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
});

auto out = biz.inject("orders", orders);

print_keys(biz);
print_keys(out);
std::cout << std::boolalpha << biz.has("orders") << " " << out.has("orders");
```

### Output

```text
customers
customers, orders
false true
```

Example 2. Inject several named tables in one call.

The batch-named overload is the right fit when all incoming tables already have
stable business names. The supplied order is preserved exactly, so the new
workspace ends in `"orders"`, then `"payments"`, then `"refunds"`.

```cpp
auto out = biz.inject({
    {"orders", orders},
    {"payments", payments},
    {"refunds", refunds},
});

for (std::int64_t i = 0; i < out.ntables(); ++i) {
  if (i > 0) std::cout << ", ";
  std::cout << out.keys()[static_cast<std::size_t>(i)];
}
```

### Output

```text
customers, orders, payments, refunds
```

Example 3. Inject one unnamed table and let `Nexus` generate the key.

This overload is convenient for scratch work, temporary workspaces, or any flow
where the table identity matters less than the table data. The first unnamed
injection into this workspace becomes `table_0`.

```cpp
auto out = biz.inject(orders);

for (std::int64_t i = 0; i < out.ntables(); ++i) {
  if (i > 0) {
    std::cout << ", ";
  }
  std::cout << out.keys()[static_cast<std::size_t>(i)];
}
```

### Output

```text
customers, table_0
```

Example 4. Inject several unnamed tables in one call.

The batch unnamed overload simply repeats the same auto-naming rule for each
incoming frame. Names are assigned left to right in the same order as the
initializer list.

```cpp
auto out = biz.inject({orders, payments, refunds});

for (std::int64_t i = 0; i < out.ntables(); ++i) {
  if (i > 0) {
    std::cout << ", ";
  }
  std::cout << out.keys()[static_cast<std::size_t>(i)];
}
```

### Output

```text
customers, table_0, table_1, table_2
```

Example 5. Mixing explicit auto-style names with unnamed injection changes the
future auto-generated names.

This subtle behavior matters when you intentionally name a table `table_N`
yourself. `Nexus` notices that pattern and advances its internal counter, so the
next unnamed injection starts after `N` instead of trying to reuse lower numbers.

```cpp
auto out = biz
  .inject("table_7", orders)
  .inject(payments)
  .inject(refunds);

for (std::int64_t i = 0; i < out.ntables(); ++i) {
  if (i > 0) {
    std::cout << ", ";
  }
  std::cout << out.keys()[static_cast<std::size_t>(i)];
}
```

### Output

```text
customers, table_7, table_8, table_9
```

Example 6. Injecting an explicitly typed empty list is a no-op.

This example is less about data work and more about overload selection. A plain
`inject({})` is ambiguous to a reader because two initializer-list overloads exist.
If you really mean "inject nothing", spell the list type explicitly.

```cpp
auto same_named =
  biz.inject(std::initializer_list<std::pair<std::string_view, DataFrame>>{});
auto same_unnamed =
  biz.inject(std::initializer_list<DataFrame>{});

std::cout << same_named.ntables() << " " << same_unnamed.ntables() << "\n";

for (std::int64_t i = 0; i < same_named.ntables(); ++i) {
  if (i > 0) {
    std::cout << ", ";
  }
  std::cout << same_named.keys()[static_cast<std::size_t>(i)];
}
std::cout << "\n";

for (std::int64_t i = 0; i < same_unnamed.ntables(); ++i) {
  if (i > 0) {
    std::cout << ", ";
  }
  std::cout << same_unnamed.keys()[static_cast<std::size_t>(i)];
}
```

### Output

```text
1 1
customers
customers
```

Example 7. Validation failures: empty key, duplicate key, and undefined frame.

`inject(...)` is append-only, but it is not permissive. It rejects invalid names,
duplicate names, and undefined frames immediately rather than silently guessing at
what the caller meant.

```cpp
try {
  auto bad = biz.inject("", orders);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto dup = biz.inject("customers", orders);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  DataFrame undefined;
  auto ghost = biz.inject("ghost", undefined);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Nexus::inject: key cannot be empty
Nexus::inject: duplicate key 'customers'
Nexus::inject: frame is undefined
```

## Notes

- `inject(...)` is non-mutating. The source `Nexus` is left unchanged; the result
  is a new handle.
- New tables are always appended at the end of the workspace. If you care about
  the final table order, the order of the initializer list is the order that will
  appear in [`keys()`](keys.md) and [`frames()`](frames.md).
- Unnamed injection uses auto-generated keys of the form `table_N`.
- Explicitly naming a table `table_N` is not cosmetic only; it advances the
  internal auto-key counter used by later unnamed injections.
- `inject(...)` never overwrites an existing table name. Duplicate keys are always
  an error.
- If you want to mutate an existing `Nexus` in place instead of receiving a new
  one, the corresponding mutating surface is `inject_(...)`.

## See Also

- [`Nexus::keys`](keys.md): inspect the final key order after injection.
- [`Nexus::frames`](frames.md): inspect the corresponding frame sequence.
- [`Nexus::has`](has.md): confirm whether a named injection succeeded under the new
  workspace handle.
- [`Nexus::ntables`](ntables.md): verify how many tables the expanded workspace now
  contains.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="join.html">
    <span class="label">Next</span>
    <span class="title">join</span>
  </a>
</div>
