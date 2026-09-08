<!-- pluss-hand-authored -->
# pluss::note::rand_between

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::rand_between(string_view lo, string_view hi,
                               integer_range sizes, std::uint64_t seed = 0)
Note pluss::note::rand_between(string_view lo, string_view hi,
                               std::initializer_list<std::int64_t> sizes,
                               std::uint64_t seed = 0)
```

## Random Single Characters From A Chosen Range

`rand_between()` fills a `Note` of shape `sizes` with single characters
drawn uniformly from the **inclusive** ASCII range `[lo, hi]`. Where
[`rand`](rand.md) is fixed to the alphabet, `rand_between` lets you name the
endpoints: digits `"0"`–`"9"`, a hex-nibble set `"0"`–`"f"` (with the gap
characters included — see below), a punctuation band, an arbitrary slice of
the ASCII table.

### Parameters

- **`lo`, `hi`** — each exactly one ASCII character (code point
  `0x00`–`0x7F`). They are the inclusive bounds of the draw, read as byte
  values. `lo` must be `<= hi`.
- **`sizes`** — the output shape, same rules as [`full`](full.md): an
  `initializer_list` or any contiguous `std::int64_t` range, empty for a
  scalar, non-negative extents, rank at most three.
- **`seed`** — the PRNG seed, with the same contract as `rand`: a non-zero
  `seed` is fully deterministic for a given `lo`, `hi`, and `sizes`; `seed`
  `0` (default) draws fresh entropy each call.

The draw is over **contiguous code points**, not over a curated character
class. `rand_between("0", "z", …)` therefore includes `:`, `;`, `A`–`Z`,
`[`, and so on — every byte between `'0'` (0x30) and `'z'` (0x7A). Choose
`lo` and `hi` so the whole span is acceptable, or post-filter the result.

The result is **UTF-8 mode**.

## Example 1: A Column Of Random Digits

```cpp
std::cout << pluss::note::print(pluss::note::rand_between("0", "9", {10}, 5))
          << "\n";
```

### Output

```text
Note(shape=[10], dim=1, numel=10, encoding=utf8)
["6", "0", "8", "2", "4", "1", "3", "9", "8", "6"]
```

Ten independent draws from `'0'`..`'9'` inclusive, fixed by seed `5`.

## Example 2: A Lower-Case Letter Block

```cpp
const pluss::Note n = pluss::note::rand_between("a", "z", {2, 3}, 5);
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["w", "a", "i"], 
  ["c", "u", "r"]
]
```

Any rank up to three; draw order is row-major.

## Example 3: `lo` Greater Than `hi` Is Rejected

```cpp
try {
  pluss::note::rand_between("z", "a", {3}, 1);
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::note::rand_between: expected lo <= hi
```

## Example 4: A Multi-Character Bound Is Rejected

```cpp
try {
  pluss::note::rand_between("ab", "z", {3}, 1);
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::note::rand_between: expected a single ASCII character
```

Each bound must be exactly one ASCII character.

## See Also

- [`pluss::note::rand`](rand.md): random letters, fixed to the alphabet.
- [`pluss::note::arange`](arange.md): a deterministic stepped run over a
  character range.
- [`pluss::note::full`](full.md): one fixed value across a shape.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rand.html">
    <span class="label">Previous</span>
    <span class="title">rand</span>
  </a>
  <a class="next-link" href="rep.html">
    <span class="label">Next</span>
    <span class="title">rep</span>
  </a>
</div>
