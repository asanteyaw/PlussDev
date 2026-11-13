<!-- pluss-hand-authored -->

# Nexus::relationship_audit

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
DataFrame Nexus::relationship_audit(std::string_view name) const
```

## What It Does

Runs the full [`validate_relationship()`](validate_relationship.md) check and returns
every intermediate count as a one-row `DataFrame`, instead of just the final `bool`. This
is the relationship-level counterpart to [`grain_audit()`](grain_audit.md).

## Parameters

**name** (`std::string_view`)
    The relationship to audit, matching [`Relationship::name`](relationships.md). Must
    already be declared — an unknown name throws
    `"Nexus::relationship_audit: unknown relationship '<name>'"`.

## Returns

`DataFrame`
    A single row with columns:

    - `relationship`, `left_table`, `right_table`, `cardinality` — echoed from the
      declaration.
    - `declared_status` — the `Relationship::status` field as declared (`"declared"`,
      `"validated"`, `"warning"`, or `"invalid"`); this is descriptive metadata, not
      recomputed here.
    - `computed_status` — what this audit actually finds: `"validated"` if fully valid
      with no gaps, `"warning"` if valid but with some missing/orphaned rows tolerated by
      `left_optional`/`right_optional`, `"invalid"` otherwise.
    - `left_rows`/`right_rows` — total row counts on each side.
    - `left_complete_rows`/`right_complete_rows` — rows with a non-missing key value on
      that side.
    - `left_missing_keys`/`right_missing_keys` — the complement (rows with a missing key
      value).
    - `left_duplicate_rows`/`right_duplicate_rows` — rows involved in a duplicate key
      combination on that side.
    - `left_orphans`/`right_orphans` — unmatched-row counts, same as
      [`detect_orphans()`](detect_orphans.md)'s row counts for each side.
    - `left_unique_required`/`right_unique_required` — whether `cardinality` requires that
      side to be unique.
    - `left_unique_valid`/`right_unique_valid` — whether that side actually is unique,
      when required (`true` automatically when uniqueness isn't required on that side).
    - `left_optional`/`right_optional` — echoed from the declaration.
    - `valid` — the same overall verdict [`validate_relationship()`](validate_relationship.md)
      returns.

## Examples

Example 1. Auditing a relationship with one orphaned row on the left.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben"}))},
});
auto orders = DataFrame({
    {"order_id",    Slate(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 4.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

Relationship rel;
rel.name = "orders_to_customers";
rel.left_table = "orders";
rel.right_table = "customers";
rel.keys = {{"customer_id", "customer_id"}};
rel.cardinality = RelationshipCardinality::ManyToOne;

auto linked = biz.add_relationship(rel);

auto audit = linked.relationship_audit("orders_to_customers");
std::cout << audit.features({"relationship", "cardinality", "left_orphans", "right_orphans", "left_unique_valid", "valid"});
```

### Output

```text
==============================================================================================
     | relationship         cardinality  left_orphans  right_orphans  left_unique_valid  valid
----------------------------------------------------------------------------------------------
0    | orders_to_customers  many_to_one  1             0              true               false
----------------------------------------------------------------------------------------------
[1 cs x 6 feat]
```

One left-side orphan (order `103`, referencing a nonexistent customer `4.0`).
`right_orphans` is `0` — both customers `1` and `2` are referenced by at least one order.
`left_unique_valid` is `true` because `ManyToOne` doesn't require the left (`orders`) side
to be unique, so there's nothing to fail there; the overall `valid` is still `false`
because of the one left-side orphan.

## Notes

- `declared_status` is descriptive (whatever `Relationship::status` was set to); it's
  `computed_status` that reflects this audit's actual findings.
- `left_unique_valid`/`right_unique_valid` are `true` automatically on a side where
  uniqueness isn't required by `cardinality`.
- [`audit()`](audit.md) is an exact alias.
- Results are cached internally per relationship name, same as
  [`validate_relationship()`](validate_relationship.md).

## See Also

- [`Nexus::validate_relationship`](validate_relationship.md): the same check, collapsed to
  a `bool`.
- [`Nexus::detect_orphans`](detect_orphans.md): the actual unmatched rows behind the
  `left_orphans`/`right_orphans` counts.
- [`Nexus::grain_audit`](grain_audit.md): the table-level counterpart to this
  relationship-level audit.
- [`Nexus::audit(name)`](audit.md): the shorter relationship-level spelling; note that
  zero-argument [`audit()`](audit.md) now returns the workspace-wide audit table instead.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="relationship.html">
    <span class="label">Previous</span>
    <span class="title">relationship</span>
  </a>
  <a class="next-link" href="relationships.html">
    <span class="label">Next</span>
    <span class="title">relationships</span>
  </a>
</div>
