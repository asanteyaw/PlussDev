<!-- pluss-hand-authored -->
# pluss::note::from_words

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::from_words(string_view text)
```

## Split One String Into A Column Of Whitespace-Separated Tokens

`from_words()` takes a single string and returns a rank-`1` `Note` with one
cell per whitespace-delimited run of non-space characters. It is the quick
path from a sentence, a command line, or a space-separated record to an
addressable token column.

Separators are ASCII whitespace — space, tab (`\t`), newline (`\n`),
carriage return (`\r`), vertical tab, and form feed. **Any run** of one or
more of these characters is a single separator, so consecutive spaces,
leading whitespace, and trailing whitespace never produce empty cells. This
is the "split on whitespace" behavior of most scripting languages, not a
fixed single-delimiter split.

The result is always rank `1` and **UTF-8 mode**. Tokens are taken as raw
byte spans between separators, so multi-byte UTF-8 characters inside a token
are preserved; only ASCII whitespace ends a token. An input that is empty or
all whitespace yields a zero-length `Note`.

`from_words` does no punctuation handling — `"end."` is one token including
the period. Strip or split punctuation with `Note` methods afterward if you
need bare words.

## Example 1: A Sentence With Irregular Spacing

```cpp
std::cout << pluss::note::print(pluss::note::from_words("  the   quick brown  "))
          << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["the", "quick", "brown"]
```

Leading, trailing, and doubled spaces all collapse; no empty cells.

## Example 2: Mixed Whitespace Kinds

```cpp
const pluss::Note n = pluss::note::from_words("tab\tand\nnewline");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["tab", "and", "newline"]
```

Tabs and newlines separate tokens just as spaces do.

## Example 3: A Single Token

```cpp
pluss::note::print(std::cout, pluss::note::from_words("single")) << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["single"]
```

Still a rank-`1` `Note` — one cell, not a scalar.

## Example 4: Only Whitespace

```cpp
std::cout << pluss::note::print(pluss::note::from_words("   ")) << "\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
```

No tokens, so a zero-length `Note`.

## See Also

- [`pluss::note::from_lines`](from_lines.md): split on line breaks instead
  of on whitespace runs.
- [`pluss::note::perm`](perm.md): generate character selections of a word
  rather than reading words from text.
- [`pluss::note::make`](make.md): build a column from tokens you already
  hold separately.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_lines.html">
    <span class="label">Previous</span>
    <span class="title">from_lines</span>
  </a>
  <a class="next-link" href="full.html">
    <span class="label">Next</span>
    <span class="title">full</span>
  </a>
</div>
