<!-- pluss-hand-authored -->

# Locus::pull

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
LocusPull Locus::pull(std::string_view root) const
```

## What It Establishes

`pull()` starts a root-grain feature plan. The root decides what one output
record means and fixes the output record count; later `take()` calls lift plain
streams through root-preserving links, while reducers collapse related records
back to one value per root record.

The complete builder surface lives in the
[LocusPull API Reference](../../locus_pull_api_reference.md). This root method
only validates and binds the catalog. It does not require metadata for plain root
streams. Aggregating a related catalog is stricter: the root needs a complete,
unique primary key so records can be grouped without changing their grain.

Unlike `LocusTraverse`, `LocusPull` is a mutable plan. Its planning methods append
specifications and return the same builder by reference, making it suitable for a
named plan assembled across conditionals. `collect()` is the only terminal.

## Parameter

**root** (`std::string_view`)
: The exact key of the catalog whose records define the output grain. It is
  validated immediately.

## Returns

`LocusPull`
: An empty feature plan owning a cheap Locus handle and the root key.

## Examples

Example 1. Pull two streams that already live on the root catalog.

```cpp
const auto pulled = data.pull("orders").take({"order_id", "amount"}).collect();
print_keys(pulled);
print_numbers(pulled, "amount");
```

### Output

```text
keys=[order_id, amount] rows=4
amount=[50, 75, 20, 90]
```

Example 2. A named builder exposes its root and specification count without
executing, and an alias changes the output stream name rather than its values.

```cpp
auto pull_plan = data.pull("orders");
std::cout << "root=" << pull_plan.root() << " specs=" << pull_plan.specs().size() << "\n";
pull_plan.take("state", "order_state");
std::cout << "root=" << pull_plan.root() << " specs=" << pull_plan.specs().size() << "\n";
print_words(pull_plan.collect(), "order_state");
```

### Output

```text
root=orders specs=0
root=orders specs=1
order_state=[paid, paid, held, void]
```

Example 3. An unknown root fails before any specification is added.

```cpp
print_error("unknown", [&] { (void)data.pull("absent"); });
```

### Output

```text
unknown: Locus::pull: unknown catalog 'absent'
```

## Execution And Cost

The entry call is a key lookup plus cheap handle ownership. A pull allocates its
result only at `collect()`. Reuse one named builder when specifications are added
incrementally; construct separate builders when plans must remain independent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from.html"><span class="label">Previous</span><span class="title">from</span></a>
  <a class="next-link" href="query.html"><span class="label">Next</span><span class="title">query</span></a>
</div>

