<!-- pluss-hand-authored -->

# Nexus::join

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::join(std::string_view left, std::string_view right, const JoinSpec& spec) const -> DataFrame
```

## What It Does

Joins two tables already in the workspace, one hop at a time, and returns the joined
`DataFrame` directly — no relationship graph traversal, no multi-table plan, just
`left.unify(...)` under the hood using whatever `JoinSpec` you provide. This is the
lowest-level joining primitive on `Nexus`; [`materialize()`](materialize.md) is the version
that chains several `join()`-style steps together from a [`JoinPlan`](plan.md), and
[`NexusTraverse`](../../nexus_traverse_api_reference) (via [`from()`](from.md)) is the fluent,
higher-level way to build that chain without constructing a `JoinPlan` by hand.

```cpp
struct JoinSpec {
  std::vector<std::pair<std::string, std::string>> keys{};
  JoinHow how{JoinHow::Inner};
  std::string right_suffix{};
  std::optional<RelationshipCardinality> validate{};
  std::optional<std::string> relationship_name{};
  std::optional<std::string> result_name{};
};
```

`keys` lists `(left_feature, right_feature)` pairs to join on. `how` picks the join
semantics (`Inner`, `Left`, `Right`, `Outer`, `Cross`, `LeftOnly`, `RightOnly` — see
`join_how_string` in [`DataFrame::unify`](../dataframe/unify.md)'s own reference for what
each produces). `right_suffix` disambiguates overlapping non-key column names from the
right table (defaults to `"_" + right` when empty). `validate` optionally checks the join's
actual cardinality against an expectation, throwing if it doesn't match. `result_name`
becomes the returned frame's title (defaults to `left + "_" + right`).

```{important} relationship_name resolves keys/validate from a declared Relationship instead
Setting `spec.relationship_name` to a declared relationship's name looks that relationship
up and, **only for fields you left unset**, fills in `keys` (from the relationship's own
`keys`, oriented correctly regardless of which side `left`/`right` are) and `validate`
(from the relationship's `cardinality`). If you set `keys` explicitly *and* also give
`relationship_name`, your explicit `keys` win — the relationship is only consulted for
`cardinality`-based `validate` in that case. If two or more relationships connect the same
table pair and you don't name one explicitly, the call throws — see Example 3.
```

## Parameters

**left** / **right** (`std::string_view`)
    The two tables to join, in that role order. Both must already exist in the workspace.

**spec** (`const JoinSpec&`)
    The join configuration — see above. For `spec.how == JoinHow::Cross`, `keys` must be
    empty and `relationship_name` must not be set (a cross join has no join keys by
    definition).

## Returns

`DataFrame`
    The joined result, titled `spec.result_name` if given, otherwise `left + "_" + right`.

## Examples

Example 1. An explicit `JoinSpec` with keys spelled out directly.

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
    {"amount",      Slate(pluss::make_series({50.0, 75.0, 20.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

JoinSpec spec;
spec.keys = {{"customer_id", "customer_id"}};
spec.how = JoinHow::Left;

auto joined = biz.join("orders", "customers", spec);
std::cout << joined.ncases() << " " << joined.nfeatures();
```

### Output

```text
3 4
```

Example 2. The same join, driven by a declared relationship's `keys` instead of spelling
them out.

```cpp
Relationship rel;
rel.name = "orders_to_customers";
rel.left_table = "orders";
rel.right_table = "customers";
rel.keys = {{"customer_id", "customer_id"}};
rel.cardinality = RelationshipCardinality::ManyToOne;

auto linked = biz.add_relationship(rel);

JoinSpec via_relationship;
via_relationship.relationship_name = "orders_to_customers";

auto joined2 = linked.join("orders", "customers", via_relationship);
std::cout << joined2.ncases() << " " << joined2.nfeatures();
```

### Output

```text
3 4
```

Example 3. Unlike [`NexusTraverse::join`](../../nexus_traverse_api_reference), which
auto-detects a relationship connecting two tables when you don't specify one,
`Nexus::join()` never guesses — a `JoinSpec` with neither `keys` nor `relationship_name`
set always throws, even when a relationship declaration would have made the join
unambiguous.

```cpp
try {
  JoinSpec empty;
  auto bad = linked.join("orders", "customers", empty);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::join: join keys cannot be empty
```

## Notes

- Delegates directly to `left.unify(...)` internally — everything
  [`DataFrame::unify`](../dataframe/unify.md) documents about join semantics applies here.
- `relationship_name` only fills in fields you left unset on `spec` — see the important
  callout above.
- A `JoinHow::Cross` join accepts no `keys` and no `relationship_name`.
- This is a single hop between two named tables — for a multi-hop chain across several
  tables, see [`path()`](path.md)/[`plan()`](plan.md)/[`materialize()`](materialize.md), or
  the fluent [`NexusTraverse`](../../nexus_traverse_api_reference) builder.

## See Also

- [`Nexus::materialize`](materialize.md): chains several `join()`-equivalent steps from a
  `JoinPlan`.
- [`Nexus::plan`](plan.md) / [`path`](path.md): build the multi-hop plan `materialize()`
  consumes.
- [`Nexus::from`](from.md): the fluent, higher-level entry point into the same joining
  machinery.
- [`DataFrame::unify`](../dataframe/unify.md): the underlying join primitive.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="inject.html">
    <span class="label">Previous</span>
    <span class="title">inject</span>
  </a>
  <a class="next-link" href="keys.html">
    <span class="label">Next</span>
    <span class="title">keys</span>
  </a>
</div>
