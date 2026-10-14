<!-- pluss-hand-authored -->

# LocusTraverse::join

[Back to LocusTraverse API Reference](../../locus_traverse_api_reference)

## Signatures

```cpp
LocusTraverse LocusTraverse::join(std::string_view right) const
LocusTraverse LocusTraverse::join(
    std::string_view right,
    const pluss::bundle::locus::JoinSpec& spec) const
```

## Appending One Hop

`join()` adds exactly one catalog to the current traversal anchor. It is the
right operation when the next catalog and its relationship are known. Use
[`via()`](via.md) when the destination may require several declared links and
the graph should choose the shortest route.

The one-argument overload uses `JoinSpec{}`. Its default mode is `Inner`, and
the sole direct relationship between the current anchor and `right` supplies
the key pairs and cardinality contract. A missing relationship is an error, and
multiple direct relationships are never guessed between.

The `JoinSpec` overload can name one of several declared relationships, provide
key pairs without any declaration, change the matching mode, impose a
cardinality contract, and control right-side collision suffixing. After the hop
is appended, `right` becomes the anchor for the next `join()` or `via()` call.
The data is read only when `collect()` runs.

## Parameters

**right** (`std::string_view`)
: Name of the catalog to append. It must already exist in the captured Locus.
  The name is copied into the returned builder.

**spec** (`const locus::JoinSpec&`)
: Complete description of this hop.

| Field | Meaning | Default |
|---|---|---|
| `keys` | Ordered `(current-key, right-key)` pairs. Several pairs form one composite key. When empty, a named or uniquely inferred relationship supplies them. | `{}` |
| `how` | Record-retention mode: `Inner`, `Left`, `Right`, `Outer`, `Cross`, `LeftOnly`, or `RightOnly`. | `Inner` |
| `right_suffix` | Suffix for a right stream whose name collides with a left stream and is not coalesced. An empty value is normalized by this builder to `"_" + right`. | `""` |
| `validate` | Optional key cardinality to enforce against actual matches. | unset |
| `relationship_name` | Exact declared relationship to orient between the current anchor and `right`. | unset |
| `result_name` | Plan-label metadata for the hop. It does not rename output streams. | unset |

When a declared relationship supplies keys, its declared cardinality is
oriented to the traversal direction and enforced unless `validate` explicitly
replaces it. Walking a declaration in reverse also reverses `OneToMany` and
`ManyToOne`.

## Matching Modes

| Mode | Records retained |
|---|---|
| `Inner` | Matched pairs only. Repeated keys may produce several rows. |
| `Left` | Every accumulated-left record, with null right streams where no partner exists. |
| `Right` | Every right record, with null accumulated-left streams where no partner exists. |
| `Outer` | Every match plus unmatched records from both sides. |
| `Cross` | Cartesian product. It accepts neither keys nor a relationship name. |
| `LeftOnly` | Accumulated-left records that have no partner. |
| `RightOnly` | Right records that have no partner. |

Null or absent key values never match. Match order is deterministic: each mode
preserves the relational kernel's stable side order, and repeated-key matches
follow source record order.

## Returns

`LocusTraverse`
: A new immutable builder with one additional plan step and `right` as its
  current anchor. The source builder remains reusable.

## Examples

All outputs are emitted by
`PlussMic/examples/bundle/locus_traverse_reference_demo.cpp`.

Example 1. The inferred relationship and default inner mode retain four of five
orders because customer `9` has no partner.

```cpp
const auto result = data.from("orders").join("customers").collect();
print_keys(result);
print_words(result, "name");
```

### Output

```text
keys=[order_id, customer_id, amount, state, name] rows=4
name=[Ada, Bola, Bola, Chen]
```

Example 2. All seven modes are available. This fixture has one unmatched order,
one unmatched customer, five orders, and four customers.

```cpp
for (const auto [name, how] : {
         std::pair{"inner", JoinHow::Inner},
         std::pair{"left", JoinHow::Left},
         std::pair{"right", JoinHow::Right},
         std::pair{"outer", JoinHow::Outer},
         std::pair{"left_only", JoinHow::LeftOnly},
         std::pair{"right_only", JoinHow::RightOnly},
     }) {
  const auto result = data.from("orders").join("customers", mode(how)).collect();
  std::cout << name << " rows=" << result.maxrows() << "\n";
}
const auto crossed = shop(false).from("orders")
    .join("customers", mode(JoinHow::Cross)).collect();
std::cout << "cross rows=" << crossed.maxrows() << "\n";
```

### Output

```text
inner rows=4
left rows=5
right rows=5
outer rows=6
left_only rows=1
right_only rows=1
cross rows=20
```

Example 3. Explicit keys allow a join in a Locus with no declared links. `Left`
keeps the unmatched order and materializes its right streams as null.

```cpp
JoinSpec manual;
manual.keys = {{"customer_id", "customer_id"}};
manual.how = JoinHow::Left;
const auto result = shop(false).from("orders").join("customers", manual).collect();
print_numbers(result, "order_id");
print_words(result, "name");
```

### Output

```text
order_id=[100, 101, 102, 103, 104]
name=[Ada, Bola, Bola, Chen, null]
```

Example 4. When two declarations connect the same catalogs, the short overload
fails rather than selecting by declaration order. Name the intended relationship.

```cpp
print_error("ambiguous", [&] {
  (void)ambiguous.from("orders").join("customers");
});
JoinSpec named;
named.relationship_name = "orders_to_buyers";
print_keys(ambiguous.from("orders").join("customers", named).collect());
```

### Output

```text
ambiguous: LocusTraverse::join: multiple direct relationships connect 'orders' and 'customers'. Specify JoinSpec.relationship_name explicitly
keys=[order_id, customer_id, amount, state, name] rows=4
```

Example 5. Target existence and relationship ambiguity are checked while the
builder is formed. Explicit key existence and cardinality require data, so they
are checked at collection.

```cpp
print_error("unknown catalog", [&] {
  (void)data.from("orders").join("absent");
});
print_error("no relationship", [&] {
  (void)shop(false).from("orders").join("customers");
});

JoinSpec bad_key;
bad_key.keys = {{"missing", "customer_id"}};
const auto lazy = data.from("orders").join("customers", bad_key);
std::cout << "bad key plan built\n";
print_error("bad key collect", [&] { (void)lazy.collect(); });
```

### Output

```text
unknown catalog: LocusTraverse::join: unknown catalog 'absent'
no relationship: LocusTraverse::join: no direct relationship connects 'orders' and 'customers'
bad key plan built
bad key collect: Locus::materialize: the accumulated result has no key 'missing' for the step from 'orders' (a column may have been renamed by an earlier suffix)
```

Example 6. An explicit cardinality contract is checked against actual keys.

```cpp
JoinSpec strict;
strict.keys = {{"customer_id", "customer_id"}};
strict.validate = RelationshipCardinality::OneToOne;
print_error("cardinality", [&] {
  (void)data.from("orders").join("customers", strict).collect();
});
```

### Output

```text
cardinality: Catalog::join: left keys repeat, contradicting the declared one-to-one contract
```

## Schema And Cost

Left streams appear first. Right streams follow in their original order; a
same-named key may be coalesced according to `JoinOptions`, while other
collisions receive `right_suffix`. A suffix that creates a second collision is
an error rather than an invitation to rename silently.

Each collected hop encodes the key streams, matches records, gathers every
output stream, and allocates a new Catalog. Composite keys increase encoding
work with the number of components. A plan with several adjacent hops uses the
previous output as the next left side, so avoid carrying unused wide streams
through long chains when an alternate pre-shaped Catalog is appropriate.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html"><span class="label">Previous</span><span class="title">drop</span></a>
  <a class="next-link" href="options.html"><span class="label">Next</span><span class="title">options</span></a>
</div>
