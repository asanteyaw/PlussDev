<!-- pluss-hand-authored -->

# NexusPull::root

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
const std::string& NexusPull::root() const noexcept
```

## What It Does

Reports the anchor table this builder was created against — the same
`root` string passed to [`Nexus::pull(root)`](../nexus/pull.md) when the
builder was constructed. Every field the builder pulls, however many hops
away it lives, comes back at this table's row grain.

## Parameters

None.

## Returns

`const std::string&`
    The root table's name, unchanged for the builder's whole lifetime —
    there is no way to re-anchor an existing `NexusPull` to a different
    root; start a new one via `nx.pull(other_root)` instead.

## Examples

Example 1. Inspect the anchor before deciding what to pull.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

// nx: the customers/regions/events workspace from Nexus::pull's own page.
auto builder = nx.pull("customers").take("customer_id");
std::cout << builder.root();
```

### Output

```text
customers
```

## Notes

- `root()` is unaffected by which fields have been requested so far — it
  reflects only how the builder was constructed, not its accumulated
  [`.specs()`](specs.md).
- Useful mainly in generic code that builds a `NexusPull` from elsewhere and
  needs to know, without re-threading the original table name, what row
  grain the eventual [`.collect()`](collect.md) result will have.

## See Also

- [`NexusPull::specs`](specs.md): inspect the accumulated field requests
  themselves.
- [`Nexus::pull`](../nexus/pull.md): where `root` comes from, and what "row
  grain" means for a pull.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="specs.html">
    <span class="label">Next</span>
    <span class="title">specs</span>
  </a>
</div>
