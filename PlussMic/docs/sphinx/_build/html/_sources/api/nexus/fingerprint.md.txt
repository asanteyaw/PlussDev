<!-- pluss-hand-authored -->

# Nexus::fingerprint

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::fingerprint() const -> std::string
```

## What It Does

Computes a deterministic hash summarizing the whole workspace's content — every table's
name, shape, feature names, dtypes, and declared [`TableMeta`](table_meta.md); every
declared [`Relationship`](relationships.md); and every saved view's
[`JoinPlan`](plan.md) — as a 16-character lowercase hex string (a 64-bit FNV-1a hash).
Two workspaces with identical content produce identical fingerprints; any difference
anywhere in that content changes the result.

```{note} A content fingerprint, not a cryptographic one
This hashes structural and declared-metadata content (shapes, names, dtypes, `TableMeta`,
relationships, saved views) — it is **not** a hash of the tables' actual cell values, and
it is not collision-resistant against adversarial input the way a cryptographic hash would
be. Use it to detect accidental drift between two workspace snapshots, not as a security
primitive.
```

## Parameters

`None`

## Returns

`std::string`
    A 16-character lowercase hex digest.

## Examples

Example 1. The same workspace fingerprints identically every time; changing it changes the
result.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto biz = Nexus({{"customers", customers}});

auto fp1 = biz.fingerprint();
auto fp2 = biz.fingerprint();
std::cout << (fp1 == fp2) << " " << fp1.size() << "\n";

auto changed = biz.inject("extra", DataFrame({{"x", Thread(pluss::make_series({1.0}))}}));
std::cout << (fp1 == changed.fingerprint());
```

### Output

```text
1 16
0
```

## Notes

- Deterministic given the same content — does not depend on insertion order artifacts
  beyond what's already captured in `keys()`' own order, and sorts saved-view names
  internally before hashing them so view-declaration order doesn't affect the result.
- Not a hash of the underlying cell data — see the note above.
- Embedded (truncated) into each row of [`schema()`](schema.md)'s own `fingerprint` column.

## See Also

- [`Nexus::schema`](schema.md): includes a per-table fingerprint derived the same way.
- [`Nexus::save`](save.md): persists the workspace whose content this method summarizes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="flow.html">
    <span class="label">Next</span>
    <span class="title">flow</span>
  </a>
</div>
