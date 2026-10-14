<!-- pluss-hand-authored -->

# Locus::link, Locus::add_relationship, Locus::unlink, Locus::relationships, Locus::neighbors, Locus::path

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::link(const Relationship& rel) const -> Locus
Locus::link(std::string_view left, std::string_view left_key,
            std::string_view right, std::string_view right_key,
            RelationshipCardinality cardinality = ManyToOne) const -> Locus
Locus::link(std::string_view left, std::string_view right, std::string_view key,
            RelationshipCardinality cardinality = ManyToOne) const -> Locus
Locus::add_relationship(const Relationship& rel) const -> Locus
Locus::add_relationships(std::initializer_list<Relationship> rels) const -> Locus
Locus::unlink(std::string_view name) const -> Locus

Locus::relationships() const -> const std::vector<Relationship>&
Locus::links() const -> const std::vector<Relationship>&              // alias
Locus::has_relationship(std::string_view name) const -> bool
Locus::relationship(std::string_view name) const -> std::optional<Relationship>
Locus::neighbors(std::string_view table) const -> std::vector<std::string>
Locus::path(std::string_view from, std::string_view to) const -> std::optional<JoinPath>
```

## What It Does

`link()` declares a relationship between two tables and returns a new `Locus` with it
added -- `Locus` values are immutable, so this (like every non-`_`-suffixed mutator)
returns a modified copy rather than changing the receiver. The five-argument form
takes the left and right key column names separately; the four-argument form uses one
shared key name on both sides. `add_relationship`/`add_relationships` are the same
declaration, spelled to take a `Relationship` value (or several) directly -- useful
when you already have one built, for example restored from a saved workspace.
`cardinality` is enforced at join time ([`join`](join.md)), not at link time: `link()`
only checks that both tables and both key columns exist.

An unnamed relationship is auto-named `"left->right"`; linking the same pair again
gets a distinct auto-name (`"...#2"`, and so on) rather than colliding. An *explicit*
duplicate name is refused. `unlink()` removes a declared relationship by name, without
touching either catalog's data.

`relationships()`/`links()` are the same accessor under two names. `has_relationship`
and `relationship` look one up by name, the throwing-free and non-throwing forms.
`neighbors(table)` lists every table directly linked to `table`, in declaration order.
`path(from, to)` finds the shortest chain of links between two tables (fewest hops,
ties broken by declaration order); an empty-but-present path means `from == to`, and
no links at all between them is `std::nullopt`.

## Parameters

| Parameter | Meaning |
|---|---|
| `rel` / `rels` | A `Relationship` value (or several), built directly. |
| `left`, `right` | The two tables to link. |
| `left_key`, `right_key`, `key` | The key column(s) the link matches on. |
| `cardinality` | `OneToOne`, `OneToMany`, or `ManyToOne` (default); enforced by `join`, not by `link`. |
| `name` | A relationship's name, for `has_relationship`, `relationship`, and `unlink`. |
| `table` | A table name, for `neighbors`. |
| `from`, `to` | Two table names, for `path`. |

## Returns

The `link`/`add_relationship`/`add_relationships`/`unlink` family returns a new
`Locus`. `relationships`/`links` return a reference to the declared list.
`has_relationship` returns `bool`. `relationship` returns
`std::optional<Relationship>`. `neighbors` returns `std::vector<std::string>`.
`path` returns `std::optional<JoinPath>`.

## Errors

`link` throws on an unknown table, an unknown key column, or an explicit duplicate
relationship name. `neighbors` and `path` throw on an unknown table name (`path`
excepted when `from == to`, which needs no lookup).

## Examples

Example 1. Declaring two links, reading them back, and the auto-naming /
duplicate-name rules.

```cpp
auto shop = make_shop();
shop = shop.link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);
shop = shop.link("items", "item_order", "orders", "order_id", RelationshipCardinality::ManyToOne);
std::cout << "nrelationships: " << shop.nrelationships() << '\n';

std::cout << "has_relationship(orders->customers): " << shop.has_relationship("orders->customers") << '\n';
std::cout << "relationship(items->orders).cardinality == ManyToOne: "
          << (shop.relationship("items->orders")->cardinality == RelationshipCardinality::ManyToOne) << '\n';

std::cout << "neighbors(orders): ";
for (const auto& n : shop.neighbors("orders")) std::cout << n << ' ';
std::cout << '\n';

const auto twice = shop.link("orders", "cust_id", "customers", "id");
std::cout << "twice has_relationship(orders->customers#2): " << twice.has_relationship("orders->customers#2") << '\n';

Relationship dup;
dup.name = "orders->customers";
dup.left_table = "orders";
dup.right_table = "customers";
dup.keys = {{"cust_id", "id"}};
try {
  (void)shop.link(dup);
  std::cout << "duplicate link: did not throw\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << "duplicate link threw: " << msg.substr(0, msg.find('\n')) << '\n';
}

const auto fewer = shop.unlink("items->orders");
std::cout << "after unlink, has_relationship(items->orders): " << fewer.has_relationship("items->orders") << '\n';
std::cout << "shop still has_relationship(items->orders): " << shop.has_relationship("items->orders") << '\n';
```

### Output

```text
nrelationships: 2
has_relationship(orders->customers): true
relationship(items->orders).cardinality == ManyToOne: true
neighbors(orders): customers items 
twice has_relationship(orders->customers#2): true
duplicate link threw: Locus::link: duplicate relationship name 'orders->customers'
after unlink, has_relationship(items->orders): false
shop still has_relationship(items->orders): true
```

`unlink` on `shop` returns `fewer`, a separate `Locus` -- `shop` itself is unaffected,
the same immutable-handle pattern every non-`_` mutator follows.

Example 2. `path`: the shortest chain of links, a self-path, and an unreachable table.
Continues from Example 1's `shop` (still linked `orders->customers` and
`items->orders` -- `unlink` above returned a separate `Locus`, `fewer`, and left
`shop` itself unchanged).

```cpp
const auto to_customers = shop.path("items", "customers");
std::cout << "path(items, customers) hops: " << to_customers->steps.size() << '\n';
std::cout << "hop 0: " << to_customers->steps[0].from_table << " -> " << to_customers->steps[0].to_table << '\n';
const auto self = shop.path("orders", "orders");
std::cout << "path(orders, orders) hops: " << self->steps.size() << '\n';
const auto isolated = shop.with("island", shop.catalog("customers"));
std::cout << "path(island, orders) has value: " << isolated.path("island", "orders").has_value() << '\n';
```

### Output

```text
path(items, customers) hops: 2
hop 0: items -> orders
path(orders, orders) hops: 0
path(island, orders) has value: false
```

`items` reaches `customers` in two hops (through `orders`). A table reaches itself in
zero hops, which is a present-but-empty path, not `std::nullopt` -- that's reserved
for genuinely unreachable tables, like `island` (linked to nothing).

## See Also

- [`audit`](audit.md): checking a declared link's cardinality and orphan rows against
  the real data.
- [`join`](join.md): where a link's declared keys and cardinality actually get used.
