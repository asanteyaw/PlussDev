<!-- pluss-hand-authored -->

# LocusTraverse::via

[Back to LocusTraverse API Reference](../../locus_traverse_api_reference)

## Signature

```cpp
LocusTraverse LocusTraverse::via(
    std::string_view to,
    pluss::bundle::locus::JoinHow how = pluss::bundle::locus::JoinHow::Left) const
```

## Traversing A Relationship Path

`via()` connects the current anchor to a target catalog through the fewest
declared relationships. It appends every hop of that path to the existing plan,
then makes `to` the new anchor. Use it when the destination matters more than
the intermediate catalogs. Use [`join()`](join.md) when each adjacent hop,
key set, suffix, or cardinality override must be written explicitly.

Path resolution occurs when `via()` is called, not at collection. The captured
Locus must contain both endpoint names and a connecting route. If several
fewest-hop routes exist, declaration order deterministically chooses one. Each
path step retains its relationship name, oriented key pairs, and appropriately
reversed cardinality.

The same `how` value is assigned to every appended hop. The default is `Left`,
not the `Inner` default of `join()`. Consequently, a path normally preserves
the records that have survived on the accumulated left side at each step and
materializes unavailable right streams as null. `Inner` narrows at every hop;
an item lacking an order or an order lacking a customer is removed as soon as
that step executes.

Calling `via()` with the current anchor names a valid zero-hop route. It appends
nothing and returns an equivalent plan.

## Parameters

**to** (`std::string_view`)
: Existing target catalog name. The name is copied into the returned builder.

**how** (`locus::JoinHow`)
: Matching mode applied uniformly to every hop. The default is `Left`. All
  seven join modes are accepted, although multi-hop `Cross` is rarely useful:
  a path still determines which catalogs are visited, but each hop forms a
  Cartesian product rather than using its relationship keys.

## Returns

`LocusTraverse`
: A new immutable builder containing the appended path and `to` as its current
  anchor. No Catalog is materialized until `collect()`.

## Examples

All outputs are emitted by the permanent Locus traversal reference executable.

Example 1. `items` reaches `customers` through `orders`. Default left traversal
keeps the item whose order points to an absent customer and gives it a null name.

```cpp
const auto result = data.from("items")
    .via("customers")
    .pick({"item_id", "name"})
    .collect();
print_numbers(result, "item_id");
print_words(result, "name");
```

### Output

```text
item_id=[1000, 1001, 1002, 1003]
name=[Ada, Ada, Bola, null]
```

Example 2. Changing the uniform mode to `Inner` removes the unmatched path at
the customer hop. `Cross` uses the path only to choose catalogs and forms a
Cartesian product at each edge.

```cpp
std::cout << "left rows="
          << data.from("items").via("customers").collect().maxrows() << "\n";
std::cout << "inner rows="
          << data.from("items").via("customers", JoinHow::Inner)
                 .collect().maxrows()
          << "\n";
std::cout << "cross rows="
          << data.from("items").via("customers", JoinHow::Cross)
                 .collect().maxrows()
          << "\n";
```

### Output

```text
left rows=4
inner rows=3
cross rows=80
```

Example 3. A zero-hop route preserves the anchor schema and records.

```cpp
print_keys(data.from("orders").via("orders").collect());
```

### Output

```text
keys=[order_id, customer_id, amount, state] rows=5
```

Example 4. The returned target becomes the anchor, so a later operation starts
there. After `via("orders")`, `join("customers")` resolves the direct
orders-to-customers relationship rather than looking for an items-to-customers
edge.

```cpp
const auto chained = data.from("items")
    .via("orders")
    .join("customers")
    .pick({"item_id", "name"})
    .collect();
print_numbers(chained, "item_id");
print_words(chained, "name");
```

### Output

```text
item_id=[1000, 1001, 1002]
name=[Ada, Ada, Bola]
```

Example 5. Unknown and disconnected targets fail during route construction,
before any call to `collect()`.

```cpp
print_error("unknown target", [&] {
  (void)data.from("orders").via("absent");
});
const auto island = data.with("island", data.catalog("customers"));
print_error("no path", [&] {
  (void)island.from("orders").via("island");
});
```

### Output

```text
unknown target: Locus::plan: unknown catalog 'absent'
no path: Locus::plan: no relationship path from 'orders' to 'island'
```

## Schema And Cost

Route search is breadth-first over the declared relationship graph and is
cached by the Locus relationship engine until that immutable workspace changes.
Appending the resolved steps copies plan metadata but does not read Catalog
values. Collection cost is still one full join per path edge; shortest hop count
does not necessarily mean least data volume. For a performance-sensitive path
where a different route has smaller intermediate results, spell the intended
sequence with `join()`.

Every hop carries streams from the accumulated left and the newly joined right,
subject to collision suffixing and key coalescing. Long paths can therefore
become wide even when a final `pick()` is narrow, because final projection does
not push into intermediate joins.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pick.html"><span class="label">Previous</span><span class="title">pick</span></a>
  <a class="next-link" href="../../locus_traverse_api_reference.html"><span class="label">Next</span><span class="title">LocusTraverse API Reference</span></a>
</div>
