<!-- pluss-hand-authored -->

# Locus::profile

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
pluss::table::DataFrame Locus::profile() const
```

## What It Does

Calls `Nexus::profile` on this `Locus`'s aligned [`nexus()`](nexus.md)
view: a deeper, table-level diagnostics frame than [`summary`](summary.md)
-- where `summary()` answers "what tables are here and how large are
they?", `profile()` answers the next layer of per-table questions (feature
counts, null rates, and the rest `Nexus::profile` documents).

## Parameters

None.

## Returns

A `DataFrame` with one row per catalog.

## Examples

Example 1. One row per catalog, the same row count as `summary()`, with
more columns.

```cpp
const auto linked = make_linked_shop();
std::cout << "profile ncases=" << linked.profile().ncases() << "\n";
```

### Output

```text
profile ncases=3
```

## See Also

- [`summary`](summary.md): the smaller inventory this builds on.
- [`scan`](scan.md): table rows and relationship rows combined in one
  frame.
