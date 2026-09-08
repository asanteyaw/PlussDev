<!-- pluss-hand-authored -->
# pluss::note::rand

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::rand(integer_range sizes, bool capitalize = true,
                       std::uint64_t seed = 0)
Note pluss::note::rand(std::initializer_list<std::int64_t> sizes,
                       bool capitalize = true, std::uint64_t seed = 0)
```

## One Random Letter Per Cell

`rand()` allocates a `Note` of shape `sizes` and fills each cell with a
single random letter drawn uniformly from the 26-letter English alphabet.
It is the text counterpart of `pluss::series::rand` — a quick source of
throwaway data shaped exactly how you need it, for tests, fixtures,
placeholder tables, and examples.

Every cell is an independent draw of one character. `rand` does not build
words or syllables; for random *words*, draw letters and join, or use a
domain-specific generator.

### Parameters

- **`sizes`** — the output shape, same rules as [`full`](full.md): an
  `initializer_list` (no wrapper needed) or any contiguous `std::int64_t`
  range, empty for a scalar, non-negative extents, rank at most three.
- **`capitalize`** — `true` (default) draws from `A`–`Z`; `false` draws
  from `a`–`z`.
- **`seed`** — the PRNG seed. A **non-zero** `seed` makes the draw fully
  deterministic: the same `seed`, `sizes`, and `capitalize` always produce
  the same `Note`. A `seed` of `0` (the default) is drawn from a
  process-wide entropy source, so each call differs — use `0` for genuine
  scratch data, a fixed non-zero value whenever the output must be
  reproducible (tests, documented examples).

The result is **UTF-8 mode**. Filling `n` cells uses storage proportional to
`n`.

## Example 1: A Seeded Row Of Capitals

```cpp
std::cout << pluss::note::print(pluss::note::rand({6}, true, 42)) << "\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["W", "I", "K", "O", "V", "A"]
```

Six independent draws from `A`–`Z`. Seed `42` fixes the sequence.

## Example 2: The Same Seed Reproduces The Same Draw

```cpp
const pluss::Note again = pluss::note::rand({6}, true, 42);
std::cout << again << "\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["W", "I", "K", "O", "V", "A"]
```

Identical to Example 1 — a non-zero seed is a reproducibility guarantee.

## Example 3: Lower-Case Letters

```cpp
std::cout << pluss::note::print(pluss::note::rand({6}, false, 42)) << "\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["w", "i", "k", "o", "v", "a"]
```

`capitalize = false` draws from `a`–`z`. With the same seed the letter
*positions* in the alphabet match Example 1, lower-cased.

## Example 4: A Two-Dimensional Block

```cpp
const pluss::Note n = pluss::note::rand({2, 4}, true, 7);
pluss::note::print(std::cout, n) << "\n";
```

### Output

```text
Note(shape=[2, 4], dim=2, numel=8, encoding=utf8)
[
  ["H", "C", "O", "W"], 
  ["M", "B", "G", "B"]
]
```

Any rank up to three; the draw order follows row-major cell order.

## Example 5: A Scalar Draw

```cpp
std::cout << pluss::note::print(pluss::note::rand({}, true, 99)) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"D"
```

An empty `sizes` list gives a single-cell scalar `Note`.

## See Also

- [`pluss::note::rand_between`](rand_between.md): random single characters
  from an arbitrary ASCII range, not just letters.
- [`pluss::note::arange`](arange.md): a deterministic stepped run instead of
  a random draw.
- [`pluss::note::full`](full.md): fill a shape with one fixed value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="print.html">
    <span class="label">Previous</span>
    <span class="title">print</span>
  </a>
  <a class="next-link" href="rand_between.html">
    <span class="label">Next</span>
    <span class="title">rand_between</span>
  </a>
</div>
