<!-- pluss-hand-authored -->
# Note::capitalize

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::capitalize() const
```

## Uppercase The First Character, Lowercase The Rest

`capitalize()` returns a new `Note`, same shape as the input, with each cell
"sentence-cased": the **first character** is uppercased in place and every
following ASCII letter is lowercased. Use it to normalize a free-text field
to a single leading capital — a name, a short label, a one-line description.

The transform acts on **position 0** regardless of what character is there. A leading space or digit is left
as-is (uppercasing a non-letter is a no-op), and the rest of the cell is
lowercased from the second character on. It does **not** search for the first
letter — `"3rd place"` stays `"3rd place"`, not `"3Rd place"`. For "capital
letter at the start of every word", use [`title`](title.md).

The lower-casing of the tail is **ASCII-only**: a non-ASCII letter after
position 0 is passed through unchanged. `capitalize()` changes case only, is
idempotent, shape-preserving, and returns a fresh `Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"heLLo wORLd", "ALL CAPS", "3rd place", "café MODE"}).capitalize())
    << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Hello world", "All caps", "3rd place", "Café mode"]
```

`3rd place` is unchanged (position 0 is a digit); `café MODE` →
`Café mode` (the `é` after position 0 is passed through, the ASCII tail is
lowercased).

## Example 2: A Scalar

```cpp
const pluss::Note n = pluss::make_note("mIxEd").capitalize();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"Mixed"
```

## Example 3: A Leading Space Or Digit Is Left As-Is

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({"  spaced", "42 answers"}).capitalize())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["  spaced", "42 answers"]
```

Position 0 is a space / a digit; there is nothing to uppercase, and the rest
is already lowercase. Chain [`strip`](strip.md) first if the leading space
should go.

## Example 4: Shape Is Preserved

```cpp
std::cout << pluss::note::print(pluss::make_note({{"aB", "cD"}}).capitalize())
          << "\n";
```

### Output

```text
Note(shape=[1, 2], dim=2, numel=2, encoding=utf8)
[
  ["Ab", "Cd"]
]
```

## See Also

- [`Note::title`](title.md): capitalize the first letter of **every** word.
- [`Note::upper`](upper.md) / [`Note::lower`](lower.md): full-cell case
  folding.
- [`Note::strip`](strip.md): drop a leading space before capitalizing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="base64_encode.html">
    <span class="label">Previous</span>
    <span class="title">base64_encode</span>
  </a>
  <a class="next-link" href="casefold.html">
    <span class="label">Next</span>
    <span class="title">casefold</span>
  </a>
</div>
