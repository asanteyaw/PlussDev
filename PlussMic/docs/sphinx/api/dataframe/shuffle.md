<!-- pluss-hand-authored -->

# DataFrame::shuffle

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::shuffle(std::optional<std::uint64_t> random_state = std::nullopt) const -> DataFrame
```

## What It Does

Randomly permutes row (case) order, leaving the feature order and every value
untouched. It is exactly [`reshuffle(Axis::Case, random_state)`](reshuffle.md) under
a more discoverable name — reach for `shuffle()` whenever "randomly reorder the
rows" is literally what you mean, which covers the large majority of cases
(breaking up incidental ordering before sampling, building a randomized train/test
split, and so on); reach for [`reshuffle()`](reshuffle.md) directly only when you
specifically need to permute the *feature* axis instead.

## Parameters

**random_state** (`std::optional<std::uint64_t>`, default `std::nullopt`)

A seed for reproducible shuffling. Omit it (the default) for a non-deterministic
shuffle seeded from `std::random_device`. See [`reshuffle()`](reshuffle.md)'s Notes
for why a given seed's exact resulting permutation is not portable across builds.

## Returns

`DataFrame`
    A new frame with the same feature set and the same per-row values, in a
    permuted row order.

## Examples

Example 1. Shuffle row order, then verify the same rows are still all present.

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
auto shuffled = orders.shuffle(42);
std::cout << shuffled.ncases() << "\n";
std::cout << shuffled.sort_values({"id"}).equals(orders) << "\n";
```

### Output

```text
3
true
```

`shuffled` still has all `3` original rows — re-sorting it back by `id` ascending
reproduces `orders` exactly. As with [`reshuffle()`](reshuffle.md), the specific row
order a given `random_state` produces depends on the standard library's own
`std::shuffle` implementation, so it isn't asserted here as a fixed, portable
sequence.

Example 2. The same `random_state` reproduces the same permutation within one build.

```cpp
auto first = orders.shuffle(7);
auto second = orders.shuffle(7);
std::cout << first.equals(second) << "\n";
```

### Output

```text
true
```

Two calls with the identical seed `7` produce the identical row order on the same
build — `shuffle()` is deterministic given a fixed seed, even though the exact
permutation itself is not part of this API's portable contract.

## Notes

- `shuffle()` never changes the feature order or any cell's value — only row
  position moves.
- Omitting `random_state` draws a fresh seed from `std::random_device` each call, so
  two unseeded calls essentially never produce the same order.
- `shuffle(random_state)` is identical to `reshuffle(Axis::Case, random_state)`;
  pick whichever name reads better at the call site.

## See Also

- [`DataFrame::reshuffle`](reshuffle.md): the general form, supporting the feature
  axis as well.
- [`DataFrame::sample`](sample.md): draw a random subset of rows rather than
  permuting the full set.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="skew.html">
    <span class="label">Next</span>
    <span class="title">skew</span>
  </a>
</div>
