<!-- pluss-hand-authored -->
# Note::remove_prefix

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::remove_prefix(std::string_view prefix) const
```

## Strip A Leading Prefix When Present

`remove_prefix(prefix)` returns a new `Note`, same shape as the input, with
one leading occurrence of the literal string `prefix` removed from each cell
that begins with it. Cells that do not start with `prefix` are left
unchanged. Use it to drop a mount point, a namespace, a scheme, or a
uniform leading tag.

The match is a **literal, exact prefix check** — only when the cell's first
`prefix.size()` bytes equal `prefix` is anything removed, and only **one**
copy is removed even if the cell begins with `prefix` twice. It is the near
inverse of [`ensure_prefix`](ensure_prefix.md) (though neither is a perfect
inverse of the other, since `ensure_prefix` will not remove and
`remove_prefix` will not add). It removes only from the start; use
[`remove`](remove.md) or [`replace`](replace.md) for a match anywhere.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: Drop A Leading Path Segment

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"src/main.cpp", "docs/x", "src/src/y"}).remove_prefix("src/"))
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["main.cpp", "docs/x", "src/y"]
```

`"docs/x"` does not start with `src/` and is unchanged; `"src/src/y"` loses
only its first `src/`.

## Example 2: Absent Prefix Leaves The Cell Alone

```cpp
const pluss::Note n = pluss::make_note({"abc"}).remove_prefix("xyz");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["abc"]
```

## See Also

- [`Note::remove_suffix`](remove_suffix.md): the trailing-end counterpart.
- [`Note::ensure_prefix`](ensure_prefix.md): add a prefix if missing.
- [`Note::starts_with`](starts_with.md): test for the prefix first.
- [`Note::strip`](strip.md): `strip(chars)` removes **characters** (a set)
  from the boundary, not a fixed string.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_letters.html">
    <span class="label">Previous</span>
    <span class="title">remove_letters</span>
  </a>
  <a class="next-link" href="remove_punctuation.html">
    <span class="label">Next</span>
    <span class="title">remove_punctuation</span>
  </a>
</div>
