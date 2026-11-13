<!-- pluss-hand-authored -->

# LocusPull::take

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
// Root fields, unaliased
LocusPull& LocusPull::take(pluss::msharp::RangeView::Of<std::string_view> sources)
LocusPull& LocusPull::take(std::initializer_list<std::string_view> sources)

// A single root (or related) field, with an optional alias
LocusPull& LocusPull::take(std::string_view source, std::string_view alias = {})

// Root fields, renamed as a batch of (source, alias) pairs
LocusPull& LocusPull::take(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
LocusPull& LocusPull::take(std::initializer_list<std::pair<std::string_view, std::string_view>> fields)

// Related-table fields, scoped under one table token
LocusPull& LocusPull::take(std::string_view table,
                           pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
LocusPull& LocusPull::take(std::string_view table,
                           std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
```

## What It Does

Requests one or more **plain** (non-aggregate) fields -- the fluent spelling
of [`add`](add.md) for `PullOp::Take` specs, built through `nexus::take`
under the hood. Which overload you reach depends only on the shape of field
list you're describing:

- **A list of root feature names** -- `.take({"id", "name"})` -- pulls those
  features from `root` itself, each keeping its own name.
- **One field, optionally renamed** -- `.take("name", "customer")` -- the
  single-field form; omit the alias to keep the source name. This is also the
  form that reaches a *related* field directly, written
  `"regions.zone"` -- see Example 3.
- **A batch of `(source, alias)` renames on root** --
  `.take({{"id", "customer_id"}, {"name", "customer_name"}})`.
- **A batch of related-table fields, scoped under one table token** --
  `.take("regions", {{"zone", "area"}})` -- equivalent to
  `.take("regions.zone", "area")`, without repeating `"regions."` on every
  pair.

Every batch overload also accepts a named container (`std::vector`, and
anything else `pluss::msharp::RangeView` converts from) in place of a brace
literal, not just a literal `{...}` -- the dedicated `initializer_list`
overload alongside each `RangeView` one exists specifically so a brace
literal still resolves to the right overload unambiguously (see Example 6
for the one case this doesn't cover: a bare, empty `{}`).

```{important} A plain field cannot cross a one-to-many link
`.take(...)` only ever follows **root-preserving** links (many-to-one or
one-to-one) -- a field on a table reached through a one-to-many or
many-to-many relationship would require more than one output row per root
record, which `take()` cannot produce. Requesting one throws at
[`collect()`](collect.md); see Example 5. Use a reducer
([`sum`](sum.md), [`count`](count.md), and the rest, or
[`agg()`](agg.md)) to pull a value *through* such a link instead.
```

## Parameters

| Parameter | Meaning |
|---|---|
| `sources` | Root feature names to pull unchanged, each keeping its own name. |
| `source` | A single feature -- a root name, or `"table.feature"` for a related field. |
| `alias` | The output column name for the single-field form. Empty means "keep `source`'s own name." |
| `fields` | Several `(source, alias)` pairs; `source` is root-relative in the unscoped overloads, `table`-relative in the scoped ones. |
| `table` | The related table every entry in `fields` is qualified against, in the two-argument overloads. |

Every field name is validated only when [`collect()`](collect.md) actually
runs -- an unknown feature, an unreachable table, or an expanding path all
surface there.

## Returns

A reference to the same builder, so calls chain in place.

## Errors

Throws at [`collect()`](collect.md), not at `take()` itself: an empty source
or table name, an unknown feature, an unreachable table, or a field that
would require root to expand into more than one row.

## Examples

Example 1. Plain root fields and a related field, by brace literal.

```cpp
auto plain = data.pull("customers").take("id").take("regions.zone").collect();
print_keys(plain);
print_numbers(plain, "id");
print_words(plain, "zone");
```

### Output

```text
keys=[id, zone] rows=4
id=[1, 2, 3, 4]
zone=[west, west, east, north]
```

Example 2. Single-field renames.

```cpp
auto renamed = data.pull("customers").take("name", "customer").take("regions.zone", "area").collect();
print_keys(renamed);
```

### Output

```text
keys=[customer, area] rows=4
```

Example 3. A batch of root names, and a batch of root renames.

```cpp
auto names = data.pull("customers").take({"name", "regions.zone", "id"}).collect();
print_keys(names);

auto aliases = data.pull("customers")
                   .take({std::pair<std::string_view, std::string_view>{"id", "customer_id"},
                          {"name", "customer_name"}})
                   .collect();
print_keys(aliases);
```

### Output

```text
keys=[name, zone, id] rows=4
keys=[customer_id, customer_name] rows=4
```

Example 4. The two-argument, table-scoped batch form.

```cpp
auto scoped = data.pull("customers")
                  .take("id")
                  .take("regions", {{"region_id", "region"}, {"zone", "area"}})
                  .collect();
print_keys(scoped);
```

### Output

```text
keys=[id, region, area] rows=4
```

Example 5. Failures: an empty source, an empty table, an unknown feature
(stored without complaint, rejected at `collect()`), and a field that would
expand root.

```cpp
print_error("empty source", [&] { (void)data.pull("customers").take(""); });
print_error("empty table", [&] {
  (void)data.pull("customers").take("", {{"zone", "area"}});
});
const auto unknown = data.pull("customers").take("regions.absent");
std::cout << "unknown plan specs=" << unknown.specs().size() << "\n";
print_error("unknown collect", [&] { (void)unknown.collect(); });
print_error("expanding", [&] { (void)data.pull("customers").take("events.spend").collect(); });
```

### Output

```text
empty source: pull spec source cannot be empty
empty table: pull table scope cannot be empty
unknown plan specs=1
unknown collect: Locus::pull: unknown feature 'absent' on table 'regions'
expanding: Locus::pull: requested field 'events.spend' requires expanding relationship 'events->customers' from 'customers' to 'events'. pull preserves root grain and cannot traverse one-to-many or many-to-many paths
```

An unknown feature doesn't fail `take()` itself -- the spec is stored (`specs`
reports `1`) and only rejected once `collect()` tries to resolve it.
`events.spend` fails differently: `customers -[ManyToOne]-> ... <-[OneToMany]-
events` means one customer has *several* events, so a plain field from
`events` can't become one value per customer -- that's exactly what
[`sum`](sum.md)/[`agg`](agg.md) are for.

Example 6. An empty batch, on all three batch overloads, is a no-op -- it
appends nothing. A bare `{}` is ambiguous between the batch overloads'
shapes, so an empty call site needs its `initializer_list<T>` spelled out
explicitly; `take("regions", {})` does not have this problem, since the
`table` argument already selects the two-argument overload.

```cpp
auto empty_lists = data.pull("customers");
empty_lists.take(std::initializer_list<std::string_view>{});
empty_lists.take(std::initializer_list<std::pair<std::string_view, std::string_view>>{});
empty_lists.take("regions", std::initializer_list<std::pair<std::string_view, std::string_view>>{});
std::cout << "specs=" << empty_lists.specs().size() << "\n";
print_error("collect", [&] { (void)empty_lists.collect(); });
```

### Output

```text
specs=0
collect: Locus::pull: at least one field is required
```

Three empty batches add zero specs, not an error -- the error comes later,
from [`collect()`](collect.md) refusing a plan with no fields requested at
all, not from any one of the empty calls.

## See Also

- [`add`](add.md): the lower-level method every `take(...)` overload funnels
  through.
- [`sum`](sum.md) and the other reducers, [`agg`](agg.md): the aggregate
  counterparts, for fields reached through a one-to-many link.
