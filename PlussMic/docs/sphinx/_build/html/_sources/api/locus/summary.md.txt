<!-- pluss-hand-authored -->

# Locus::summary

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::summary() const -> pluss::table::DataFrame
```

## What It Does

Calls `Nexus::summary` on this `Locus`'s aligned [`nexus()`](nexus.md)
view: the compact table inventory for the workspace, one row per catalog.
It's deliberately small -- what tables are here, and how large are they --
compared to the deeper diagnostics [`profile`](profile.md) builds.

## Parameters

None.

## Returns

A `DataFrame` with one row per catalog.

## Examples

Example 1. One row per catalog, for a three-table workspace.

```cpp
const auto linked = make_linked_shop();
std::cout << "summary ncases=" << linked.summary().ncases() << "\n";
```

### Output

```text
summary ncases=3
```

## See Also

- [`profile`](profile.md): the deeper, per-feature diagnostics version of
  this same inventory.
- [`schema`](schema.md): the structural, `TableMeta`-combined version.
