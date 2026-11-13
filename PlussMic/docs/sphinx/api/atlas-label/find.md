<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 785b3b234aa7 -->

# AtlasLabelAccessor::find

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
Slate AtlasLabelAccessor::find(const Slate& targets) const
```

## What It Does

Looks up the position of each value in `targets` within the atlas's current depth
labels, returning one integer position per target, in the same order as `targets`.

```{important} A target with no match returns -1 — it does not throw
Unlike [`align()`](align.md), which throws on an unmatched target label,
`find()` reports a missing label as position `-1` in the result rather than raising.
Check for `-1` in the returned `Slate` if you need to detect misses.
```

## Parameters

**targets** (`const Slate&`)
    The depth-label values to look up.

## Returns

`Slate`
    One integer position per value in `targets`, named `"find"`, indexed by the target
    values themselves. A target with no match yields `-1`.

## Examples

Example 1. Look up the positions of two known depth labels.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({110.0, 90.0}))},
});
auto q3 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({95.0, 130.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2, q3}, pluss::table::Slate(pluss::make_note({"jan", "feb", "mar"})));

std::cout << atlas.label().find(pluss::table::Slate(pluss::make_note({"mar", "jan"})));
```

### Output

```text
mar  2
jan  0
Name: find, ptype: Series
```

Example 2. A target that isn't one of the atlas's depth labels resolves to `-1`, no
throw.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({110.0, 90.0}))},
});
auto q3 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({95.0, 130.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2, q3}, pluss::table::Slate(pluss::make_note({"jan", "feb", "mar"})));

std::cout << atlas.label().find(pluss::table::Slate(pluss::make_note({"nope"})));
```

### Output

```text
nope  -1
Name: find, ptype: Series
```

## Notes

- See the important callout above: unmatched targets yield `-1`, they do not throw
  (a real difference from [`align()`](align.md)).
- The result's index is the target values themselves, not `0, 1, 2, ...` — see both
  examples.

## See Also

- [`AtlasLabelAccessor::pos`](pos.md): look up a single label's position directly,
  without wrapping it in a `Slate` first.
- [`AtlasLabelAccessor::align`](align.md): reorder/subset the bound atlas's frames to a
  target label order — throws on a miss instead of returning `-1`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html">
    <span class="label">Previous</span>
    <span class="title">diff</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
