<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6a6d35c1a9f6 -->

# DataFrame::unsqueeze

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::unsqueeze() const -> Atlas
DataFrame::unsqueeze(const Thread& depth_labels) const -> Atlas
```

## What It Does

Wraps the current frame into a depth-`1` `Atlas` — `pluss::table`'s stacked,
multi-frame container — so it can participate in APIs that expect an `Atlas` rather
than a lone `DataFrame`. `unsqueeze()` is literally `Atlas({*this})`; the
`depth_labels` overload is `Atlas({*this}, depth_labels)`, additionally naming that
one depth layer.

## Parameters

**depth_labels** (`const Thread&`, one-argument overload only)

A single label naming the frame's one depth layer. Must be defined and have exactly
`size() == 1`, or the call throws.

## Returns

`Atlas`
    A new atlas of `depth() == 1`, whose one frame (`.frame(0)`) is exactly this
    frame.

## Examples

Example 1. Lift a frame into a one-layer atlas with no depth label.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});

auto atlas = orders.unsqueeze();
std::cout << atlas.depth() << "\n";
std::cout << atlas.frame(0).equals(orders) << "\n";
std::cout << atlas.axis0_labels().has_value() << "\n";
```

### Output

```text
1
true
false
```

`atlas` has exactly one depth layer, and that layer (`.frame(0)`) is `orders`
itself — `equals()` confirms it matches exactly. No `depth_labels` were given, so
`axis0_labels()` reports no value.

Example 2. Attach a depth label to that one layer.

```cpp
auto labeled_atlas = orders.unsqueeze(Thread(pluss::make_note({"train"})));
std::cout << labeled_atlas.axis0_labels().has_value() << "\n";
```

### Output

```text
true
```

`labeled_atlas` carries one depth label, `"train"`, alongside its single frame —
useful when this frame is about to be combined with other unsqueezed frames into a
multi-depth atlas (say, one layer per dataset split) and each layer needs a name.

## Notes

- `unsqueeze()` never copies or transforms the frame's data — the atlas's one layer
  is exactly this frame, unchanged.
- The `depth_labels` overload requires exactly one label (`size() == 1`); passing a
  `Thread` of any other length throws `"DataFrame::unsqueeze: depth labels must have
  size 1"`.
- This is the natural entry point when an API expects an `Atlas` (for example,
  multi-frame plotting or archival) but the data in hand is currently a single
  `DataFrame`.

## See Also

- [`DataFrame::kindelize`](kindelize.md): a different "leave the `DataFrame`
  representation" conversion, into a `Catalog` of independent streams instead of an
  `Atlas`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unify.html">
    <span class="label">Previous</span>
    <span class="title">unify</span>
  </a>
  <a class="next-link" href="unstack.html">
    <span class="label">Next</span>
    <span class="title">unstack</span>
  </a>
</div>
