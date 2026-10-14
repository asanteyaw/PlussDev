<!-- pluss-hand-authored -->

# LocusTraverse::pick

[Back to LocusTraverse API Reference](../../locus_traverse_api_reference)

## Signatures

```cpp
LocusTraverse LocusTraverse::pick(const std::vector<std::string>& features) const
LocusTraverse LocusTraverse::pick(std::initializer_list<std::string_view> features) const
```

## Selecting The Final Schema

`pick()` records the exact streams and stream order wanted from the final
materialized traversal. It is a positive projection: streams not named are
excluded. Use [`drop()`](drop.md) when nearly every stream is wanted and naming
the exclusions is clearer.

The initializer-list overload is intended for fixed names at the call site. The
vector overload accepts names discovered at runtime. Both copy their names into
the returned builder, so a temporary vector or temporary strings are safe.

Selection occurs after all joins. A name may therefore refer to a stream added
by a later `join()` or `via()`, and it may need the right-side suffix chosen for
a collision. This also means `pick()` is not a request to avoid materializing
unselected join streams.

`pick()` replaces an earlier pick list. It does not append or intersect with it.
When `drop()` is also set, the picked schema is formed first and the drop list is
then applied. The explicit order of `features` becomes output stream order.

## Parameters

**features**
: Ordered final stream names. Each name must be non-empty and may appear only
  once in a call. An empty list is valid and produces a defined, empty Catalog
  when collected.

## Returns

`LocusTraverse`
: A new builder with its pick list replaced. The source builder and captured
  Locus are unchanged; no stream values are read.

## Examples

All outputs are emitted by
`PlussMic/examples/bundle/locus_traverse_reference_demo.cpp`.

Example 1. The literal overload can select streams from both sides of a join.
The requested order becomes Catalog key order.

```cpp
print_keys(data.from("orders")
    .join("customers")
    .pick({"name", "order_id"})
    .collect());
```

### Output

```text
keys=[name, order_id] rows=4
```

Example 2. The vector overload accepts a runtime-owned list.

```cpp
const std::vector<std::string> names{"amount", "state"};
print_keys(data.from("orders").pick(names).collect());
```

### Output

```text
keys=[amount, state] rows=5
```

Example 3. A second pick replaces the first list and establishes a new order.

```cpp
print_keys(data.from("orders")
    .pick({"order_id"})
    .pick({"state", "amount"})
    .collect());
```

### Output

```text
keys=[state, amount] rows=5
```

Example 4. `drop()` runs after `pick()`.

```cpp
print_keys(data.from("orders")
    .pick({"order_id", "state"})
    .drop({"state"})
    .collect());
```

### Output

```text
keys=[order_id] rows=5
```

Example 5. Empty selection is defined and explicit. With no streams, the result
has no stream from which to retain a record extent, so its row count is zero.

```cpp
print_keys(data.from("orders").pick({}).collect());
```

### Output

```text
keys=[] rows=0
```

Example 6. Structural name errors are immediate. Existence is deferred because
the name may be introduced by a hop appended after `pick()`.

```cpp
print_error("duplicate", [&] {
  (void)data.from("orders").pick({"order_id", "order_id"});
});
print_error("empty name", [&] {
  (void)data.from("orders").pick({""});
});
const auto lazy = data.from("orders").pick({"absent"});
std::cout << "unknown pick built\n";
print_error("unknown pick collect", [&] { (void)lazy.collect(); });
```

### Output

```text
duplicate: LocusTraverse::pick: duplicate feature 'order_id'
empty name: LocusTraverse::pick: feature names cannot be empty
unknown pick built
unknown pick collect: Catalog::keys: unknown key 'absent'
```

## Shape, Ownership, And Cost

Picking does not alter any retained stream's kind, values, validity, length, or
ragged position. The output Catalog may remain ragged. The method itself copies
only names and builder metadata. At collection, Catalog projection builds a new
schema whose stream handles refer to the selected payloads. All joins still run
before projection, so selecting fewer streams primarily reduces the returned
schema, not relationship matching cost.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="options.html"><span class="label">Previous</span><span class="title">options</span></a>
  <a class="next-link" href="via.html"><span class="label">Next</span><span class="title">via</span></a>
</div>
