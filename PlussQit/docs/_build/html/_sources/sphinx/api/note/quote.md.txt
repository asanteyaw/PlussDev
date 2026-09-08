<!-- pluss-hand-authored -->
# Note::quote

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::quote() const
```

## Wrap Every Cell In Double Quotes

`quote()` returns a new `Note`, same shape as the input, with each cell
wrapped in a leading and trailing `"`. Use it to render text for a context
that expects quoted tokens — a CSV field that may contain spaces, a
generated argument list, a display column where the boundaries of each value
should be visible, or a log line where trailing whitespace would otherwise be
invisible.

The wrap is **literal and minimal**: exactly one `"` is prepended and one
appended, and the cell content in between is copied **verbatim**. `quote()`
does **not** escape interior `"` characters, does not add a `\` before
anything, and does not skip cells that are already quoted. So
`quote()` applied to `a"b` produces `"a"b"` — three quote characters, which
is not a well-formed quoted literal. If the content can contain `"`, escape
or replace it first (for example [`replace`](replace.md) `"` with `\"`), or
use a real serializer.

`unquote()` is the inverse: it strips one matching pair of leading/trailing
`"` or `'`. Because neither method escapes, `quote().unquote()` round-trips
exactly only when the original content has no delimiter quotes of its own.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. An empty cell becomes `""` (two bytes).

## Example 1: A One-Dimensional Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"payload", "two words", ""}).quote())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\"payload\"", "\"two words\"", "\"\""]
```

`print` shows the added quotes as escaped `\"` inside its own quoting; the
cells actually hold `"payload"`, `"two words"`, and `""`.

## Example 2: A Scalar

```cpp
const pluss::Note n = pluss::make_note("solo").quote();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"\"solo\""
```

The cell holds the five characters `"solo"`.

## Example 3: Shape Is Preserved

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({{"a", "b"}, {"c", "d"}}).quote())
    << "\n";
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["\"a\"", "\"b\""], 
  ["\"c\"", "\"d\""]
]
```

## Example 4: Interior Quotes Are Not Escaped

```cpp
std::cout << pluss::note::print(pluss::make_note({"a\"b"}).quote()) << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["\"a\"b\""]
```

The cell now holds `"a"b"` — a bare wrap, not a valid quoted string. Replace
or escape interior `"` first if that matters.

## Example 5: Round-Trip With `unquote`

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"payload", "  spaced  "}).quote().unquote())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["payload", "  spaced  "]
```

`unquote()` removes exactly the pair `quote()` added, so the boundary spaces
in the second cell survive intact.

## See Also

- `pluss::coden::Note::unquote`: strip one matching pair of leading/trailing
  `"` or `'`.
- `pluss::coden::Note::surround`: wrap each cell in an arbitrary
  prefix/suffix pair rather than double quotes.
- [`Note::replace`](replace.md): escape interior `"` before quoting when
  the result must be a valid literal.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nlp_remove_urls.html">
    <span class="label">Previous</span>
    <span class="title">nlp_remove_urls</span>
  </a>
  <a class="next-link" href="regex_findall.html">
    <span class="label">Next</span>
    <span class="title">regex_findall</span>
  </a>
</div>
