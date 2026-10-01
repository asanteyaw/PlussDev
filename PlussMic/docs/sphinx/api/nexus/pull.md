<!-- pluss-hand-authored -->

# Nexus::pull

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::pull(std::string_view root) const -> NexusPull
Nexus::pull(std::string_view root, std::initializer_list<std::string_view> fields) const -> DataFrame
Nexus::pull(std::string_view root, pluss::msharp::RangeView::Of<std::string_view> fields) const -> DataFrame
template <detail::StringViewNameRange NameRange>
Nexus::pull(std::string_view root, const NameRange& fields) const -> DataFrame
Nexus::pull(std::string_view root, std::initializer_list<PullSpec> fields) const -> DataFrame
Nexus::pull(std::string_view root, const std::vector<PullSpec>& fields) const -> DataFrame
```

## What It Does

Assembles a wide feature table, one row per `root` row, from fields spread across the
whole relationship graph — a "give me these specific columns, from wherever they live"
alternative to [`materialize()`](materialize.md), which instead returns the full joined
frame with every column from every hop. `pull()` never changes `root`'s own row count or
row grain; every requested field, however many hops away, comes back as exactly one value
per `root` row.

There are now three public ways to describe the requested fields:

- the fluent builder returned by `pull(root)` (`.take(...)`, `.agg(...).sum(...).end()`, `.collect()`)
- the original compact string grammar (`"orders.amount.sum"`, `"regions.zone"`)
- the preferred typed `PullSpec` helpers (`take(...)`, `count(...)`, `sum(...)`, `mean(...)`,
  `median(...)`, `min(...)`, `max(...)`, `std(...)`, `var(...)`)

The execution rules are the same either way. The fluent builder is now the preferred
teaching surface because it removes the repetitive fully qualified helper calls from
real feature-engineering code while still delegating to the same underlying typed pull
engine. The lower-level typed form remains useful when you want to inject a prebuilt
`PullSpec`, especially for windowed reducers.

Each entry in `fields` is a small expression, parsed according to its own grammar:

- **A bare name** (`"amount"`) — a feature on `root` itself.
- **`table.feature`** (`"customers.name"`) — a feature on a related table, reached by
  walking the shortest relationship path from `root` to `table` (via the same search
  [`path()`](path.md) uses). The path must be **root-preserving**: every hop along the way
  must be `ManyToOne` or `OneToOne` from `root`'s side, so each `root` row maps to at most
  one row on `table`. A path that would require expanding `root` into several rows (a
  `OneToMany` or `ManyToMany` hop) throws — see Example 5.
- **`table.feature.aggregate`** (`"orders.amount.sum"`) — an aggregate of a related table's
  feature, computed by grouping that table's rows by `root`'s declared primary key.
  `aggregate` is one of `count`, `ndistinct`, `sum`, `mean`, `median`, `min`, `max`, `std`,
  `var` (case-insensitive).
  Unlike plain fields, aggregate fields work across **any** cardinality, including
  `OneToMany` — that's the whole point of aggregating. `table` must not be `root` itself
  (aggregating a root feature against itself doesn't mean anything — see Example 8).
- **`` `quoted.name` ``** — backtick-quote any path segment that itself contains a literal
  `.`, so it isn't mistaken for a path separator — see Example 9.

Every output column is aliased automatically: a plain root feature keeps its own name; a
plain related-table feature becomes `<table>_<feature>` (each piece sanitized to
alphanumerics-and-underscores); an aggregate becomes `<table>_<feature>_<aggregate>`.

```{important} Aggregate fields require a complete, unique declared primary key on root
Every aggregate field needs to group by *something* — that something is `root`'s declared
[`TableMeta::primary_key`](table_meta.md). If `root` has no declared metadata, or a
`primary_key` that doesn't fully and uniquely identify its rows, requesting **any**
aggregate field throws before touching the aggregation itself — see Example 10. Plain
(non-aggregate) fields have no such requirement.
```

```{important} Windowed aggregates: <agg>_<N><unit>[_root]
Appending a duration to an aggregate token turns it into a **windowed** aggregate:
`"orders.amount.sum_7d"` sums `amount` only over orders within a trailing time window,
instead of every matching row unconditionally. The duration is a positive integer followed
by a unit — `s`/`sec`/`secs`/`second`/`seconds`, `m`/`min`/`mins`/`minute`/`minutes`,
`h`/`hr`/`hrs`/`hour`/`hours`, `d`/`day`/`days`, or `w`/`wk`/`wks`/`week`/`weeks`. A
windowed aggregate requires the related table (`orders`, above) to have a declared
[`time_field`](table_meta.md).

The window's **end point** — where the trailing window is measured *from* — depends on the
anchor, chosen by an optional `_root` suffix:

- **`_<N><unit>`** (no `_root`; the default, "Latest" anchor): the window ends at each
  `root` row's own **most recent matching related-table row**, per the related table's
  `time_field`. Concretely, `"orders.amount.sum_7d"` sums each customer's orders that fall
  within 7 days *before their own latest order* — a trailing window anchored to each
  customer's own activity, not a fixed calendar date.
- **`_<N><unit>_root`**: the window ends at `root`'s **own** declared `time_field` instead
  — which also means `root` itself must have a declared `time_field` for this form, not
  just the related table. See Example 7 for how this changes the result compared to the
  default "Latest" anchor.

`ndistinct` windows are not supported and throw `"ndistinct windows are not supported
yet"`. `median`, `std`, and `var` are supported for ordinary grouped pulls, but not for
windowed pulls yet.
```

## Parameters

**root** (`std::string_view`)
    The table whose row grain is preserved. Must already exist in the workspace.

**fields** (`std::initializer_list<std::string_view>`)
    One or more field expressions, parsed as described above. Each parsed field's output
    alias must be unique — two expressions that would collide on the same alias throw. At
    least one field is required.

## Returns

`DataFrame`
    One row per `root` row, one column per requested field (aliased as described above),
    titled `root + "_pull"`.

## Fluent Builder Surface

`pull(root)` returns a `NexusPull` builder. The most important public methods on that
builder are:

```cpp
NexusPull& take(std::string_view source, std::string_view alias = {});
NexusPull& take(pluss::msharp::RangeView::Of<std::string_view> sources);
NexusPull& take(
    std::string_view table,
    pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
NexusPullAgg& count(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
NexusPullAgg& sum(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
NexusPullAgg& mean(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
NexusPullAgg& median(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
NexusPullAgg& min(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
NexusPullAgg& max(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
NexusPullAgg& std(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
NexusPullAgg& var(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields);
NexusPullAgg agg(std::string_view table);
NexusPull& add(const PullSpec& spec);
NexusPull& add(std::initializer_list<PullSpec> specs);
DataFrame collect() const;
```

The important detail is that `RangeView` is the canonical builder edge, while
brace-list, vector, array, and similar name ranges are adapted through the same
`StackArray`-backed helper pattern the `DataFrame` API already uses.

- Root-field listings such as `.take({"customer_id", "region_id"})` stay ergonomic,
  but now funnel through the same owned `StackArray<std::string_view>` adaptation used
  elsewhere in Pluss.
- Scoped related-table lifts such as `.take("regions", {{"zone", "zone"}})` accept a
  range of `(source, alias)` pairs and then expose it to the builder as a
  `RangeView`.
- The same batched-pair idea now extends to grouped reducers, so
  `.agg("events").sum({{"spend", "total_spend"}, {"fee", "total_fee"}})` is valid and
  avoids one method call per feature when several columns share the same reduction.
- If you prefer standard associative containers, `std::map` and `std::unordered_map`
  adapt through the same pair-based builder surface for `.meta(...)`, `.take(...)`, and
  scoped reducer batches.
- In other words, user-facing code stays short; the adapter types standardize the
  overload surface and prevent the ambiguity that the earlier raw `initializer_list`
  overload set introduced.

## Fluent Builder Example

The fluent pull builder is the preferred spelling when you are building a real root-grain
feature table.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"region_id",   Thread(pluss::make_series({10.0, 20.0, 10.0}))},
});
auto regions = DataFrame({
    {"region_id", Thread(pluss::make_series({10.0, 20.0}))},
    {"zone",      Thread(pluss::make_note({"west", "east"}))},
});
auto events = DataFrame({
    {"event_id",    Thread(pluss::make_series({100.0, 101.0, 102.0, 103.0, 104.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 1.0, 2.0, 2.0, 2.0}))},
    {"occurred_at", Thread(pluss::make_timeline({
        "2026-01-01T00:00:00Z",
        "2026-01-20T00:00:00Z",
        "2026-01-05T00:00:00Z",
        "2026-01-25T00:00:00Z",
        "2026-02-10T00:00:00Z",
    }))},
    {"spend",       Thread(pluss::make_series({10.0, 5.0, 2.0, 8.0, 4.0}))},
});

TableMeta customer_meta;
customer_meta.primary_key = {"customer_id"};

TableMeta event_meta;
event_meta.time_field = "occurred_at";

Relationship customer_region;
customer_region.name = "customer_region";
customer_region.left_table = "customers";
customer_region.right_table = "regions";
customer_region.keys = {{"region_id", "region_id"}};
customer_region.cardinality = RelationshipCardinality::ManyToOne;

Relationship customer_events;
customer_events.name = "customer_events";
customer_events.left_table = "customers";
customer_events.right_table = "events";
customer_events.keys = {{"customer_id", "customer_id"}};
customer_events.cardinality = RelationshipCardinality::OneToMany;

auto nx = Nexus({
    {"customers", customers},
    {"regions", regions},
    {"events", events},
}).meta({
    {"customers", customer_meta},
    {"events", event_meta},
})
  .links({customer_region, customer_events});

auto pulled = nx.pull("customers")
    .take("customer_id")
    .take("regions", {{"zone", "zone"}})
    .agg("events")
        .count("event_id", "event_count")
        .sum("spend", "total_spend")
        .median("spend", "median_spend")
    .end()
    .add({sum("events.spend", "recent_spend").windowed(30, "d")})
    .collect();

for (std::size_t i = 0; i < pulled.features().size(); ++i) {
  if (i > 0) std::cout << " ";
  std::cout << pulled.features()[i];
}
```

### Output

```text
customer_id zone event_count total_spend median_spend recent_spend
```

Notice the alias behavior:

- `.take("regions", {{"zone", "zone"}})` scopes several related-table lifts under one table token
- `.agg("events") ... .end()` scopes several grouped reducers under one child table token
- aggregate helpers usually want an explicit alias (`"event_count"`, `"total_spend"`)
- windows are declared structurally with `.windowed(...)` or `.root_windowed(...)`

## Example: Batched Maps Instead Of Repeated Chains

If several fields share the same operation, a small associative container usually reads
better than one call per field.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
    {"region_id",   Thread(pluss::make_series({10.0, 20.0}))},
});
auto regions = DataFrame({
    {"region_id", Thread(pluss::make_series({10.0, 20.0}))},
    {"zone",      Thread(pluss::make_note({"west", "east"}))},
});
auto events = DataFrame({
    {"event_id",    Thread(pluss::make_series({100.0, 101.0, 102.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 1.0, 2.0}))},
    {"spend",       Thread(pluss::make_series({10.0, 5.0, 8.0}))},
    {"occurred_at", Thread(pluss::make_timeline({
        "2026-01-01T00:00:00Z",
        "2026-01-20T00:00:00Z",
        "2026-01-25T00:00:00Z",
    }))},
});

TableMeta customer_meta;
customer_meta.primary_key = {"customer_id"};

TableMeta event_meta;
event_meta.time_field = "occurred_at";

Relationship customer_region;
customer_region.name = "customer_region";
customer_region.left_table = "customers";
customer_region.right_table = "regions";
customer_region.keys = {{"region_id", "region_id"}};
customer_region.cardinality = RelationshipCardinality::ManyToOne;

Relationship customer_events;
customer_events.name = "customer_events";
customer_events.left_table = "customers";
customer_events.right_table = "events";
customer_events.keys = {{"customer_id", "customer_id"}};
customer_events.cardinality = RelationshipCardinality::OneToMany;

std::unordered_map<std::string, TableMeta> meta_batch;
meta_batch.emplace("customers", customer_meta);
meta_batch.emplace("events", event_meta);

auto nx = Nexus({
    {"customers", customers},
    {"regions", regions},
    {"events", events},
}).meta(meta_batch)
  .links({customer_region, customer_events});

auto pulled = nx.pull("customers")
    .take(std::map<std::string, std::string>{
        {"customer_id", "customer_id"},
    })
    .take("regions", std::map<std::string, std::string>{
        {"zone", "zone"},
    })
    .agg("events")
        .count(std::map<std::string, std::string>{
            {"event_id", "event_count"},
            {"occurred_at", "dated_event_count"},
        })
        .sum(std::unordered_map<std::string, std::string>{
            {"spend", "total_spend"},
        })
    .end()
    .collect();

for (std::size_t i = 0; i < pulled.features().size(); ++i) {
  if (i > 0) std::cout << " ";
  std::cout << pulled.features()[i];
}
```

### Output

```text
customer_id zone event_count dated_event_count total_spend
```

This is still the same `NexusPull` builder. The only difference is that repeated
same-shape calls are collapsed into one bulk declaration.

## Example: Range-Friendly Root Lists

The builder accepts root-field collections from ordinary C++ ranges, not only brace
lists.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
    {"region_id",   Thread(pluss::make_series({10.0, 20.0}))},
});
auto regions = DataFrame({
    {"region_id", Thread(pluss::make_series({10.0, 20.0}))},
    {"zone",      Thread(pluss::make_note({"west", "east"}))},
});

Relationship rel;
rel.name = "customer_region";
rel.left_table = "customers";
rel.right_table = "regions";
rel.keys = {{"region_id", "region_id"}};
rel.cardinality = RelationshipCardinality::ManyToOne;

std::vector<std::string_view> root_fields = {"customer_id", "region_id"};

auto nx = Nexus({
    {"customers", customers},
    {"regions", regions},
}).links({rel});

auto pulled = nx.pull("customers")
    .take(root_fields)
    .take("regions", {{"zone", "zone"}})
    .collect();

for (std::size_t i = 0; i < pulled.features().size(); ++i) {
  if (i > 0) std::cout << " ";
  std::cout << pulled.features()[i];
}
```

### Output

```text
customer_id region_id zone
```

The same call also works with:

- `.take({"customer_id", "region_id"})`
- `.take(std::array<std::string_view, 2>{"customer_id", "region_id"})`
- `.take(pluss::StackArray<std::string_view>{"customer_id", "region_id"})`

## Examples

Example 1. A bare field from `root` itself.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0, 104.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 2.0, 1.0}))},
    {"amount",      Thread(pluss::make_series({50.0, 75.0, 20.0, 30.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

TableMeta customers_meta;
customers_meta.primary_key = {"customer_id"};
auto tagged = biz.set_table_meta("customers", customers_meta);

Relationship rel;
rel.name = "orders_to_customers";
rel.left_table = "orders";
rel.right_table = "customers";
rel.keys = {{"customer_id", "customer_id"}};
rel.cardinality = RelationshipCardinality::ManyToOne;

auto linked = tagged.add_relationship(rel);

auto p1 = linked.pull("customers", {"name"});
std::cout << p1.features("name");
```

### Output

```text
0  Ada
1  Ben
2  Cy
Name: name, ptype: Note
```

Example 2. A plain field pulled from `root` (`orders`, the "many" side) across to a
related table (`customers`, the "one" side) — root-preserving, since each order matches
exactly one customer.

```cpp
auto p2 = linked.pull("orders", {"order_id", "customers.name"});
std::cout << p2.features("customers_name");
```

### Output

```text
0  Ada
1  Ben
2  Ben
3  Ada
Name: customers_name, ptype: Note
```

Example 3. An aggregate: how many orders each customer has.

```cpp
auto p3 = linked.pull("customers", {"name", "orders.order_id.count"});
std::cout << p3.features("orders_order_id_count");
```

### Output

```text
0  2
1  2
2  0
Name: orders_order_id_count, ptype: Series
```

Cy (customer `3`, with zero orders) correctly gets `0`, not a missing value.

Example 4. An aggregate that has nothing to aggregate returns a missing value, not `0` —
contrast `count`'s Example 3.

```cpp
auto p4 = linked.pull("customers", {"name", "orders.amount.sum"});
std::cout << p4.features("orders_amount_sum");
```

### Output

```text
0  80
1  95
2  NULL
Name: orders_amount_sum, ptype: Series
```

Example 5. Pulling a plain (non-aggregate) field the *other* direction — from `customers`
(the "one" side) out to `orders` (the "many" side) — is not root-preserving, and throws.

```cpp
try {
  auto bad = linked.pull("customers", {"orders.amount"});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::pull: requested field 'orders.amount' requires expanding relationship 'orders_to_customers' from 'customers' to 'orders'. pull preserves root grain and cannot traverse one-to-many or many-to-many paths
```

This is exactly the situation `orders.amount.sum` (Example 4) is for — aggregate the
many-side rows down to one value per `root` row, instead of trying to expand `root`.

Example 6. Windowed aggregates: a customer's two orders 10 days apart, summed within a
trailing 7-day window anchored at their own latest order.

```cpp
auto customers2 = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben"}))},
});
auto orders2 = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 1.0, 2.0}))},
    {"amount",      Thread(pluss::make_series({50.0, 30.0, 75.0}))},
    {"ordered_at",  Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z",
        "2024-01-11T00:00:00Z",
        "2024-01-01T00:00:00Z",
    }))},
});

auto biz2 = Nexus({
    {"customers", customers2},
    {"orders", orders2},
});

TableMeta customers2_meta;
customers2_meta.primary_key = {"customer_id"};
auto tagged2 = biz2.set_table_meta("customers", customers2_meta);

TableMeta orders2_meta;
orders2_meta.time_field = "ordered_at";
auto tagged3 = tagged2.set_table_meta("orders", orders2_meta);

auto linked2 = tagged3.add_relationship(rel);

auto plain_sum = linked2.pull("customers", {"name", "orders.amount.sum"});
std::cout << plain_sum.features("orders_amount_sum") << "\n";

auto windowed_sum = linked2.pull("customers", {"name", "orders.amount.sum_7d"});
std::cout << windowed_sum.features("orders_amount_sum_7d");
```

### Output

```text
0  80
1  75
Name: orders_amount_sum, ptype: Series
0  30
1  75
Name: orders_amount_sum_7d, ptype: Series
```

The plain sum counts both of Ada's orders (`50 + 30 = 80`). The windowed sum only counts
orders within 7 days *before Ada's own latest order* (`Jan 11`) — her `Jan 1` order
(`50`) is 10 days earlier, outside that window, so only the `Jan 11` order's own `30`
survives. Ben has one order either way, so his result is unchanged.

Example 7. The `_root` anchor changes the window's end point from "each customer's own
latest order" to "the root table's own declared time — here, `signup_at`."

```cpp
auto customers3 = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben"}))},
    {"signup_at",   Thread(pluss::make_timeline({
        "2024-01-08T00:00:00Z",
        "2024-01-01T00:00:00Z",
    }))},
});

auto biz3 = Nexus({
    {"customers", customers3},
    {"orders", orders2},
});

TableMeta customers3_meta;
customers3_meta.primary_key = {"customer_id"};
customers3_meta.time_field = "signup_at";
auto step1 = biz3.set_table_meta("customers", customers3_meta);
auto step2 = step1.set_table_meta("orders", orders2_meta);
auto linked3 = step2.add_relationship(rel);

auto root_anchored = linked3.pull("customers", {"name", "orders.amount.sum_7d_root"});
std::cout << root_anchored.features("orders_amount_sum_7d_root");
```

### Output

```text
0  50
1  75
Name: orders_amount_sum_7d_root, ptype: Series
```

Ada signed up `Jan 8`. A 7-day trailing window ending there (`Jan 1`–`Jan 8`) includes her
`Jan 1` order (`50`) but excludes her `Jan 11` order (which happens *after* signup) —
the opposite selection from Example 6's "Latest" anchor, which excluded `Jan 1` and kept
`Jan 11`.

Example 8. An aggregate field targeting `root` itself throws.

```cpp
try {
  auto bad = linked.pull("customers", {"customers.name.count"});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::pull: aggregate field 'customers.name.count' must target a related table, not the root table
```

Example 9. Backtick-quoting a feature name that itself contains a literal `.`.

```cpp
auto customers4 = DataFrame({
    {"customer_id",  Thread(pluss::make_series({1.0, 2.0}))},
    {"display.name", Thread(pluss::make_note({"Ada A.", "Ben B."}))},
});
auto orders4 = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
});

auto biz4 = Nexus({
    {"customers", customers4},
    {"orders", orders4},
});
auto linked4 = biz4.add_relationship(rel);

auto p9 = linked4.pull("orders", {"order_id", "customers.`display.name`"});
std::cout << p9.features("customers_display_name");
```

### Output

```text
0  Ada A.
1  Ben B.
Name: customers_display_name, ptype: Note
```

Without the backticks, `customers.display.name` would parse as three path segments
(`customers`, then `display`, then `name`) instead of one table plus one literally-dotted
feature name.

Example 10. Requesting any aggregate field when `root` has no usable declared primary key
throws — checked before any aggregation runs.

```cpp
try {
  auto bad = linked.pull("orders", {"order_id", "customers.name.count"});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::pull: no metadata declared for table 'orders'
```

`orders` (used as `root` here) never had `TableMeta` declared at all in this session, so
the aggregate field's primary-key requirement fails immediately.

Example 11. Continuing from Example 1's `linked` workspace: the same
`pull(root, fields)` request via named containers instead of a brace
literal — a `std::vector<std::string_view>` (the `RangeView` overload) and a
`std::vector<std::string>` (the templated overload, reached because owning
`std::string` doesn't match `RangeView`'s same-element-type vector
constructor but does satisfy the broader `StringViewNameRange` concept).

```cpp
std::vector<std::string_view> as_view = {"name"};
std::vector<std::string> as_owned = {"name"};

auto p_view = linked.pull("customers", as_view);
auto p_owned = linked.pull("customers", as_owned);

for (std::size_t i = 0; i < p_view.features().size(); ++i) {
  if (i > 0) std::cout << " ";
  std::cout << p_view.features()[i];
}
std::cout << "\n";
for (std::size_t i = 0; i < p_owned.features().size(); ++i) {
  if (i > 0) std::cout << " ";
  std::cout << p_owned.features()[i];
}
```

### Output

```text
name
name
```

```{note} `pluss::msharp::RangeView` is never spelled out in these examples
See [`Nexus::select`](select.md)'s own note on this — the same
three-overload pattern (brace literal / named `RangeView`-convertible
container / any other qualifying range) applies to every `fields`
parameter on this page, including the fluent [`NexusPull::take`](../nexus-pull/take.md)
examples above.
```

## Notes

- Never changes `root`'s row count — plain fields must be root-preserving (throws
  otherwise); aggregate fields collapse any cardinality down to one value per `root` row.
- Output column aliases are generated automatically: bare name for a root feature,
  `<table>_<feature>` for a plain related field, `<table>_<feature>_<aggregate>` for an
  aggregate. Two requested fields that would alias to the same name throw.
- Aggregate fields require `root` to have a complete, unique declared `primary_key` — see
  the important callout above.
- Windowed aggregates require the related table's declared `time_field`; the `_root`
  anchor additionally requires `root`'s own declared `time_field`. `ndistinct` windows are
  unsupported.
- `count`/`ndistinct` aggregates fill unmatched `root` rows with `0`; every other
  aggregate leaves them missing (`NULL`) — see Examples 3 and 4.

## See Also

- [`Nexus::materialize`](materialize.md): the full-frame alternative, returning every
  column from every hop instead of hand-picked fields.
- [`Nexus::path`](path.md): the same root-preservation check applies to the routes this
  method walks for plain fields.
- [`Nexus::table_meta`](table_meta.md): declare `primary_key` and `time_field`, both
  required by different parts of this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="profile.html">
    <span class="label">Previous</span>
    <span class="title">profile</span>
  </a>
  <a class="next-link" href="relationship.html">
    <span class="label">Next</span>
    <span class="title">relationship</span>
  </a>
</div>
