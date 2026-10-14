<!-- pluss-hand-authored -->

# Locus::from

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
LocusTraverse Locus::from(std::string_view table) const
```

## What It Establishes

`from()` binds one existing catalog as the anchor of an immutable relational
plan. It validates the anchor immediately, then returns a `LocusTraverse` without
joining, projecting, or aligning data. This separation matters when plans are
built conditionally: all calls before `collect()` manipulate plan metadata, while
the terminal performs catalog alignment and relationship matching once.

The complete continuation surface lives in the
[LocusTraverse API Reference](../../locus_traverse_api_reference.md). Use its
`join()` method when the next hop is known, `via()` when the relationship graph
should find a shortest route, `pick()` or `drop()` for the final stream set, and
`collect()` to execute.

The returned builder owns a cheap Locus handle. It is therefore valid after the
source expression or local variable goes out of scope; it does not retain a raw
reference to the caller.

## Parameter

**table** (`std::string_view`)
: The exact key of the anchor catalog. The key must already exist. Empty and
  unknown names fail before a builder is returned.

## Returns

`LocusTraverse`
: A zero-hop plan anchored at `table`. Calling `collect()` immediately returns
  that catalog through the normal materialization path.

## Examples

Example 1. A zero-hop traversal materializes the anchor unchanged.

```cpp
const auto traversed = data.from("orders").collect();
print_keys(traversed);
print_numbers(traversed, "order_id");
```

### Output

```text
keys=[order_id, customer_id, amount, state] rows=4
order_id=[100, 101, 102, 103]
```

Example 2. The builder owns its workspace, so it may be retained from a temporary
Locus and refined later.

```cpp
const auto retained_walk = shop().from("orders");
print_words(retained_walk.pick({"state"}).collect(), "state");
```

### Output

```text
state=[paid, paid, held, void]
```

Example 3. Anchor validation is eager.

```cpp
print_error("unknown", [&] { (void)data.from("absent"); });
```

### Output

```text
unknown: Locus::from: unknown catalog 'absent'
```

## Execution And Cost

`from()` performs a key lookup and copies a reference-counted Locus handle. Its
cost does not depend on record count. Alignment, path execution, and output
allocation belong to `LocusTraverse::collect()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../locus_api_reference.html"><span class="label">Previous</span><span class="title">Locus API Reference</span></a>
  <a class="next-link" href="pull.html"><span class="label">Next</span><span class="title">pull</span></a>
</div>

