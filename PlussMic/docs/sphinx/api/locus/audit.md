<!-- pluss-hand-authored -->

# Locus::audit, Locus::validate, Locus::validate_keys, Locus::duplicates, Locus::detect_orphans, Locus::check

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::audit(const JoinOptions& options = {}) const -> std::vector<LinkAudit>
Locus::audit(std::string_view relationship, const JoinOptions& options = {}) const -> LinkAudit
Locus::validate(const JoinOptions& options = {}) const -> void
Locus::validate_keys(std::string_view table) const -> bool
Locus::validate_keys(std::string_view table, std::initializer_list<std::string_view> keys) const -> bool
Locus::duplicates(std::string_view table) const -> Catalog
Locus::duplicates(std::string_view table, std::initializer_list<std::string_view> keys) const -> Catalog
Locus::validate_relationship(std::string_view name) const -> bool
Locus::check(std::string_view name) const -> bool                           // alias
Locus::detect_orphans(std::string_view relationship_name,
                       RelationshipSide side = RelationshipSide::Left) const -> Catalog
```

## What It Does

`audit()` checks every declared relationship against the aligned data and returns one
`LinkAudit` per link; the one-argument overload checks a single named relationship.
Each `LinkAudit` reports both sides' row counts, whether each side's key is unique,
how many rows on each side have no partner (`left_orphans`/`right_orphans`), and a
`status` -- `Declared` is unreached, `Warning` is orphans on an optional side,
`Invalid` is a cardinality violation or a missing key. `validate()` throws only on
`Invalid`; `Warning` is reported, never raised, since an optional-side orphan is
ordinary data, not a broken workspace.

`validate_keys(table, keys)` checks whether `keys` (the table's declared primary key,
when `keys` is omitted) are present and hold unique values -- the same check `join`
and aggregating [`pull`](../../locus_api_reference.md) rely on internally.
`duplicates(table, keys)` returns the actual rows whose key repeats, so a failed
`validate_keys` has something concrete to look at.

`validate_relationship`/`check` are the same single-relationship pass/fail check
(`check` is the shorter alias) -- `true` only when that link's `audit()` status is
*not* `Invalid`. `detect_orphans(name, side)` returns the records on one side of a
link that find no partner on the other.

## Parameters

| Parameter | Meaning |
|---|---|
| `options` | Join/raggedness options the audit's underlying alignment uses. |
| `relationship` / `name` | A declared relationship's name. |
| `table` | A table name, for `validate_keys` and `duplicates`. |
| `keys` | Key column names to check; defaults to the table's declared primary key. |
| `side` | `RelationshipSide::Left` or `Right`, for `detect_orphans`. |

## Returns

`audit()` (no name) returns `std::vector<LinkAudit>`; the named overload returns one
`LinkAudit`. `validate` returns `void` (throws instead of returning a verdict).
`validate_keys`, `validate_relationship`, and `check` return `bool`. `duplicates` and
`detect_orphans` return a `Catalog` of the matching rows.

## Errors

`validate()` throws when any link's status is `Invalid`. The others return a verdict
rather than throwing for a *failed* check; they still throw on an unknown table or
relationship name.

## Examples

Example 1. A workspace with a deliberate orphan: order `13` names a customer that
doesn't exist.

```cpp
// customers 1..3; orders 10..13 (order 13 points at a customer that does not exist).
auto shop = make_shop().link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);

const auto audits = shop.audit();
std::cout << "audits.size(): " << audits.size() << '\n';
const auto& a = audits[0];
std::cout << "left_unique (orders.cust_id): " << a.left_unique << '\n';
std::cout << "right_unique (customers.id): " << a.right_unique << '\n';
std::cout << "left_rows: " << a.left_rows << " right_rows: " << a.right_rows << '\n';
std::cout << "left_orphans (orders with no customer): " << a.left_orphans << '\n';
std::cout << "right_orphans (customers with no orders): " << a.right_orphans << '\n';
std::cout << "status == Warning: " << (a.status == RelationshipValidationStatus::Warning) << '\n';

const auto one = shop.audit("orders->customers");
std::cout << "audit(name).left_orphans: " << one.left_orphans << '\n';

shop.validate();
std::cout << "validate() on warning-only orphans: did not throw\n";
```

### Output

```text
audits.size(): 1
left_unique (orders.cust_id): false
right_unique (customers.id): true
left_rows: 4 right_rows: 3
left_orphans (orders with no customer): 1
right_orphans (customers with no orders): 1
status == Warning: true
audit(name).left_orphans: 1
validate() on warning-only orphans: did not throw
```

Orphans on *both* sides are reported (customer 3 has no orders, order 13 has no
customer) -- the status is `Warning`, not `Invalid`, because an unmatched row is
expected on a `ManyToOne` link's optional side, and `validate()` does not raise it.

Example 2. Key validation, duplicates, single-relationship checks, and
`detect_orphans`. Continues from Example 1's `shop`.

```cpp
std::cout << "validate_keys(customers, {id}): " << shop.validate_keys("customers", {"id"}) << '\n';
std::cout << "validate_keys(orders, {cust_id}): " << shop.validate_keys("orders", {"cust_id"}) << '\n';

const auto dup_rows = shop.duplicates("orders", {"cust_id"});
std::cout << "duplicates(orders, cust_id) rows: " << dup_rows.get("order_id")->size() << '\n';

const auto orphan_orders = shop.detect_orphans("orders->customers", RelationshipSide::Left);
std::cout << "detect_orphans(left) rows: " << orphan_orders.get("order_id")->size() << '\n';

std::cout << "validate_relationship(orders->customers): " << shop.validate_relationship("orders->customers") << '\n';
std::cout << "check(orders->customers) (alias): " << shop.check("orders->customers") << '\n';
```

### Output

```text
validate_keys(customers, {id}): true
validate_keys(orders, {cust_id}): false
duplicates(orders, cust_id) rows: 2
detect_orphans(left) rows: 1
validate_relationship(orders->customers): false
check(orders->customers) (alias): false
```

`customers.id` is unique, so `validate_keys` passes; `orders.cust_id` repeats
(customer `1` has two orders), so it fails. `validate_relationship`/`check` both
report `false` here even though `validate()` did not throw -- those two answer "is
this link currently in a perfectly clean state," a stricter question than "is the
workspace usable."

## See Also

- [`link`](link.md): declaring the relationships these checks examine.
- [`join`](join.md): where a link's declared cardinality is actually enforced, not
  just reported.
