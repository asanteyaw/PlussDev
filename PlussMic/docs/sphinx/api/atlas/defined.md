<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3685d9f2e68b -->

# Atlas::defined

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
bool Atlas::defined() const noexcept
```

## What It Does

Reports whether the `Atlas` owns a real payload — a non-empty internal handle created by
the constructor that takes a `std::vector<DataFrame>` — as opposed to a default-constructed
`Atlas()`, which owns nothing. Every other `Atlas` method that reads state (`depth()`,
`ncases()`, `frames()`, and so on) calls this check internally and throws
`"Atlas: undefined handle"` if it fails, so `defined()` is the one call that's always safe
to make on any `Atlas`, defined or not, and the one to reach for before working with an
`Atlas` that might have come from a default-constructed variable or a moved-from value.

## Parameters

`None`

## Returns

`bool`
    `true` if the `Atlas` was constructed with frame data (even zero frames, via an empty
    vector); `false` for a default-constructed `Atlas()` or a moved-from `Atlas`.

## Examples

Example 1. A default-constructed `Atlas` is not defined.

```cpp
using namespace pluss::table;

Atlas empty_atlas;
std::cout << empty_atlas.defined();
```

### Output

```text
0
```

Example 2. An `Atlas` built from frames is defined, even once every frame is later
removed by slicing.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"sales", Slate(pluss::make_series({100.0, 120.0}))},
});
auto q2 = DataFrame({
    {"sales", Slate(pluss::make_series({110.0, 90.0}))},
});
auto atlas = Atlas({q1, q2});
std::cout << "defined: " << atlas.defined() << "\n";

auto sliced_away = atlas.slice_frames(0, 0);
std::cout << "defined after emptying: " << sliced_away.defined() << "\n";
std::cout << "depth after emptying: " << sliced_away.depth();
```

### Output

```text
defined: 1
defined after emptying: 1
depth after emptying: 0
```

`sliced_away` still owns a real (if zero-frame) internal handle — `slice_frames()` always
returns a constructed `Atlas`, never a default one — so `defined()` stays `true` even
though `depth()` is `0`. This mirrors [`empty()`](empty.md), which is the check that
actually reports "zero frames"; `defined()` only ever answers "does this handle have a
payload at all."

## Notes

- `defined()` never throws, unlike almost every other `Atlas` method — it is
  `noexcept` specifically so it can be called safely on any `Atlas`, including one that
  failed to construct or was moved from.
- A defined `Atlas` can still have zero frames (`depth() == 0`); see [`empty()`](empty.md)
  for that check.

## See Also

- [`Atlas::empty`](empty.md): whether the atlas has zero frames — a distinct question from
  whether it has a payload at all.
- [`Atlas::unique`](unique.md): whether this `Atlas` is the sole owner of its underlying
  data, a separate concern from whether it is defined.
- [`Atlas::depth`](depth.md): the frame count, only safe to call once `defined()` is `true`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="column_names.html">
    <span class="label">Previous</span>
    <span class="title">column_names</span>
  </a>
  <a class="next-link" href="depth.html">
    <span class="label">Next</span>
    <span class="title">depth</span>
  </a>
</div>
