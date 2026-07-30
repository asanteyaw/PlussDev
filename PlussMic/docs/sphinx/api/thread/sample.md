<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 189e4dae0f26 -->

# Thread::sample

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::sample(std::int64_t n) const -> Thread
Thread::sample(std::int64_t n, std::uint64_t random_state, bool replace = false) const -> Thread

template <class Float> requires std::floating_point<Float>
Thread::sample(Float frac) const -> Thread
template <class Float> requires std::floating_point<Float>
Thread::sample(Float frac, std::uint64_t random_state, bool replace = false) const -> Thread
```

## What It Does

Draws a random subset of entries. There are two ways to size the draw — a count
(`n`) or a fraction of `size()` (`frac`, rounded to the nearest integer count) —
and each has a plain form (a fresh, non-reproducible random seed every call) and an
explicit `random_state` form (reproducible on a given build — see the important
callout below).

- **Without `replace`** (the default): each position can be drawn at most once.
  Requesting more than `size()` doesn't throw — the draw silently caps at
  `size()`, so you get every entry back (in shuffled order), never a repeat.
- **With `replace = true`**: positions can repeat, and `n`/`frac` may exceed
  `size()` freely — `frac > 1.0` is only accepted when `replace = true`, otherwise
  it throws.

```{important} random_state reproduces results on a given build, not across toolchains
`sample()`'s underlying randomness comes from `std::mt19937_64` combined with
`std::shuffle` (without replacement) or `std::uniform_int_distribution` (with
replacement). The C++ standard fixes the *statistical properties* of these
facilities but not their exact bit-for-bit algorithm, so the same `random_state`
reliably reproduces the same draw *within one build* (same compiler, same standard
library), but is not guaranteed to reproduce the identical draw across different
compilers or standard library implementations. The example outputs below reflect
one particular build and are illustrative of the *shape* of the result, not a
cross-platform guarantee of the exact indices drawn.
```

## Parameters

**n** (`std::int64_t`)
    Number of entries to draw. Must be `>= 0`. Without `replace`, capped silently
    at `size()`.

**frac** (floating-point)
    Fraction of `size()` to draw, rounded to the nearest integer count. Must be
    finite and `>= 0`. A value `> 1.0` requires `replace = true`.

**random_state** (`std::uint64_t`, optional)
    Seed for the random number generator. Omitted (or the no-`random_state`
    overload) draws a fresh, non-reproducible seed each call via
    `std::random_device`.

**replace** (`bool`, default `false`)
    Whether a position can be drawn more than once.

## Returns

`Thread`
    A new thread holding the drawn entries, length `n` (or the rounded `frac *
    size()`, capped as described above when `replace = false`).

## Examples

Example 1. Draw 2 entries without replacement, with a fixed seed for
reproducibility on this build.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto out = scores.sample(2, 42, false);
std::cout << out;
```

### Output (illustrative — see the important callout above)

```text
0  40
1  10

ptype: Series
```

Every value in the result is distinct, and comes from the original set — the exact
two chosen (and their order) depends on the RNG implementation for the given seed.

Example 2. Without `replace`, requesting more than `size()` caps silently instead
of throwing — every entry comes back, shuffled.

```cpp
auto out2 = scores.sample(100, 7, false);
std::cout << out2.size();
```

### Output

```text
5
```

Exactly `5` entries come back (matching `scores.size()`), not `100` — capped, no
error, no duplicates.

Example 3. With `replace = true`, the same value can be drawn more than once, and
`n` can exceed `size()`.

```cpp
auto out3 = scores.sample(8, 3, true);
std::cout << out3.size();
```

### Output

```text
8
```

Example 4. `frac = 0.4` on a 5-element thread draws `round(0.4 * 5) = 2` entries.

```cpp
auto out4 = scores.sample(0.4, 11, false);
std::cout << out4.size();
```

### Output

```text
2
```

Example 5. `frac > 1.0` without `replace = true` throws.

```cpp
try {
  auto bad = scores.sample(1.5, 5, false);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::sample: frac > 1 requires replace = true
```

## Notes

- Without `replace`, a draw larger than `size()` caps at `size()` silently — never
  throws, never duplicates.
- With `replace`, `n`/`frac` may exceed `size()` freely, and repeats are possible.
- `frac > 1.0` requires `replace = true`, or it throws `"Thread::sample: frac > 1
  requires replace = true"`.
- `n < 0` throws `"Thread::sample: n must be >= 0"`.
- The overloads without an explicit `random_state` draw a fresh seed via
  `std::random_device` every call — never reproducible.
- See the important callout above regarding cross-build reproducibility of a given
  `random_state`.

## See Also

- [`Thread::shuffle`](shuffle.md): randomize order without changing which entries
  are present.
- [`Thread::take`](take.md): select entries by explicit, deterministic position.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rxor.html">
    <span class="label">Previous</span>
    <span class="title">rxor</span>
  </a>
  <a class="next-link" href="scan.html">
    <span class="label">Next</span>
    <span class="title">scan</span>
  </a>
</div>
