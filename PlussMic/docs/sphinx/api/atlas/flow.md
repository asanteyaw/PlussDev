<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ab65deb80720 -->

# Atlas::flow

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::flow(const std::function<Atlas(const Atlas&)>& fn) const -> Atlas
```

## What It Does

Calls `fn` **exactly once**, passing the entire current atlas, and returns exactly what
`fn` returns — with no depth or shape validation at all, the `Atlas` analog of
[`Thread::flow`](../thread/flow.md). Reach for `flow()` when you want to drop into a
pipeline stage that changes depth arbitrarily (filtering, expanding, reducing to a single
frame's worth of depth) without a shape check getting in the way.

## Parameters

**fn** (`const std::function<Atlas(const Atlas&)>&`)
    Called exactly once with `*this`. Must return a defined `Atlas`; must not be empty.

## Returns

`Atlas`
    Exactly what `fn` returns.

## Examples

Example 1. Drop into a pipeline stage that reduces depth arbitrarily.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({4.0, 5.0, 6.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.flow([](const pluss::table::Atlas& a) { return a.head(1); });
std::cout << "depth: " << out.depth() << "\n";
std::cout << out.dloc(0);
```

### Output

```text
depth: 1
===========
     | id  
-----------
0    | 1   
1    | 2   
2    | 3   
-----------
[3 cs x 1 feat]
```

## Notes

- No shape validation — `fn` can return an `Atlas` of any depth, including one different
  from the receiver's.
- `fn` must not be empty; throws `"Atlas::flow: callback is empty"` otherwise.
- The single call point makes `flow()` a natural place to insert an arbitrary
  transformation into a larger method chain without breaking it into separate statements.

## See Also

- [`Atlas::map_frames`](map_frames.md): transform each frame independently instead of the
  whole atlas at once.
- [`Thread::flow`](../thread/flow.md): the single-column analog of this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="features.html">
    <span class="label">Previous</span>
    <span class="title">features</span>
  </a>
  <a class="next-link" href="frame.html">
    <span class="label">Next</span>
    <span class="title">frame</span>
  </a>
</div>
