<!-- pluss-hand-authored -->
# Note::NlpAccessor::to_greek

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::to_greek() const
```

## Romanize Greek Letters Into ASCII

`to_greek()` replaces every Greek letter in each cell with a Latin
approximation of its sound: `Α`/`α` → `a`, `Θ`/`θ` → `th`, `Χ`/`χ` → `ch`,
`Ψ`/`ψ` → `ps`, `Ω`/`ω` → `o`, and so on. Use it to render Greek names, terms,
or math-variable names in a Latin-only context.

### What is converted

- **Both cases fold to lower-case Latin.** `Α` and `α` both become `a`; the
  method does not preserve capitalization.
- **Accented (monotonic) vowels** are handled: `ά έ ή ί ό ύ ώ` and their
  capitals map to the same base as the unaccented letter (`ά` → `a`), as do
  the dialytika forms `ϊ ϋ ΐ ΰ`.
- **Combining diacritics are dropped**, so
  [`to_nfd`](../note/to_nfd.md)`().nlp().to_greek()` also handles **polytonic**
  Greek (breathings and the circumflex), which is stored as a base letter
  plus separate marks.
- The word-final sigma `ς` maps to `s`, the same as `σ`/`Σ`.

Any character that is **not** Greek — Latin letters, digits, spaces,
punctuation — passes through unchanged, so mixed-script text keeps its
non-Greek parts intact.

The operation is shape-preserving and cell-independent, works on UTF-8 text
(a bytes-mode `Note` is rejected with `Note::NlpAccessor::to_greek requires a
UTF-8 text Note (not bytes)`), and returns a fresh UTF-8 `Note`.

## Example 1: Accented Words

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Ελλάς", "θεωρία", "Χάος", "ΩΜΕΓΑ"}).nlp().to_greek())
    << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["ellas", "theoria", "chaos", "omega"]
```

The tonos on `ά` is folded away; `ΩΜΕΓΑ` (all caps) still comes out lower-case.

## Example 2: Mixed Script Keeps Its Latin Parts

```cpp
std::cout << pluss::make_note({"alpha=Α beta=β"}).nlp().to_greek() << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["alpha=a beta=b"]
```

Only `Α` and `β` change; `alpha=`, ` `, `=`, and `beta` are untouched.

## Example 3: Final Sigma

```cpp
std::cout << pluss::make_note({"λόγος"}).nlp().to_greek() << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["logos"]
```

## Example 4: Polytonic Greek Via NFD

```cpp
std::cout << pluss::make_note({"ἄνθρωπος"}).to_nfd().nlp().to_greek() << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["anthropos"]
```

`ἄ` carries a breathing mark and an acute accent. [`to_nfd`](../note/to_nfd.md)
splits it into `α` plus two combining marks; `to_greek` maps the
`α` and drops the marks.

## See Also

- [`Note::NlpAccessor::to_cyrillic`](to_cyrillic.md): the same idea
  for the Cyrillic alphabet.
- [`Note::NlpAccessor::to_ascii`](to_ascii.md): fold Latin accents
  and replace anything else with `?`.
- [`Note::to_nfd`](../note/to_nfd.md): decompose accents so polytonic Greek
  transliterates cleanly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_cyrillic.html"><span class="label">Previous</span><span class="title">to_cyrillic</span></a>
  <a class="next-link" href="tokens.html"><span class="label">Next</span><span class="title">tokens</span></a>
</div>
