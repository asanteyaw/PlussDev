<!-- pluss-hand-authored -->

# Nexus::run

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
DataFrame Nexus::run(std::string_view name) const

DataFrame Nexus::run(std::string_view name, const JoinPlan& plan) const
```

## What It Does

Executes a join plan and returns the result, titled `name`. The one-argument overload
looks up a plan previously saved via [`store()`](store.md) by that name and executes it —
the usual way to reuse a saved view. The two-argument overload normalizes and executes
`plan` directly **without saving it** — a one-off run under an explicit title, useful when
you want a named result without permanently registering the view in the workspace.

## Parameters

**name** (`std::string_view`)
    For the one-argument overload: the saved view to execute — must already exist (see
    [`has_store()`](has_store.md)), or the call throws
    `"Nexus::run: unknown store '<name>'"`. For the two-argument overload: just the title
    applied to the result; no lookup happens.

**plan** (`const JoinPlan&`, two-argument overload)
    The plan to normalize and execute, following the same rules
    [`store()`](store.md)'s saving overloads apply.

## Returns

`DataFrame`
    The materialized result, titled `name`.

## Examples

Example 1. Save a view, then run it by name.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Slate(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 2.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

Relationship rel;
rel.name = "orders_to_customers";
rel.left_table = "orders";
rel.right_table = "customers";
rel.keys = {{"customer_id", "customer_id"}};

auto linked = biz.add_relationship(rel);

auto plan_value = linked.plan("orders", "customers", JoinHow::Left);
auto stored = linked.store("orders_with_customers", plan_value);

auto result = stored.run("orders_with_customers");
std::cout << result.title() << " " << result.ncases() << " " << result.nfeatures();
```

### Output

```text
orders_with_customers 3 3
```

Example 2. Running an unknown saved view throws.

```cpp
try {
  auto bad = stored.run("nope");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::run: unknown store 'nope'
```

Example 3. The two-argument overload runs a plan directly, under a title, without saving
it as a view.

```cpp
auto plan2 = linked.plan("customers", "orders", JoinHow::Left);
auto result2 = stored.run("customers_with_orders", plan2);

std::cout << result2.title() << " " << result2.ncases() << "\n";
std::cout << std::boolalpha << stored.has_store("customers_with_orders");
```

### Output

```text
customers_with_orders 4
false
```

The result is titled and returned normally, but `"customers_with_orders"` never becomes a
retrievable saved view — `has_store()` still reports `false` for it afterward.

## Notes

- The one-argument overload throws on an unknown view name; the two-argument overload
  never looks anything up by `name`, it's only used as the result's title.
- The two-argument overload does **not** call [`store()`](store.md) — running a plan this
  way is a one-off, not a registration.

## See Also

- [`Nexus::store`](store.md): save a `JoinPlan` under a name for later reuse via this
  method's one-argument overload.
- [`Nexus::has_store`](has_store.md): check whether a name is a valid saved view before
  running it.
- [`Nexus::materialize`](materialize.md): the untitled, plan-only execution primitive both
  overloads build on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename.html">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="save.html">
    <span class="label">Next</span>
    <span class="title">save</span>
  </a>
</div>
