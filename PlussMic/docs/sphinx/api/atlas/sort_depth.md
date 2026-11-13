<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d98a09cd6cd3 -->

# Atlas::sort_depth

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas Atlas::sort_depth(bool ascending = true, bool nulls_last = true) const
```

## What It Does

Returns a new atlas with its frames reordered by depth label — sorting the label axis
reorders the underlying frames along with it, not just the labels in isolation. The `Atlas`
analog of [`Archive::label`](../archive/label.md)`.sort()`.

## Parameters

**ascending** (`bool`, defaults to `true`)
    Sort direction for the depth labels.

**nulls_last** (`bool`, defaults to `true`)
    Whether `NULL` depth labels sort to the end (`true`) or the start (`false`).

## Returns

`Atlas`
    A new atlas, same depth, with frames reordered by depth label.

## Examples

Example 1. Sort ascending: the frame originally at depth-index 1 (labeled `"a"`) moves to
depth-index 0.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({100.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({200.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Slate(pluss::make_note({"b", "a"})));

auto sorted = atlas.sort_depth();
std::cout << "sorted labels:\n" << sorted.label().values();
std::cout << "dloc(0):\n" << sorted.dloc(0);
```

### Output

```text
sorted labels:
0  a
1  b
ptype: Note
dloc(0):
===========
     | id  
-----------
0    | 200 
-----------
[1 cs x 1 feat]
```

## Notes

- Reorders the underlying frames together with the label stream — this is a full
  depth-axis sort, not a label-only sort.
- Requires depth labels to be set; behavior on an unlabeled atlas mirrors
  [`Archive::label`](../archive/label.md)'s own requirement.

## See Also

- [`Atlas::relabel_depth`](relabel_depth.md): replace the depth labels entirely, instead of
  reordering by the existing ones.
- [`Atlas::align_depth`](align_depth.md): reorder/filter to an explicit target label order.
- [`Archive::label`](../archive/label.md)`.sort()`: the depth-stacked-catalog analog of
  this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice_frames.html">
    <span class="label">Previous</span>
    <span class="title">slice_frames</span>
  </a>
  <a class="next-link" href="squeeze.html">
    <span class="label">Next</span>
    <span class="title">squeeze</span>
  </a>
</div>
