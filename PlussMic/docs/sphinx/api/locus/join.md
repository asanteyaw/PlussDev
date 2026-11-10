<!-- pluss-hand-authored -->

# Locus::join, Locus::join_detailed

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::join(std::string_view left, std::string_view right,
            const JoinSpec& spec = {}, const JoinOptions& options = {}) const -> Catalog
Locus::join(const JoinPath& path, JoinHow how = JoinHow::Left,
            const JoinOptions& options = {}) const -> Catalog
Locus::join_detailed(std::string_view left, std::string_view right,
                      const JoinSpec& spec = {}, const JoinOptions& options = {}) const -> LocusJoinResult
```

## What It Does

Joins two catalogs already in the workspace and returns the result directly. With no
keys set in `spec`, the declared link between `left` and `right` supplies them --
an error if more than one link connects the pair -- and that link's cardinality is
enforced; set `spec.keys` to join on explicit columns without any declared link at
all. Matching is done on the key streams alone and every other stream is rebuilt
through the matches, so a ragged catalog's raggedness is preserved on both sides of
the join, not flattened.

The `JoinPath` overload joins along a chain of links, accumulating columns table by
table -- [`path()`](link.md) finds the shortest such chain, and each hop enforces its
own link's cardinality, the same as a single-hop `join()` would.

`join_detailed()` is the same join, returning both the `Catalog` (`.catalog`) and a
`CatalogJoinReport` (`.report`) describing what was matched, left without a partner,
or omitted -- `join()` itself discards that report.

### `JoinSpec` fields

| Field | Type | Default |
|---|---|---|
| `keys` | `std::vector<std::pair<std::string, std::string>>` | empty |
| `how` | `JoinHow` | `JoinHow::Inner` |
| `right_suffix` | `std::string` | empty |
| `validate` | `std::optional<RelationshipCardinality>` | empty |
| `relationship_name` | `std::optional<std::string>` | empty |
| `result_name` | `std::optional<std::string>` | empty |

| Field | Meaning | Default |
|---|---|---|
| `keys` | `(left_feature, right_feature)` pairs to join on. Supplied by the declared link when empty. | empty |
| `how` | `Inner`, `Left`, `Right`, `Outer`, or `Cross`. | `Inner` |
| `right_suffix` | Disambiguates overlapping non-key column names from the right side. | `"_" + right` |
| `validate` | An expected cardinality to check the join's actual result against. | unset |
| `relationship_name` | Which declared link to resolve `keys`/`validate` from, when several connect the same pair. | unset |
| `result_name` | The returned `Catalog`'s title. | `left + "_" + right` |

## Parameters

| Parameter | Meaning |
|---|---|
| `left`, `right` | The two tables to join. |
| `spec` | The `JoinSpec` above. |
| `path` | A `JoinPath` (from [`path()`](link.md)), for the multi-hop overload. |
| `how` | The join kind for every hop of a `JoinPath` join. |
| `options` | Raggedness policy for how unmatched/short streams are handled. |

## Returns

`join` returns the joined `Catalog` directly. `join_detailed` returns a
`LocusJoinResult` -- `.catalog` (the same `Catalog`) and `.report` (a
`CatalogJoinReport`, with its own `.summary()`).

## Errors

Throws when neither `spec.keys` nor a single unambiguous declared link is available,
when a named key column doesn't exist, when an unknown table is named, or when the
join's actual cardinality contradicts a declared or requested one.

## Examples

Example 1. Inner (default), left, and explicit-key joins, and the no-link failure.

```cpp
// customers 1..3; orders 10..13 (order 13's customer, 9, does not exist).
const auto shop = make_shop().link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);

const auto joined = shop.join("orders", "customers");
std::cout << "inner join rows: " << joined.get("amount")->size() << '\n';

JoinSpec left_spec;
left_spec.how = JoinHow::Left;
const auto kept = shop.join("orders", "customers", left_spec);
std::cout << "left join rows: " << kept.get("amount")->size() << '\n';

JoinSpec explicit_keys;
explicit_keys.keys = {{"cust_id", "id"}};
const auto manual = make_shop().join("orders", "customers", explicit_keys);
std::cout << "explicit-key join rows: " << manual.get("amount")->size() << '\n';

try {
  (void)make_shop().join("orders", "customers");
  std::cout << "unlinked join: did not throw\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << "unlinked join threw: " << msg.substr(0, msg.find('\n')) << '\n';
}
```

### Output

```text
inner join rows: 3
left join rows: 4
explicit-key join rows: 3
unlinked join threw: Locus::join: no direct relationship connects 'orders' and 'customers'
```

The default `Inner` join drops order `13` (no matching customer); `Left` keeps all
four orders, with a null on the customer side for the orphan. With no declared link
at all, `join()` with no explicit keys has nothing to join on and throws.

Example 2. `join_detailed()`: the same join, plus a report. Continues from
Example 1's `shop`.

```cpp
const auto detailed = shop.join_detailed("orders", "customers");
std::cout << "join_detailed matched: " << detailed.catalog.get("amount")->size() << '\n';
std::cout << "join_detailed report: " << detailed.report.summary() << '\n';
```

### Output

```text
join_detailed matched: 3
join_detailed report: 3 output record(s); 3 matched pair(s). left: 1 of 4 record(s) without a partner (0 without a key), 1 omitted. right: 1 of 3 without a partner (0 without a key), 1 omitted.
```

The report's own prose names exactly what `join()` alone would leave you to infer: one
order and one customer each had no partner, and each was omitted (the default `Inner`
semantics), not silently dropped without a count.

## See Also

- [`link`](link.md): declaring the relationship a keyless `join()` call resolves.
- [`audit`](audit.md): checking cardinality and orphans ahead of a join, without
  joining anything.
