<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::replace

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signatures

```cpp
DataFrameNoteAccessor::replace(std::string_view from,
                                std::string_view to) const -> DataFrame

DataFrameNoteAccessor::replace(std::string_view from,
                                std::string_view to,
                                const TextOptions& options) const -> DataFrame
```

## What It Does

Replaces occurrences of `from` with `to` in every selected text feature's
cells, returning a new frame. Like [`count()`](count.md), the two-argument
overload's default (`{}`) is a **literal, case-sensitive, unlimited**
replace — but unlike `count()`, this default matches what the
single-argument-style call actually does, so there's no regex-by-default
surprise here: `replace(from, to)` and `replace(from, to, {})` are exactly
the same call.

```{note} Verified fast path: replace(from, to) and replace(from, to, {}) are byte-identical
When `options` is left at its default (`regex = false`, `case_sensitive =
true`, `limit = -1`), `replace()` takes a fast path straight to the
underlying `Note::replace` primitive — the same code path the
two-argument overload uses. There is no behavioral difference between the
two spellings; the two-argument overload exists purely for convenience
when no options are needed.
```

`options` controls three independent things:

- **`options.regex`** (default `false`): treat `from` as a regular
  expression instead of literal text. Regex replacement text (`to`)
  supports ECMAScript-style backreferences (`$1`, `$2`, ...) to capture
  groups in `from`.
- **`options.case_sensitive`** (default `true`): when `false`, matching
  ignores case (ASCII fold), but the replacement text `to` is inserted
  exactly as given — case-insensitive *matching* doesn't mean
  case-preserving *replacement*.
- **`options.limit`** (default `-1`): caps the number of replacements per
  cell; `-1` means unlimited. Matches are always non-overlapping — after a
  replacement, scanning resumes right after the inserted `to` text, not
  back inside it.

`options.prefix` is not used by `replace()`.

## Parameters

**from** (`std::string_view`)
    The literal text or regex pattern to search for. An empty `from` under
    literal mode is a no-op (the cell is returned unchanged).

**to** (`std::string_view`)
    The replacement text. Under regex mode, may contain `$1`, `$2`, ...
    backreferences to `from`'s capture groups.

**options** (`TextOptions`, two-argument overload only)
    `options.regex`, `options.case_sensitive`, `options.limit` as described
    above.

## Returns

`DataFrame`
    A new frame with every selected feature's text transformed by the
    replacement.

## Examples

Example 1. The default: literal, case-sensitive, unlimited replacement.

```cpp
using namespace pluss::table;

auto notes = DataFrame({
    {"text", Thread(pluss::make_note(
        {"cat and cat", "Cat there", "no match", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << notes;

auto out1 = notes.nt().replace("cat", "dog");
std::cout << out1;
```

### Output (before)

```text
==================
     | text       
------------------
0    | cat and cat
1    | Cat there  
2    | no match   
3    | NULL       
------------------
[4 cs x 1 feat]
```

### Output

```text
==================
     | text       
------------------
0    | dog and dog
1    | Cat there  
2    | no match   
3    | NULL       
------------------
[4 cs x 1 feat]
```

Both occurrences of `"cat"` in row 0 are replaced. Row 1's `"Cat"` (capital
C) doesn't match the lowercase, case-sensitive `"cat"` needle, so it's left
untouched. Row 3 is invalid and stays `NULL`.

Example 2. `case_sensitive = false`: matching ignores case, but the
replacement text is still inserted literally as given.

```cpp
auto out2 = notes.nt().replace("cat", "dog", {.case_sensitive = false});
std::cout << out2;
```

### Output

```text
==================
     | text       
------------------
0    | dog and dog
1    | dog there  
2    | no match   
3    | NULL       
------------------
[4 cs x 1 feat]
```

Now row 1's `"Cat"` matches too (case-insensitively) and is replaced with
the literal lowercase `"dog"` — the original capitalization of the matched
text (`"Cat"`) is discarded, not preserved or transferred to the
replacement.

Example 3. `limit` caps replacements per cell, and matches never overlap —
notice the second cell's *second* replacement lands on what was originally
its *third* `a`, not its second, because the first replacement's `X`
already consumed one match.

```cpp
auto words = DataFrame({
    {"word", Thread(pluss::make_note(
        {"banana", "aardvark", "xyz", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << words;

auto out3 = words.nt().replace("a", "X", {.limit = 2});
std::cout << out3;
```

### Output (before)

```text
===============
     | word    
---------------
0    | banana  
1    | aardvark
2    | xyz     
3    | NULL    
---------------
[4 cs x 1 feat]
```

### Output

```text
===============
     | word    
---------------
0    | bXnXna  
1    | XXrdvark
2    | xyz     
3    | NULL    
---------------
[4 cs x 1 feat]
```

`"banana"` has three `a`s, but only the first two are replaced (`limit =
2`), leaving the trailing `"na"` untouched: `"bXnXna"`. `"aardvark"` has
two `a`s *adjacent* to each other at the start — both get replaced
(`"XXrdvark"`), reaching the limit before the third `a` (inside
`"vark"`) is ever considered. `"xyz"` has no `a` at all, so it's
unchanged.

Example 4. Regex mode with backreferences: swap the two sides of an `@`
using capture groups.

```cpp
auto emails = DataFrame({
    {"email", Thread(pluss::make_note(
        {"alice@example", "bob@test", "invalid", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << emails;

auto out4 = emails.nt().replace(
    R"((\w+)@(\w+))", "$2#$1", {.regex = true});
std::cout << out4;
```

### Output (before)

```text
====================
     | email        
--------------------
0    | alice@example
1    | bob@test     
2    | invalid      
3    | NULL         
--------------------
[4 cs x 1 feat]
```

### Output

```text
====================
     | email        
--------------------
0    | example#alice
1    | test#bob     
2    | invalid      
3    | NULL         
--------------------
[4 cs x 1 feat]
```

`(\w+)@(\w+)` captures the text before and after `@`; `"$2#$1"` swaps them
around a `#`. Row 2 (`"invalid"`) has no `@` at all, so the pattern doesn't
match and the text is returned unchanged — `regex_replace` leaves
non-matching text exactly as it was, it never errors on a cell with zero
matches.

## Notes

- `replace(from, to)` and `replace(from, to, {})` are the exact same
  operation — see the note above.
- An empty `from` under literal mode is a no-op (returns the cell
  unchanged); this check happens before any matching is attempted.
- Matches never overlap: after a replacement, scanning resumes
  immediately past the inserted `to` text.
- `case_sensitive = false` affects matching only — the replacement text is
  always inserted exactly as given, never case-adjusted to match what was
  found.
- Regex mode supports `$1`, `$2`, ... backreferences in `to`.
- `options.prefix` has no effect on `replace()`.
- A null cell on input stays null on output.

## See Also

- [`DataFrameNoteAccessor::strip`](strip.md) / [`lstrip()`](lstrip.md) /
  [`rstrip()`](rstrip.md): remove specific characters rather than replace
  arbitrary substrings.
- [`DataFrameNoteAccessor::count`](count.md): count matches instead of
  replacing them (and note its opposite single-argument regex default).
- [`DataFrameNoteAccessor::extract`](extract.md): pull matched text out
  into new columns instead of substituting it in place.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="partition.html">
    <span class="label">Previous</span>
    <span class="title">partition</span>
  </a>
  <a class="next-link" href="rstrip.html">
    <span class="label">Next</span>
    <span class="title">rstrip</span>
  </a>
</div>
