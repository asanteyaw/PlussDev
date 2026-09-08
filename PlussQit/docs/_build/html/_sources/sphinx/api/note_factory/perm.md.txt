<!-- pluss-hand-authored -->
# pluss::note::perm

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::perm(string_view word, std::int64_t pick,
                       WordSelectMode mode = WordSelectMode::Combination,
                       bool with_replacement = false,
                       bool unique_results = true)
```

`WordSelectMode` is `pluss::note::WordSelectMode`, an enum with values
`Combination` and `Permutation`.

## Combinations And Permutations Of A Word's Characters

`perm()` enumerates length-`pick` selections of the characters of `word` and
returns them as a rank-`1` `Note`, one selection per cell. It is a
combinatorial generator: anagram and word-game candidate lists, brute-force
key spaces over a small alphabet, test corpora that need every arrangement
of a set of letters.

The characters of `word` are treated **positionally** — the pool is "the
characters at positions `0..n-1`", not "the distinct letters". Repeated
letters in `word` therefore contribute multiple pool entries; whether that
surfaces as duplicate output strings depends on `unique_results`.

### Parameters

- **`word`** — the character pool, taken in left-to-right order.
- **`pick`** — how many characters each selection contains. Must be `>= 0`.
  `pick == 0` yields a single empty selection (`[""]`). Without replacement,
  `pick` greater than `word.size()` is impossible and yields an empty
  `Note` (not an error).
- **`mode`**:
  - `WordSelectMode::Combination` (default) — order does not matter; each
    selection keeps `word`'s left-to-right order and picks strictly
    forward. `"abc"`, `pick 2` → `ab`, `ac`, `bc`.
  - `WordSelectMode::Permutation` — order matters; every arrangement is
    emitted. `"abc"`, `pick 2` → `ab`, `ac`, `ba`, `bc`, `ca`, `cb`.
- **`with_replacement`** — when `true`, a position may be reused within one
  selection, so `pick` can exceed `word.size()`. `"ab"`, `pick 2`,
  combination, with replacement → `aa`, `ab`, `bb`.
- **`unique_results`** — when `true` (default), identical output strings are
  collapsed to their first occurrence; when `false`, every selection the
  enumeration produces is kept, including duplicates that arise from
  repeated letters in `word`.

The result is rank `1` and **UTF-8 mode**, in the order the enumeration
visits selections (depth-first over `word`'s positions). `perm` is
deterministic — there is no seed.

The count grows combinatorially with `word.size()` and `pick`. Keep both
small; this is a generator for tens or hundreds of results, not millions.

## Example 1: Combinations (Order Ignored)

```cpp
std::cout << pluss::note::print(pluss::note::perm("abc", 2)) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["ab", "ac", "bc"]
```

The three ways to choose 2 of 3 characters, each in `word` order.

## Example 2: Permutations (Order Matters)

```cpp
const pluss::Note n =
    pluss::note::perm("abc", 2, pluss::note::WordSelectMode::Permutation);
std::cout << n << "\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["ab", "ac", "ba", "bc", "ca", "cb"]
```

Every ordered arrangement of 2 characters.

## Example 3: With Replacement

```cpp
pluss::note::print(
    std::cout,
    pluss::note::perm("ab", 2, pluss::note::WordSelectMode::Combination, true))
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["aa", "ab", "bb"]
```

A position may repeat within a selection, so `pick` is not capped at
`word.size()`.

## Example 4: Repeated Letters Are Deduplicated By Default

```cpp
std::cout << pluss::note::print(pluss::note::perm("aab", 2)) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["aa", "ab"]
```

The pool is `a, a, b`. The positional enumeration produces `aa`, `ab`,
`ab`; `unique_results = true` keeps the first of each — `aa`, `ab`.

## Example 5: Keeping Every Selection

```cpp
const pluss::Note n = pluss::note::perm(
    "aab", 2, pluss::note::WordSelectMode::Combination, false, false);
std::cout << n << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["aa", "ab", "ab"]
```

`unique_results = false` keeps the duplicate `ab` that the two `a`
positions generate.

## Example 6: `pick` Of Zero

```cpp
std::cout << pluss::note::print(pluss::note::perm("abc", 0)) << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
[""]
```

Exactly one selection of length zero: the empty string.

## Example 7: `pick` Larger Than The Word

```cpp
const pluss::Note n = pluss::note::perm("abc", 4);
std::cout << n << "\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
```

Without replacement there is no way to choose 4 from 3 characters, so the
result is an empty rank-`1` `Note` — not an error. (With
`with_replacement = true` it would instead enumerate all length-4
selections.)

## See Also

- [`pluss::note::rand`](rand.md): random letters rather than an exhaustive
  enumeration.
- [`pluss::note::arange`](arange.md): a single ordered run of characters.
- [`pluss::note::from_words`](from_words.md): build a `Note` from words that
  already exist in a string.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="make.html">
    <span class="label">Previous</span>
    <span class="title">make</span>
  </a>
  <a class="next-link" href="print.html">
    <span class="label">Next</span>
    <span class="title">print</span>
  </a>
</div>
