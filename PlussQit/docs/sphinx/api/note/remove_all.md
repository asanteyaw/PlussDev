# Note::remove_all

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::remove_all(std::string_view needle) const -> Note
```

## What It Does

Removes every literal occurrence of a substring from each text cell.

This differs from [`replace`](replace.md) in one practical way: you are not
rewriting the match into something else, you are deleting it entirely.

That makes `remove_all()` useful for:

- marker cleanup
- substring erasure before tokenization
- deleting repeated labels or prefixes without involving regex

The key behavioral point is that `remove_all()` is global within each cell:
every matching substring is removed, not just the first occurrence and not just
boundary occurrences.

## Parameters

**needle**

The literal substring to remove everywhere it appears in each cell.

Interior occurrences are removed too, not just boundary occurrences. If you
only want to trim a boundary run, use [`strip`](strip.md) instead.

`needle` is treated literally. Characters such as `.` or `*` have no regex
meaning here.

## Returns

`Note`
    A shape-preserving `Note` with all literal occurrences removed.

## Examples

Example 1. Remove every `beta` substring from each cell.

```cpp
auto note = pluss::make_note({"alpha beta beta", "beta only"});
auto cleaned = note.remove_all("beta");
std::cout << "remove_all:\n" << cleaned << "\n";
```

### Output

```text
remove_all:
  [0] alpha  
  [1]  only
```

Example 2. Normalize the result afterward when deletion leaves boundary noise.

```cpp
auto note = pluss::make_note({"##alpha##", "##beta"});
auto cleaned = note.remove_all("#");
auto normalized = cleaned.strip();
std::cout << "remove_all_strip:\n" << pluss::note::print(normalized) << "\n";
```

### Output

```text
remove_all_strip:
  [0] alpha
  [1] beta
```

Example 3. Remove a repeated marker substring literally. A trailing bare `id`
remains because the pattern here is `id-`, not `id`.

```cpp
auto note = pluss::make_note({"id-001-id", "id-204"});
auto cleaned = note.remove_all("id-");
pluss::tils::OutputEngine out;
out.writeln("remove_all_marker:");
out.writeln(cleaned);
```

### Output

```text
remove_all_marker:
  [0] 001-id
  [1] 204
```

## Notes

- `remove_all()` is literal substring erasure, not regex erasure.
- The first example intentionally leaves doubled spaces behind. Deletion and
  whitespace normalization are separate concerns.
- The third example is intentionally exact: `id-` is removed, but the trailing
  bare `id` in `001-id` remains because it is not the same literal substring.
- If you want the result normalized afterward, pair this with
  [`strip`](strip.md) or another whitespace cleanup pass.
- This is often the cleanest intermediate step before retokenizing text.

## See Also

- [`Note::replace`](replace.md): rewrite a substring instead of deleting it.
- [`Note::strip`](strip.md): clean up boundary whitespace or marker runs after
  removal.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tex_to_text.html">
    <span class="label">Previous</span>
    <span class="title">tex_to_text</span>
  </a>
  <a class="next-link" href="quote.html">
    <span class="label">Next</span>
    <span class="title">quote</span>
  </a>
</div>
