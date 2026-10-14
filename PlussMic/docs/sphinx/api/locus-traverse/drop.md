<!-- pluss-hand-authored -->

# LocusTraverse::drop

[Back to LocusTraverse API Reference](../../locus_traverse_api_reference)

## Signatures

```cpp
LocusTraverse LocusTraverse::drop(const std::vector<std::string>& features) const
LocusTraverse LocusTraverse::drop(std::initializer_list<std::string_view> features) const
```

## Excluding Final Streams

`drop()` records streams to remove after the traversal has been materialized.
It is useful when most of an accumulated schema is wanted and the exclusions
are easier to name than the retained streams. For a small positive selection,
[`pick()`](pick.md) states the intent more directly.

The initializer-list overload is concise for literal names. The vector overload
accepts names assembled at runtime and copies them into the returned builder;
the caller's strings need not outlive the call. Neither overload mutates the
current builder or performs a join.

`drop()` is a replacement setting, not an accumulating command. A second call
replaces the first exclusion list. This keeps an immutable branch independently
configurable, but means `.drop({"a"}).drop({"b"})` removes only `b`. Put both
names in one call when both must be removed.

When `pick()` is also present, collection first selects and orders the picked
streams and then removes the dropped names. A dropped name must therefore exist
in the post-pick schema, not merely somewhere in the joined Catalog.

## Parameters

**features**
: Stream names in the final materialized schema. Names must be non-empty and
  unique within the call. An empty list is valid and makes `drop()` a no-op.

## Returns

`LocusTraverse`
: A new builder carrying the same Locus, join plan, anchor, and options, with
  its drop list replaced. Stream values are not read until `collect()`.

## Examples

All outputs are emitted by the permanent Locus traversal reference executable.

Example 1. Literal names use the initializer-list overload.

```cpp
print_keys(data.from("orders").drop({"state", "amount"}).collect());
```

### Output

```text
keys=[order_id, customer_id] rows=5
```

Example 2. Runtime-owned names use the vector overload.

```cpp
const std::vector<std::string> names{"customer_id", "state"};
print_keys(data.from("orders").drop(names).collect());
```

### Output

```text
keys=[order_id, amount] rows=5
```

Example 3. A later `drop()` replaces the earlier list; it does not union with it.

```cpp
print_keys(data.from("orders")
    .drop({"state"})
    .drop({"amount"})
    .collect());
```

### Output

```text
keys=[order_id, customer_id, state] rows=5
```

Example 4. With both projection methods, `drop()` operates on the picked schema.

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

Example 5. An empty list leaves the final schema unchanged.

```cpp
print_keys(data.from("orders").drop({}).collect());
```

### Output

```text
keys=[order_id, customer_id, amount, state] rows=5
```

Example 6. Duplicate and empty names are rejected immediately, while an unknown
but well-formed name is deferred until the final schema exists.

```cpp
print_error("duplicate", [&] {
  (void)data.from("orders").drop({"state", "state"});
});
print_error("empty name", [&] {
  (void)data.from("orders").drop({""});
});
const auto lazy = data.from("orders").drop({"absent"});
std::cout << "unknown drop built\n";
print_error("unknown drop collect", [&] { (void)lazy.collect(); });
```

### Output

```text
duplicate: LocusTraverse::drop: duplicate feature 'state'
empty name: LocusTraverse::drop: feature names cannot be empty
unknown drop built
unknown drop collect: Catalog::drop: unknown key 'absent'
```

## Ordering, Raggedness, And Cost

Surviving streams retain their pre-drop order and their own lengths, validity,
kind, and metadata. `drop()` neither aligns nor copies stream payloads while the
plan is being built. At collection it constructs a new Catalog schema from the
survivors. It is not projection pushdown: all requested joins execute before the
exclusion is applied.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="collect.html"><span class="label">Previous</span><span class="title">collect</span></a>
  <a class="next-link" href="join.html"><span class="label">Next</span><span class="title">join</span></a>
</div>
