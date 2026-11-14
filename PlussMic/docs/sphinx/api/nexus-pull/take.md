<!-- pluss-hand-authored -->

# NexusPull::take

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
// Root fields, unaliased
NexusPull& NexusPull::take(pluss::msharp::RangeView::Of<std::string_view> sources)
NexusPull& NexusPull::take(std::initializer_list<std::string_view> sources)

// Root fields, one at a time, with an optional alias
NexusPull& NexusPull::take(std::string_view source, std::string_view alias = {})

// Root fields, renamed as a batch of (source, alias) pairs
NexusPull& NexusPull::take(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
NexusPull& NexusPull::take(std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
NexusPull& NexusPull::take(const std::map<Key, Value, Compare, Alloc>& fields)
NexusPull& NexusPull::take(const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields)

// Related-table fields, scoped under one table token
NexusPull& NexusPull::take(std::string_view table, pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
NexusPull& NexusPull::take(std::string_view table, std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
NexusPull& NexusPull::take(std::string_view table, const std::map<Key, Value, Compare, Alloc>& fields)
NexusPull& NexusPull::take(std::string_view table, const std::unordered_map<Key, Value, Hash, Eq, Alloc>& fields)
```

## What It Does

Requests one or more **plain** (non-aggregate) fields — the fluent-builder
spelling of [`Nexus::pull`](../nexus/pull.md)'s bare-name and `table.feature`
field expressions. Every overload ultimately builds one or more
[`PullSpec`](../nexus/pull.md) values via the free `nexus::take(source, alias)`
factory and appends them via [`add()`](add.md); which overload you reach
depends only on what shape of *field list* you're describing:

- **A list of root feature names** — `.take({"a", "b"})` — pulls those
  features from `root` itself, each keeping its own name as the output
  column.
- **One root feature, optionally renamed** — `.take("a", "alias")` — the
  single-field convenience form; omit the alias to keep the source name.
- **A batch of `(source, alias)` renames on root** — `.take({{"a", "x"},
  {"b", "y"}})` — several root features renamed in one call.
- **A batch of related-table features, scoped under one table token** —
  `.take("regions", {{"zone", "zone"}})` — equivalent to calling
  `.take("regions.zone", "zone")` once per pair, without repeating
  `"regions."` on every entry. This is the two-argument family; the table
  name is a real parameter, not baked into the string.

:::{note} `RangeView` is never spelled out; brace calls resolve unambiguously
Every batch form above also accepts a plain `{...}` brace list directly
(that's exactly what the dedicated `initializer_list` overloads are for —
see the [index page](../../nexus_pull_api_reference.md)'s note on this).
The examples below write braces for the brace-literal forms and pass a
named container variable (`std::vector`, `std::map`, `std::unordered_map`)
for every other form, never naming `RangeView` itself.
:::

```{important} The unscoped rename-pairs form needs a named container, not a literal double-brace list
`.take({{"a", "x"}, {"b", "y"}})` — braces around a list of pairs, with no
table argument — is genuinely **ambiguous** against
`.take({"a", "b"})`'s plain-names overload: the compiler cannot tell, from
the braces alone, whether you mean two root names or two rename pairs, and
refuses to guess. This is *not* a problem for the two-argument
`take(table, {{...}})` form (Example 5 below), since supplying `table`
picks a distinct overload outright. For the *unscoped* batch-rename case,
pass a named `std::vector`/`std::map`/`std::unordered_map` instead of a
literal brace list — see Example 4.
```

## Parameters

**sources** (a range of `std::string_view`)
    Root feature names to pull unchanged, each keeping its own name.

**source** (`std::string_view`)
    A single root feature name.

**alias** (`std::string_view`, default empty)
    The output column name for the single-field form. Empty means "keep
    `source`'s own name."

**fields** (a range of `(source, alias)` pairs, or a `map`/`unordered_map`
of the same)
    Several fields renamed in one call. In the unscoped forms, `source` is a
    root feature name; in the `table`-scoped forms, `source` is a feature on
    `table`, automatically qualified as `"table.source"`.

**table** (`std::string_view`, table-scoped overloads only)
    The related table every entry in `fields` is qualified against.

Every field name is validated only when [`.collect()`](collect.md) actually
executes — an unknown feature or an unreachable table surfaces there, the
same as every other pull field.

## Returns

`NexusPull&`
    A reference to the same builder, so calls chain in place.

## Examples

Example 1. Plain root fields, brace literal.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

// nx: the customers/regions/events workspace from Nexus::pull's own page,
// with customers.region_id -[ManyToOne]-> regions.region_id and
// customers.customer_id -[OneToMany]-> events.customer_id.
auto pulled = nx.pull("customers").take({"customer_id", "region_id"}).collect();
std::cout << pulled.features();
```

### Output

```text
customer_id region_id
```

Example 2. The same request via a named `std::vector<std::string_view>`
(the `RangeView` overload) and, separately, a named
`std::vector<std::string>` (the templated `NameRange` overload) — both
produce the identical result as Example 1's brace literal.

```cpp
std::vector<std::string_view> view_fields = {"customer_id", "region_id"};
auto a = nx.pull("customers").take(view_fields).collect();

std::vector<std::string> owned_fields = {"customer_id", "region_id"};
auto b = nx.pull("customers").take(owned_fields).collect();

std::cout << a.features() << "\n" << b.features();
```

### Output

```text
customer_id region_id
customer_id region_id
```

Example 3. A single field with an explicit rename.

```cpp
auto pulled = nx.pull("customers").take("customer_id", "cust_id").collect();
std::cout << pulled.features();
```

### Output

```text
cust_id
```

Example 4. A batch of root renames — via a named `std::vector` of pairs, a
`std::map`, and a `std::unordered_map` (the literal double-brace form is
ambiguous here; see the callout above).

```cpp
std::vector<std::pair<std::string_view, std::string_view>> as_vector = {
    {"customer_id", "cust_id"}, {"region_id", "reg_id"}};
auto a = nx.pull("customers").take(as_vector).collect();

std::map<std::string, std::string> as_map = {
    {"customer_id", "cust_id"}, {"region_id", "reg_id"}};
auto b = nx.pull("customers").take(as_map).collect();

std::unordered_map<std::string, std::string> as_umap = {{"customer_id", "cust_id"}};
auto c = nx.pull("customers").take(as_umap).collect();

std::cout << a.features() << "\n" << b.features() << "\n" << c.features();
```

### Output

```text
cust_id reg_id
cust_id reg_id
cust_id
```

Example 5. Related-table fields, scoped under one table token — the
two-argument form. The brace-literal pairs are unambiguous here, since
`table` already selects this overload.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .take("regions", {{"zone", "zone"}})
    .collect();
std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  zone
------------------------
0    | 1            west
1    | 2            east
2    | 3            west
------------------------
[3 cs x 2 feat]
```

Example 6. The same table-scoped request via a named container instead of a
literal list — all three (`std::vector` of pairs, `std::map`,
`std::unordered_map`) work identically for the scoped form too.

```cpp
std::vector<std::pair<std::string_view, std::string_view>> as_vector = {
    {"zone", "region_zone"}};
auto a = nx.pull("customers").take("regions", as_vector).collect();

std::map<std::string, std::string> as_map = {{"zone", "region_zone"}};
auto b = nx.pull("customers").take("regions", as_map).collect();

std::cout << a.features() << "\n" << b.features();
```

### Output

```text
region_zone
region_zone
```

## Notes

- The unscoped single-source form (`.take("customer_id")`) and the
  two-argument table-scoped form (`.take("regions", {...})`) are the two
  building blocks; every batch overload is sugar that calls one of them
  repeatedly.
- `.take(table, ...)`'s `table` does not have to be `root` itself, and does
  not have to be a direct relationship — the same shortest-path,
  root-preserving walk [`Nexus::pull`](../nexus/pull.md) documents applies
  here too. A path requiring `root` to expand into several rows throws at
  `.collect()` time.
- For **aggregates** over a related table (`sum`, `count`, and the rest),
  use [`NexusPull::sum`](sum.md) and its siblings, or
  [`NexusPull::agg(table)`](agg.md) — `.take(...)` only ever pulls a plain
  value, never a reduction.

## See Also

- [`NexusPull::add`](add.md): the lower-level method every `.take(...)`
  overload funnels through.
- [`NexusPull::agg`](agg.md): the aggregate counterpart, scoped the same way
  the two-argument `.take(table, ...)` form is.
- [`Nexus::pull`](../nexus/pull.md): the full field-expression grammar,
  aliasing defaults, and root-preserving path rule shared by every spelling
  of a pull.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="var.html">
    <span class="label">Next</span>
    <span class="title">var</span>
  </a>
</div>
