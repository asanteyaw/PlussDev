<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 42cf8b148dcf -->

# Atlas::relabel_depth

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::relabel_depth(const Thread& labels) const -> Atlas
```

## What It Does

Returns a new atlas with `labels` replacing the depth-label axis entirely — `labels` must
be the same length as the atlas's own depth, or the call throws before any copy is made.
Unlike [`sort_depth()`](sort_depth.md), this doesn't reorder frames; it only replaces the
label values attached to each existing depth position.

## Parameters

**labels** (`const Thread&`)
    The new depth-label axis. Must have the same length as the atlas's
    [`depth()`](depth.md).

## Returns

`Atlas`
    A new atlas, same depth and frame order, with `labels` as its new depth-label axis.

## Examples

Example 1. Attach fresh depth labels to a previously unlabeled atlas.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.relabel_depth(pluss::table::Thread(pluss::make_note({"jan", "feb"})));
std::cout << "has label: " << out.label().has() << "\n";
std::cout << out.label().values();
```

### Output

```text
has label: 1
0  jan
1  feb
ptype: Note
```

Example 2. A length mismatch throws immediately, before the receiver is touched.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

try {
  atlas.relabel_depth(pluss::table::Thread(pluss::make_note({"only_one"})));
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Atlas: axis0 labels length must equal depth
```

## Notes

- Requires an exact length match with the atlas's depth — a mismatch throws
  `"Atlas: axis0 labels length must equal depth"`.
- Replaces labels only; frame order and content are unaffected. Use
  [`sort_depth()`](sort_depth.md)/[`align_depth()`](align_depth.md) to also reorder frames.

## See Also

- [`Atlas::sort_depth`](sort_depth.md): reorder frames by the existing labels, instead of
  replacing them.
- [`Atlas::align_depth`](align_depth.md): reorder/filter to an explicit target label order.
- [`Atlas::label`](label.md): the fuller accessor for inspecting, setting, and manipulating
  depth labels.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="plot.html">
    <span class="label">Previous</span>
    <span class="title">plot</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
