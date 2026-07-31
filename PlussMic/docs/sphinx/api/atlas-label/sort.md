<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: aa227b7d86b8 -->

# AtlasLabelAccessor::sort

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::sort(bool ascending = true, bool nulls_last = true) const -> Atlas
```

## What It Does

Returns a copy of the bound atlas with its frames reordered by depth label — the
accessor form of [`Atlas::sort_depth`](../atlas/sort_depth.md), which it delegates to
internally. Reordering the labels also reorders the frames themselves, so
[`dloc(i)`](../atlas/dloc.md) after a sort returns whichever frame now sits at position
`i`.

## Parameters

**ascending** (`bool`, default `true`)
    Sort direction: `true` for ascending order, `false` for descending.

**nulls_last** (`bool`, default `true`)
    Where null depth labels land: `true` places them at the end regardless of
    `ascending`, `false` places them at the start.

## Returns

`Atlas`
    A new atlas with frames reordered to match the sorted depth labels.

## Examples

Example 1. Sort an out-of-order-labeled atlas back into ascending order; the frame
content moves along with its label.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 90.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2}, pluss::table::Thread(pluss::make_note({"mar", "jan"})));

auto sorted = atlas.label().sort();
std::cout << sorted.label().values() << "\n\n";
std::cout << "dloc(0) after sort:\n" << sorted.dloc(0);
```

### Output

```text
0  jan
1  mar
ptype: Note

dloc(0) after sort:
============
     | sales
------------
0    | 110  
1    | 90   
------------
[2 cs x 1 feat]
```

`q2` — originally at depth-index 1, labeled `"jan"` — is now at depth-index 0, since
`"jan"` sorts before `"mar"`.

## Notes

- Reorders the frames themselves, not just the label values — see the example above.
- The original atlas is left unchanged — `sort()` returns a new `Atlas`.
- Directly delegates to [`Atlas::sort_depth`](../atlas/sort_depth.md); see that page for
  the same `ascending`/`nulls_last` semantics applied to an atlas without going through
  the accessor.

## See Also

- [`Atlas::sort_depth`](../atlas/sort_depth.md): the underlying `Atlas` method this
  accessor wraps.
- [`AtlasLabelAccessor::align`](align.md): reorder (and optionally subset) frames to
  match a specific target label order, rather than a sorted one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="symdiff.html">
    <span class="label">Next</span>
    <span class="title">symdiff</span>
  </a>
</div>
