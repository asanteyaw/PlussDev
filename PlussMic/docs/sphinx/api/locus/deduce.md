<!-- pluss-hand-authored -->

# Locus::deduce

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
pluss::table::DataFrame Locus::deduce(double min_confidence = 0.5) const
```

## What It Does

Calls `Nexus::deduce` on this `Locus`'s aligned [`nexus()`](nexus.md) view:
scans the workspace for plausible relationship candidates -- column-name
and value overlaps that look like an undeclared link -- and returns them as
an advisory `DataFrame`. It never creates a relationship, mutates the
workspace, or validates that a candidate is actually correct; it's a
suggestion to review, not a declaration to [`link`](link.md) automatically.

## Parameters

| Parameter | Meaning |
|---|---|
| `min_confidence` | The minimum confidence score a candidate must reach to be reported. Default `0.5`. |

## Returns

A `DataFrame` with one row per candidate relationship found, possibly empty.

## Examples

Example 1. A workspace whose real relationships are already declared has
nothing left to suggest.

```cpp
const auto linked = make_linked_shop();
std::cout << "deduce ncases=" << linked.deduce().ncases() << "\n";
```

### Output

```text
deduce ncases=0
```

## See Also

- [`link`](link.md): declaring a relationship `deduce()` only suggests.
- [`audit_report`](audit_report.md): checking relationships that are
  already declared, rather than finding undeclared ones.
