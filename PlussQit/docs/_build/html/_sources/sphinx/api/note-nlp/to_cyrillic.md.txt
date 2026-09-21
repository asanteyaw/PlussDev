<!-- pluss-hand-authored -->
# NoteNlpAccessor::to_cyrillic

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::to_cyrillic() const
```

## Romanize Cyrillic Letters Into ASCII

`to_cyrillic()` replaces every Cyrillic letter in each cell with a
Latin approximation of its sound, following the common Russian romanization:
`ж` → `zh`, `ч` → `ch`, `ш` → `sh`, `щ` → `shch`, `х` → `kh`, `ц` → `ts`,
`ё` → `yo`, `ю` → `yu`, `я` → `ya`, and the plain letters as expected.

### What is converted

- **Both cases fold to lower-case Latin.** `Москва` → `moskva`.
- **The hard sign `ъ` and soft sign `ь` are dropped** — they carry no vowel
  or consonant value in a Latin rendering, so `подъезд` → `podezd` and
  `день` → `den`.
- **Combining diacritics are dropped**, so a stressed vowel written as a base
  letter plus a combining accent transliterates to just the base.
- The alphabet covered is the 33-letter modern Russian set. Letters specific
  to other Cyrillic languages (Ukrainian `і ї є ґ`, Serbian `ђ ј љ њ ћ џ`,
  and so on) are **not** in the table and pass through unchanged.

Any non-Cyrillic character — Latin letters, digits, punctuation, spaces —
passes through unchanged, so mixed-script text keeps its non-Cyrillic parts.

The operation is shape-preserving and cell-independent, works on UTF-8 text
(a bytes-mode `Note` is rejected with `NoteNlpAccessor::to_cyrillic requires
a UTF-8 text Note (not bytes)`), and returns a fresh UTF-8 `Note`.

## Example 1: Names And Multi-Letter Sounds

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Москва", "Пушкин", "щи", "Ёлка"}).nlp().to_cyrillic())
    << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["moskva", "pushkin", "shchi", "yolka"]
```

`щ` expands to `shch`; `Ё` to `yo`; capitalization is not preserved.

## Example 2: The Hard And Soft Signs Vanish

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"подъезд", "день"}).nlp().to_cyrillic())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["podezd", "den"]
```

## Example 3: Mixed Script

```cpp
std::cout << pluss::make_note({"тест test"}).nlp().to_cyrillic() << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["test test"]
```

The Cyrillic `тест` becomes `test`; the Latin `test` and the space are
untouched.

## See Also

- [`NoteNlpAccessor::to_greek`](to_greek.md): the same idea for
  Greek.
- [`NoteNlpAccessor::to_ascii`](to_ascii.md): fold Latin accents
  and replace anything else with `?`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_ascii.html"><span class="label">Previous</span><span class="title">to_ascii</span></a>
  <a class="next-link" href="to_greek.html"><span class="label">Next</span><span class="title">to_greek</span></a>
</div>
