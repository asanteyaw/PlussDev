<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d6a2f54c05db -->

# Atlas::frames

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
const std::vector<DataFrame>& Atlas::frames() const
```

## What It Does

Returns every frame in the stack, in depth order, as a plain `std::vector<DataFrame>` —
the escape hatch out of `Atlas`'s own API when a task is easier to express as ordinary
container operations (looping, `std::accumulate`, passing straight to a function that
takes a `std::vector<DataFrame>`) than as an `Atlas` method. This is a borrowed
reference, not a copy; it stays valid as long as the `Atlas` that produced it is alive.

## Parameters

`None`

## Returns

`const std::vector<DataFrame>&`
    Every frame in the stack, in depth order. Empty for an atlas built from an empty
    frame vector.

## Examples

Example 1. Iterate the stack directly as a `std::vector<DataFrame>`.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Slate(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Slate(pluss::make_series({110.0, 90.0}))}});
auto atlas = Atlas({q1, q2});

for (const auto& frame : atlas.frames()) {
  std::cout << frame.ncases() << " cases\n";
}
```

### Output

```text
2 cases
2 cases
```

Example 2. Feed the stack into an ordinary standard-library algorithm — here, totaling
`sales` across every frame using `std::accumulate` over `frames()`, rather than reaching
for an `Atlas`-specific reduction method.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"sales", Slate(pluss::make_series({100.0, 120.0}))}});
auto q2 = DataFrame({{"sales", Slate(pluss::make_series({110.0, 90.0}))}});
auto atlas = Atlas({q1, q2});

double total = 0.0;
for (const auto& frame : atlas.frames()) {
  total += frame.features("sales").sum().to_dbl();
}
std::cout << total;
```

### Output

```text
420
```

`frames()`'s result is a real `std::vector<DataFrame>`, so anything that already works on
that type — `std::accumulate`, range-based `for`, passing it to a helper function —
works here without an `Atlas`-specific equivalent needing to exist.

## Notes

- Requires a defined `Atlas`; throws `"Atlas: undefined handle"` on a
  default-constructed `Atlas()`.
- Returns a reference, not a copy — copying out of it (`auto owned = atlas.frames();`) is
  cheap regardless, since `DataFrame` itself is a lightweight handle, not a deep-copying
  type.
- Every frame in the returned vector shares the same `ncases()`/`nfeatures()`/schema, by
  the strict-cube guarantee enforced at `Atlas` construction.

## See Also

- [`Atlas::frame`](frame.md) / [`Atlas::dloc`](dloc.md): retrieve one frame by position
  instead of the whole vector.
- [`Atlas::depth`](depth.md): the length of this vector.
- [`Atlas::aggregate_over_frames`](aggregate_over_frames.md): reduce this same vector to
  one `DataFrame` via a supplied function, without leaving the `Atlas` API.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="frame.html">
    <span class="label">Previous</span>
    <span class="title">frame</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
