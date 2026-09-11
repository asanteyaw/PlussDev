<!-- pluss-hand-authored -->
# Note::html_unescape

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::html_unescape() const
```

## Resolve HTML Entity References Back To Characters

`html_unescape()` is the inverse of [`html_escape`](html_escape.md): it scans
each cell for `&…;` entity references and replaces the ones it recognizes
with the characters they denote. Use it to recover plain text from a value
that was previously escaped, or that arrived from an HTML source.

### What is recognized

- The named references `&amp;` `&lt;` `&gt;` `&quot;` `&apos;` — and `&#39;`,
  which `html_escape` emits for `'`.
- **Numeric character references**: decimal `&#N;` and hexadecimal `&#xH;`
  (case-insensitive `x`), for any code point up to `U+10FFFF`. The code point
  is encoded as UTF-8 in the result, so `&#8364;` and `&#x20AC;` both become
  `€` and `&#x1F600;` becomes `😀`.

This is a deliberately small set — the five markup characters plus numeric
escapes — not the full HTML5 named-character-reference table. An entity the
method does not know (`&copy;`, `&nbsp;`, a misspelling) is **left exactly as
written**, and a bare `&` with no `;` is also left untouched. Nothing is
dropped, so unknown markup passes through rather than disappearing.

The operation works on UTF-8 text and bytes-mode `Note`s, preserves shape,
keeps the encoding tag, and returns a fresh `Note`.

## Example 1: Named References

```cpp
std::cout << pluss::make_note({"a &amp; b &lt;c&gt; &quot;q&quot; &#39;x&apos;"})
                 .html_unescape()
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["a & b <c> \"q\" 'x'"]
```

(The `\"` in the printed form is the `Note` printer escaping the double quote;
the cell contains a literal `"`.)

## Example 2: Numeric References, Decimal And Hex

```cpp
std::cout << pluss::make_note({"&#65;&#x41; &#8364; &#x1F600;"}).html_unescape() << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["AA € 😀"]
```

`&#65;` and `&#x41;` both resolve to `A`.

## Example 3: Unknown Entities Survive Verbatim

```cpp
std::cout << pluss::make_note({"&foo; &amp &notreal;"}).html_unescape() << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["&foo; &amp &notreal;"]
```

`&foo;` and `&notreal;` are not recognized; `&amp` has no semicolon. All
three are left as-is.

## Example 4: Round Trip With `html_escape`

```cpp
std::cout << pluss::make_note({"a & b < c > d \" e ' f"})
                 .html_escape()
                 .html_unescape()
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["a & b < c > d \" e ' f"]
```

## See Also

- [`Note::html_escape`](html_escape.md): the inverse.
- [`Note::url_decode`](url_decode.md) / [`Note::base64_decode`](base64_decode.md):
  the other transport decoders.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="html_escape.html">
    <span class="label">Previous</span>
    <span class="title">html_escape</span>
  </a>
  <a class="next-link" href="indent.html">
    <span class="label">Next</span>
    <span class="title">indent</span>
  </a>
</div>
