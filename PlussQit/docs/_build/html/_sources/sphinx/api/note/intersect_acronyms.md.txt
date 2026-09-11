<!-- pluss-hand-authored -->
# Note::intersect_acronyms

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
TokenBatch Note::intersect_acronyms(const Note& other) const
```

## Shared Acronyms Between Two Notes

`intersect_acronyms(other)` reduces each cell to its set of **acronyms** and
returns the acronyms present in both cells as a `TokenBatch`. It is the
acronym-set member of the set-similarity family; the scoring members are
[`jaccard_acronyms`](jaccard_acronyms.md) and
[`overlap_acronyms`](overlap_acronyms.md).

### How the set is built

An acronym is a run of **two or more uppercase ASCII letters** on a word
boundary (`NASA`, `ESA`, `NOAA`); mixed-case names and single capitals are
not acronyms. Matches are **lowercased** for comparison, so the emitted
tokens are lowercase. If a cell has no uppercase letters the set is empty and
that cell shares nothing. The set is deduplicated.

### What is emitted

Members in both sets, in the order they first appear in `self`, once each; a
cell with no shared acronym contributes an empty range. `batch.offsets()`
(length `numel + 1`) marks each cell's run.

### Shape, pairing, and rejections

Per cell and pairwise; equal [`numel`](../../note_api_reference.md) or a
scalar (`numel() == 1`) broadcast. Bytes-mode input raises
`Note::intersect_acronyms requires a UTF-8 text Note (lhs is bytes)` (or
`rhs`); a numel mismatch raises `Note::intersect_acronyms expects equal numel
or scalar broadcast`.

## Example 1: Shared Acronyms, Lowercased

```cpp
const auto batch = pluss::make_note({"NASA and ESA and NOAA"})
                       .intersect_acronyms(pluss::make_note({"ESA, NASA, JAXA"}));

std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["nasa", "esa"]
offsets: [0, 2]
```

`NASA` and `ESA` are in both cells; `NOAA` and `JAXA` are not. The result is
in `self` order (`NASA` before `ESA`) and lowercased.

## Example 2: No Uppercase, No Acronyms

```cpp
const auto batch = pluss::make_note({"nasa esa"})
                       .intersect_acronyms(pluss::make_note({"nasa"}));
std::cout << pluss::note::print(batch.tokens()) << "\n";
std::cout << "offsets: [";
for (std::size_t i = 0; i < batch.offsets().size(); ++i)
  std::cout << (i ? ", " : "") << batch.offsets()[i];
std::cout << "]\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
offsets: [0, 0]
```

The words are lowercase, so neither cell has an acronym set.

## See Also

- [`Note::jaccard_acronyms`](jaccard_acronyms.md) /
  [`Note::overlap_acronyms`](overlap_acronyms.md): score the shared-acronym
  overlap.
- [`Note::intersect_tokens`](intersect_tokens.md) /
  [`Note::intersect_terms`](intersect_terms.md) /
  [`Note::intersect_citations`](intersect_citations.md) /
  [`Note::intersect_math_symbols`](intersect_math_symbols.md): other set
  kinds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="indent.html">
    <span class="label">Previous</span>
    <span class="title">indent</span>
  </a>
  <a class="next-link" href="intersect_citations.html">
    <span class="label">Next</span>
    <span class="title">intersect_citations</span>
  </a>
</div>
