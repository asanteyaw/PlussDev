<!-- pluss-hand-authored -->
# Note::remove_control_chars

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::remove_control_chars() const
```

## Delete Control Bytes, Keeping Tab And Newline

`remove_control_chars()` returns a new `Note`, same shape as the input, with
the ASCII control bytes deleted from each cell — **except** tab (`\t`),
newline (`\n`), and carriage return (`\r`), which are kept. The `DEL` byte
(`0x7F`) is removed. Use it to sanitize text of stray control characters —
bell, backspace, escape, NUL-adjacent bytes — that would corrupt a terminal
or a log while preserving normal layout.

Non-ASCII bytes are **not** touched, so accented and non-Latin text passes
through unchanged. Only the removed bytes disappear; nothing is trimmed or
collapsed.

`remove_control_chars` is shape-preserving, cell-independent, and returns a
fresh `Note`. Use [`keep_printable`](keep_printable.md) instead when tab and
newline should also go, or [`keep_ascii`](keep_ascii.md) when non-ASCII
should go too.

## Example 1: A Batch

```cpp
const std::string s = std::string("alert") + '\x07' + "text\tkept\nkept café";
std::cout << pluss::note::print(
    pluss::make_note({s, "clean"}).remove_control_chars())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["alerttext\tkept\nkept café", "clean"]
```

The bell (`\x07`) is gone; the tab, the newline, and the `é` all remain.

## See Also

- [`Note::keep_printable`](keep_printable.md): also removes tab, newline, and
  non-ASCII.
- [`Note::keep_ascii`](keep_ascii.md): keeps ASCII controls, removes only
  non-ASCII.
- [`Note::collapse_whitespace`](collapse_whitespace.md): normalise the
  spacing that remains.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remove_all.html">
    <span class="label">Previous</span>
    <span class="title">remove_all</span>
  </a>
  <a class="next-link" href="remove_digits.html">
    <span class="label">Next</span>
    <span class="title">remove_digits</span>
  </a>
</div>
