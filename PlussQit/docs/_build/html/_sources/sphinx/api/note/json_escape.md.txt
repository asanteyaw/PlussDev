<!-- pluss-hand-authored -->
# Note::json_escape

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::json_escape() const
```

## Escape A String For A JSON String Body

`json_escape()` rewrites each cell so its characters are legal **between the
quotes** of a JSON string. It applies the escapes the JSON grammar requires
and nothing more:

| Input | Output |
|-------|--------|
| `"`   | `\"`   |
| `\`   | `\\`   |
| U+0008 backspace | `\b` |
| U+000C form feed | `\f` |
| U+000A line feed | `\n` |
| U+000D carriage return | `\r` |
| U+0009 tab | `\t` |
| any other control `U+0000`–`U+001F` | `\u00XX` (lower-case hex) |

Every byte from `0x20` up is copied through unchanged. In particular the
result is **still UTF-8** — a non-ASCII character such as `é` is *not* turned
into `\uXXXX` — and `/` is left as-is; both are valid in JSON.

`json_escape()` produces the **contents** of a JSON string. It does not add
the surrounding `"` quotes, and there is no `json_unescape` inverse. Feed the
result into a serializer, or wrap it in quotes yourself. The operation
preserves shape, keeps the input's encoding tag, and returns a fresh `Note`.

### Printing the result

The `Note` printer escapes `\` and `"` for its own display, which would show
the backslashes this method just added as `\\`. The examples below print the
raw bytes of each cell instead, so the output is exactly what `json_escape()`
wrote:

```cpp
for (std::int64_t i = 0; i < n.numel(); ++i)
  std::cout << std::string(n.at(i)) << "\n";
```

## Example 1: Quotes And Backslashes

```cpp
const auto n = pluss::make_note({std::string("say \"hi\" \\ end")}).json_escape();
for (std::int64_t i = 0; i < n.numel(); ++i)
  std::cout << std::string(n.at(i)) << "\n";
```

### Output

```text
say \"hi\" \\ end
```

Each `"` becomes `\"`, and the lone `\` becomes `\\`.

## Example 2: Control Characters

```cpp
const auto n =
    pluss::make_note({std::string("tab\t nl\n cr\r bell") + '\x07' + " end"}).json_escape();
for (std::int64_t i = 0; i < n.numel(); ++i)
  std::cout << std::string(n.at(i)) << "\n";
```

### Output

```text
tab\t nl\n cr\r bell\u0007 end
```

Tab, line feed, and carriage return get their short forms; the bell
(`U+0007`) has no short form, so it becomes the six characters `\u0007`.

## Example 3: Non-ASCII Passes Through

```cpp
const auto n = pluss::make_note({"café"}).json_escape();
for (std::int64_t i = 0; i < n.numel(); ++i)
  std::cout << std::string(n.at(i)) << "\n";
```

### Output

```text
café
```

## See Also

- [`Note::html_escape`](html_escape.md): escaping for HTML/XML markup instead
  of JSON.
- [`NoteTexAccessor::escape`](../note-tex/escape.md): escaping TeX special characters.
- [`Note::quote`](quote.md): wrap a cell in quotation marks.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="join_flat.html">
    <span class="label">Previous</span>
    <span class="title">join_flat</span>
  </a>
  <a class="next-link" href="keep_alnum.html">
    <span class="label">Next</span>
    <span class="title">keep_alnum</span>
  </a>
</div>
