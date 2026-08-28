# Note::strip

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::strip() const -> Note
Note::strip(std::string_view chars) const -> Note
```

## What It Does

Removes boundary characters from each text cell.

This method has two common modes:

- plain whitespace trimming
- explicit trimming of a chosen set of boundary characters

That makes it one of the core cleanup primitives in the `Note` surface.

The method is intentionally conservative: it only trims the left and right
edges of each cell. Interior characters are left untouched.

## Parameters

**chars**

Optional set of characters to trim from both ends of each cell.

- `strip()`
  Uses the default whitespace-trimming behavior. This is the overload you want
  for ordinary cleanup of copied text, identifiers with padding, and cells that
  arrived from CSV or spreadsheet imports with accidental leading or trailing
  spaces or tabs.
- `strip(chars)`
  Uses an explicit boundary character set instead of default whitespace. Every
  character present in `chars` is eligible for removal from the left boundary,
  the right boundary, or both, until a non-matching boundary character is
  reached.

This is boundary trimming only. Interior occurrences are untouched.

## Returns

`Note`
    A new `Note` with the same shape as the input and trimmed boundaries.

## Examples

Example 1. Remove leading and trailing whitespace.

```cpp
auto note = pluss::make_note({"  Ada Lovelace  ", "\tGRACE HOPPER\t", "Linus"});
auto out = note.strip();
std::cout << "strip:\n" << out << "\n";
```

### Output

```text
strip:
  [0] Ada Lovelace
  [1] GRACE HOPPER
  [2] Linus
```

Example 2. Trim a custom boundary character set instead of whitespace.

```cpp
auto tagged = pluss::make_note({"##alpha##", "#beta", "gamma##"});
auto out = tagged.strip("#");
std::cout << "strip_hash_space:\n" << pluss::note::print(out) << "\n";
```

### Output

```text
strip_hash_space:
  [0] alpha
  [1] beta
  [2] gamma
```

Example 3. Interior matches are preserved because `strip()` is a boundary
operation, not a global deletion operation.

```cpp
auto note = pluss::make_note({"#alpha#beta#", "##gamma##"});
auto trimmed = note.strip("#");
pluss::OutputEngine out;
out.writeln("strip_boundary_only:");
out.writeln(trimmed);
```

### Output

```text
strip_boundary_only:
  [0] alpha#beta
  [1] gamma
```

## Notes

- `strip()` preserves shape.
- `strip(chars)` does not touch interior matches. It only trims the outer
  boundary runs.
- This method is frequently paired with [`lower`](lower.md) for standardized
  text normalization.

## See Also

- [`Note::lower`](lower.md): normalize case after trimming.
- [`Note::replace`](replace.md): rewrite interior substrings rather than
  trimming boundaries.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split_whitespace.html">
    <span class="label">Previous</span>
    <span class="title">split_whitespace</span>
  </a>
  <a class="next-link" href="tex_escape.html">
    <span class="label">Next</span>
    <span class="title">tex_escape</span>
  </a>
</div>
