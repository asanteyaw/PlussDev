<!-- pluss-hand-authored -->
# Note::html_escape

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::html_escape() const
```

## Escape The Five Markup-Significant Characters

`html_escape()` rewrites each cell so it can be dropped into HTML or XML text
without being parsed as markup. It replaces exactly five characters with
their entity references:

| Character | Replacement |
|-----------|-------------|
| `&`       | `&amp;`     |
| `<`       | `&lt;`      |
| `>`       | `&gt;`      |
| `"`       | `&quot;`    |
| `'`       | `&#39;`     |

Escaping `"` and `'` as well as the three structural characters means the
result is also safe inside a single- or double-quoted attribute value, not
only in element content.

Every other byte is copied through untouched — letters, digits, whitespace,
and **all non-ASCII UTF-8** (`café — €` is unchanged). The operation works on
UTF-8 text and on [bytes](is_bytes.md)-mode `Note`s alike, preserves shape,
keeps the input's encoding tag, and returns a fresh `Note`.

`html_escape()` is **not idempotent**: applying it twice escapes the `&` in
the entities it produced (`<a>` → `&lt;a&gt;` → `&amp;lt;a&amp;gt;`), so
escape exactly once, at the point of output. The inverse is
[`html_unescape`](html_unescape.md).

## Example 1: The Five Characters, In 1-D

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"a & b < c > d \" e ' f", "plain"}).html_escape())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["a &amp; b &lt; c &gt; d &quot; e &#39; f", "plain"]
```

A cell with none of the five characters (`"plain"`) is returned as-is.

## Example 2: Non-ASCII Is Left Alone

```cpp
std::cout << pluss::make_note({"café — €"}).html_escape() << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["café — €"]
```

## Example 3: Shape Is Preserved

```cpp
std::cout << pluss::note::print(
    pluss::make_note({{"<x>", "y&z"}}).html_escape())
    << "\n";
```

### Output

```text
Note(shape=[1, 2], dim=2, numel=2, encoding=utf8)
[
  ["&lt;x&gt;", "y&amp;z"]
]
```

## Example 4: Escaping Twice Double-Escapes

```cpp
std::cout << pluss::make_note({"<a>"}).html_escape().html_escape() << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["&amp;lt;a&amp;gt;"]
```

## See Also

- [`Note::html_unescape`](html_unescape.md): the inverse.
- [`Note::json_escape`](json_escape.md): escaping for JSON string bodies
  instead of markup.
- [`Note::tex_escape`](tex_escape.md): escaping TeX special characters.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="host.html">
    <span class="label">Previous</span>
    <span class="title">host</span>
  </a>
  <a class="next-link" href="html_unescape.html">
    <span class="label">Next</span>
    <span class="title">html_unescape</span>
  </a>
</div>
