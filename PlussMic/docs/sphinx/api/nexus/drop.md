<!-- pluss-hand-authored -->

# Nexus::drop

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::drop(std::initializer_list<std::string_view> keys) const -> Nexus
Nexus::drop(pluss::msharp::RangeView::Of<std::string_view> keys) const -> Nexus
template <detail::StringViewNameRange NameRange>
Nexus::drop(const NameRange& keys) const -> Nexus
```

## What It Does

Builds a new workspace by removing the named tables you specify and carrying
forward every unmentioned table unchanged.

`drop()` is the complement of [`select()`](select.md):

- `select()` says "keep exactly these tables"
- `drop()` says "remove exactly these tables"

That difference affects ordering. `drop()` does **not** use request order to
build the output; instead, it preserves the original left-to-right order of every
table that survives.

Like the other subset operations, `drop()` carries forward table metadata for the
remaining tables and keeps only the relationships and saved views that still make
structural sense after the removal.

## Parameters

**keys** (a range of `std::string_view` — a brace literal, a named
container such as `std::vector<std::string_view>`/`std::vector<std::string>`,
or any other range whose elements convert to `std::string_view`)

The table names to remove.

- Each key must be non-empty.
- Each key must be unique within the request.
- Each key must already exist in the source workspace.
- Unlike [`select()`](select.md), request/iteration order never affects the
  output here — surviving tables always keep the *source* workspace's
  original order, regardless of what order (or container) you list them to
  remove in.

An empty range is allowed and acts as a no-op.

```{note} `pluss::msharp::RangeView` is never spelled out in these examples
`RangeView::Of<std::string_view>` has implicit constructors from a brace
list, a `std::vector<std::string_view>`, and other contiguous containers, so
passing any of those at a call site works without ever naming `RangeView`
yourself. Example 1 below uses a brace literal (the dedicated
`initializer_list` overload exists specifically to keep that call
unambiguous); Example 2 passes a named container instead, to exercise the
`RangeView`/templated overloads without naming either.
```

## Returns

`Nexus`
    A new workspace containing every source table except the ones named in
    `keys`, with the surviving tables kept in their original order.

    The source `Nexus` is unchanged.

## Examples

Example 1. Remove two named tables and preserve the original order of what
remains.

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

auto out = biz.drop({"orders", "refunds"});
print_keys(out);
print_keys(biz);
```

### Output

```text
customers, payments
customers, orders, payments, refunds
```

Example 2. Continuing from Example 1: the same request via named containers —
a `std::vector<std::string_view>` (the `RangeView` overload) and a
`std::vector<std::string>` (the templated overload, reached because owning
`std::string` doesn't match `RangeView`'s same-element-type vector
constructor but does satisfy the broader `StringViewNameRange` concept).
Both produce the identical, source-ordered result.

```cpp
std::vector<std::string_view> remove_view = {"orders", "refunds"};
std::vector<std::string> remove_owned = {"orders", "refunds"};

auto out_view = biz.drop(remove_view);
auto out_owned = biz.drop(remove_owned);
for (std::int64_t i = 0; i < out_view.ntables(); ++i) {
  if (i > 0) std::cout << ", ";
  std::cout << out_view.keys()[static_cast<std::size_t>(i)];
}
std::cout << "\n";
for (std::int64_t i = 0; i < out_owned.ntables(); ++i) {
  if (i > 0) std::cout << ", ";
  std::cout << out_owned.keys()[static_cast<std::size_t>(i)];
}
```

### Output

```text
customers, payments
customers, payments
```

Example 3. Continuing from Example 1, dropping an empty list is a no-op.

```cpp
auto out = biz.drop({});
for (std::int64_t i = 0; i < out.ntables(); ++i) {
  if (i > 0) std::cout << ", ";
  std::cout << out.keys()[static_cast<std::size_t>(i)];
}
```

### Output

```text
customers, orders, payments, refunds
```

Example 4. Continuing from Example 1, validation is strict: empty names,
duplicate names in the request, and unknown names all throw.

```cpp
try {
  auto bad = biz.drop({"", "orders"});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto dup = biz.drop({"orders", "orders"});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto missing = biz.drop({"orders", "missing"});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Nexus::drop: key cannot be empty
Nexus::drop: duplicate key 'orders'
Nexus::drop: unknown key 'missing'
```

## Notes

- `drop()` is non-mutating.
- Surviving tables keep their original order. This is the key semantic difference
  from [`select()`](select.md), whose request list determines output order.
- Surviving table metadata is preserved.
- Relationships and saved views are pruned to the subset still supported by the
  remaining tables.
- If you want the result to contain *only* a named list of tables, use
  [`select()`](select.md) instead.

## See Also

- [`Nexus::select`](select.md): keep an explicit table list instead of removing
  one.
- [`Nexus::tail`](tail.md): drop by position from the front rather than by name.
- [`Nexus::keys`](keys.md): inspect the remaining order after dropping tables.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dloc.html">
    <span class="label">Previous</span>
    <span class="title">dloc</span>
  </a>
  <a class="next-link" href="duplicates.html">
    <span class="label">Next</span>
    <span class="title">duplicates</span>
  </a>
</div>
