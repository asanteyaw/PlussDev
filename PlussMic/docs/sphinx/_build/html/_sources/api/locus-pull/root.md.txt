<!-- pluss-hand-authored -->

# LocusPull::root

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull::root() const noexcept -> const std::string&
```

## What It Does

Reports the catalog that owns the pull's output grain -- the table name
passed to [`Locus::pull(root)`](../locus/pull.md). It does not execute
anything and never changes after construction; there is no way to retarget
an existing `LocusPull` at a different root.

## Parameters

None.

## Returns

A reference to the root table's name, valid for the `LocusPull`'s own
lifetime.

## Examples

Example 1. `root()` is set at construction and stays stable as specs are
added.

```cpp
auto root_plan = data.pull("customers");
std::cout << "root=" << root_plan.root() << " specs=" << root_plan.specs().size() << "\n";
root_plan.take("id").sum("events.spend", "spent");
std::cout << "root=" << root_plan.root() << " specs=" << root_plan.specs().size() << "\n";
```

### Output

```text
root=customers specs=0
root=customers specs=2
```

`root()` reads `"customers"` both before and after two specs are added --
adding fields changes `specs()`, never `root()`.

## See Also

- [`specs`](specs.md): the other read-only planning accessor.
- [`Locus::pull`](../locus/pull.md): where `root` is fixed.
