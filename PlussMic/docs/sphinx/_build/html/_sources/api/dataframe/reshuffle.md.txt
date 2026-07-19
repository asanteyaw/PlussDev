<!-- pluss-hand-authored -->

# DataFrame::reshuffle

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::reshuffle(Axis axis = Axis::Case, std::optional<std::uint64_t> random_state = std::nullopt) const -> DataFrame
```

## What It Does

Randomly permutes either the case order (`Axis::Case`, the default) or the feature
order (`Axis::Feature`) of the frame, leaving the other axis and every value
untouched. [`shuffle(random_state)`](shuffle.md) is exactly `reshuffle(Axis::Case,
random_state)` — the dedicated, more discoverable name for the case-axis case, which
is by far the more common one (train/test splits, breaking up any accidental
ordering before sampling, and so on).

`random_state` seeds the underlying random engine (`std::mt19937_64`); passing the
same seed reproduces the same permutation on the same build, while omitting it draws
a fresh, non-reproducible permutation from `std::random_device` each call. Because
the exact permutation a given seed produces depends on the C++ standard library's
own `std::shuffle` implementation, this page verifies `reshuffle()`'s *effect* — same
rows or features, same values, different order, reproducible given a seed — rather
than asserting one specific row order for one specific seed.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

`Axis::Case` permutes row order, keeping the feature order and every row's own
values intact. `Axis::Feature` permutes feature (column) order instead, keeping row
order and every feature's own values intact.

**random_state** (`std::optional<std::uint64_t>`, default `std::nullopt`)

A seed for reproducible shuffling. Omit it (the default) for a non-deterministic
shuffle seeded from `std::random_device`.

## Returns

`DataFrame`
    A new frame with the requested axis permuted; the other axis and every
    underlying value are unchanged.

## Examples

Example 1. Reshuffle case order, then verify the result still contains exactly the
same rows.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
std::cout << orders;
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     120    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

```cpp
auto reshuffled = orders.reshuffle(Axis::Case, 42);
std::cout << reshuffled.ncases() << "\n";
std::cout << reshuffled.sort_values({"id"}).equals(orders) << "\n";
```

### Output

```text
3
true
```

`reshuffled` still has `3` cases, and re-sorting it back by `id` ascending reproduces
`orders` exactly (`equals()` returns `true`) — `reshuffle()` only permutes row order,
it never adds, drops, or changes a value. The exact row order `reshuffled` itself
prints in is intentionally not shown here, since it depends on the standard library's
`std::shuffle` implementation for the given seed rather than on anything guaranteed
by this API.

Example 2. Reshuffle feature order instead, and verify the feature set is unchanged.

```cpp
auto reordered_features = orders.reshuffle(Axis::Feature, 42);
std::cout << reordered_features.nfeatures() << "\n";
std::cout << reordered_features.reorder(orders.features()).equals(orders) << "\n";
```

### Output

```text
3
true
```

`reordered_features` still has all `3` original features, just in some permuted
order; reordering it back to `orders`'s original feature sequence via
[`reorder()`](reorder.md) reproduces `orders` exactly.

## Notes

- Passing the same `random_state` on the same build reproduces the same permutation;
  it is not guaranteed to reproduce the same permutation across different platforms,
  compilers, or standard library implementations, since `std::shuffle`'s exact
  algorithm is implementation-defined.
- `Axis::Case` and `Axis::Feature` are mutually exclusive per call — a single call
  only permutes one axis. Call `reshuffle()` twice (once per axis) to permute both.
- [`shuffle()`](shuffle.md) is the more discoverable, case-axis-only convenience for
  the common case; reach for `reshuffle(Axis::Feature, ...)` directly when you
  specifically need to permute column order instead.

## See Also

- [`DataFrame::shuffle`](shuffle.md): the case-axis-only convenience wrapper around
  this method.
- [`DataFrame::sample`](sample.md): draw a random subset of rows, rather than
  permuting the full set.
- [`DataFrame::reorder`](reorder.md): deterministic, explicit feature reordering,
  rather than a random permutation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="rfloordiv.html">
    <span class="label">Next</span>
    <span class="title">rfloordiv</span>
  </a>
</div>
