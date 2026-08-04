# Note::replace

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::replace(std::string_view from, std::string_view to) const -> Note
```

## What It Does

Replaces every occurrence of one literal substring with another, cell by cell.

This is the direct batch-text rewrite primitive for simple substitutions:

- renaming markers
- fixing labels
- replacing tokens before export
- standardizing text fragments without invoking regex machinery

`replace()` is global within each cell: every literal match is rewritten.
It is not a "first match only" primitive and it is not regex-aware.

## Parameters

**from**

The literal substring to replace.

**to**

The literal replacement text.

Every cell is processed independently, and cells that do not contain the
substring are left unchanged.

Neither `from` nor `to` is interpreted as regex syntax here. The method is
strictly literal on both sides of the substitution.

## Returns

`Note`
    A new `Note` with the same shape as the input and the requested substring
    substitutions applied.

## Examples

Example 1. Rewrite a common label prefix.

```cpp
auto note = pluss::make_note({"score: 75", "score: 88", "pending"});
auto out = note.replace("score", "mark");
std::cout << "replace:\n" << out << "\n";
```

### Output

```text
replace:
  [0] mark: 75
  [1] mark: 88
  [2] pending
```

Example 2. Replace structural separators when you want a more readable surface.

```cpp
auto names = pluss::make_note({"Ada_Lovelace", "Grace_Hopper", "Linus"});
auto out = names.replace("_", " ");
std::cout << "replace_underscore:\n" << pluss::print_note(out) << "\n";
```

### Output

```text
replace_underscore:
  [0] Ada Lovelace
  [1] Grace Hopper
  [2] Linus
```

Example 3. Replace one marker, then inspect whether the old marker is still
present.

```cpp
auto note = pluss::make_note({"midterm_1", "midterm_2", "final_exam"});
auto renamed = note.replace("midterm", "exam");
auto still_old = renamed.contains("midterm");
pluss::OutputEngine out;
out.writeln("replace_then_contains:");
out.writeln(renamed);
std::cout << "replace_then_contains_mask:";
for (std::uint8_t v : still_old) {
  std::cout << " " << static_cast<int>(v);
}
std::cout << "\n";
```

### Output

Renamed note:

```text
replace_then_contains:
  [0] exam_1
  [1] exam_2
  [2] final_exam
```

Mask after replacement:

```text
replace_then_contains_mask: 0 0 0
```

## Notes

- `replace()` is literal replacement, not regex replacement.
- The two string parameters are symmetric in the sense that both are treated as
  literal text. There is no overload here that interprets either side as a
  regex program.
- If you only want the first or last occurrence changed, the broader `Note`
  surface also exposes specialized replacement variants such as
  `replace_first(...)` and `replace_last(...)`.
- This is often paired with [`strip`](strip.md) when the surrounding text also
  needs boundary cleanup.

## See Also

- [`Note::contains`](contains.md): check for the substring before or after
  replacement.
- [`Note::tex_escape`](tex_escape.md): escape special characters rather than
  replacing a literal substring yourself.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="regex_findall.html">
    <span class="label">Previous</span>
    <span class="title">regex_findall</span>
  </a>
  <a class="next-link" href="split_lines.html">
    <span class="label">Next</span>
    <span class="title">split_lines</span>
  </a>
</div>
