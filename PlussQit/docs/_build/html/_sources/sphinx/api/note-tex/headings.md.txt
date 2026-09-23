<!-- pluss-hand-authored -->
# NoteTexAccessor::headings

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::headings(std::int64_t max_depth) const
```

## Extract A Bounded Document Outline

`headings(max_depth)` extracts the required title argument from structural
heading commands, up to an explicit outline depth. Commands and outer braces
are omitted; balanced content inside the title is retained. Both numbered and
starred forms are recognized, and an optional short title is skipped in favor
of the required long title because the latter is the document heading readers
see.

Depth describes hierarchy, not result count:

| `max_depth` | Included commands |
|---:|---|
| `1` | `\part`, `\chapter`, `\section` |
| `2` | depth 1 plus `\subsection` |
| `3` | depths 1–2 plus `\subsubsection` |
| `4` | depths 1–3 plus `\paragraph` |

`part`, `chapter`, and `section` share depth 1 so the same call extracts the
top-level outline from article- and book-shaped sources. The parameter must be
in `[1, 4]`; zero, negative values, and values above four are rejected. It does
not cap how many headings are returned.

Matching observes control-word boundaries. `\sectioned` is not a section, and
an escaped `\\section` does not begin a heading. Optional and required groups
must balance; malformed candidates are ignored rather than returned in part.
The method does not expand title macros, render formatting commands, infer a
hierarchy from document order, or validate whether a command is legal for a
particular document class.

The output is a `TokenBatch`: flat Text `tokens()` retain lane and source order,
while Int64 `offsets()` assign token ranges to receiver cells. Receiver rank is
not lost: a rank-two Note with four cells still has five boundaries. Undefined
input returns undefined tokens and offsets `[0]`; defined non-Text input is
rejected. Work is linear in bytes scanned and title bytes copied.

## Example 1: Select Top-Level Headings

```cpp
const auto source = pluss::make_note(
    std::vector<std::string>{
        R"(\part{Front} \chapter{Start} \section[Short]{Top {title}})",
        R"(\subsection*{Detail} \subsubsection{Deep} \paragraph{Leaf})",
        R"(\sectioned{not a heading} \\section{escaped})", "plain"},
    std::array<std::int64_t, 2>{2, 2});
const auto top = source.tex().headings(1);

std::cout << top.tokens() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Front", "Start", "Top {title}"]
```

The optional title `Short` is metadata, so the extracted title is
`Top {title}`. Nested braces remain balanced content.

## Example 2: Widen The Outline

```cpp
for (const std::int64_t depth : {2, 4}) {
  const auto outline = source.tex().headings(depth);
  std::cout << "depth=" << depth << ' ' << outline.tokens() << '\n';
}
```

### Output

```text
depth=2 Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Front", "Start", "Top {title}", "Detail"]
depth=4 Note(shape=[6], dim=1, numel=6, encoding=utf8)
["Front", "Start", "Top {title}", "Detail", "Deep", "Leaf"]
```

## Example 3: Map The Full Outline Back To A Grid

```cpp
const auto outline = source.tex().headings(4);
std::cout << "offsets=[";
const auto offsets = outline.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 3, 6, 6, 6]
```

The first row's two cells each own three headings. The second row contributes
none, but its two cells remain represented by repeated boundaries.

## Example 4: Reject Invalid Depths

```cpp
for (const std::int64_t depth : {0, 5}) {
  try {
    (void)source.tex().headings(depth);
  } catch (const pluss::Error&) {
    std::cout << "headings(" << depth << "): rejected\n";
  }
}
```

### Output

```text
headings(0): rejected
headings(5): rejected
```

## Example 5: Undefined And Invalid Dtype

```cpp
const auto absent = pluss::Note::undefined().tex().headings(2);
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().headings(2);
} catch (const pluss::Error&) {
  std::cout << "headings Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
headings Int64: rejected
```

## See Also

- `sections`: extract section titles only.
- `subsections`: extract subsection titles only.
- `subsubsections`: extract subsubsection titles only.
- [`args`](args.md): extract one chosen command's required argument.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="greek.html"><span class="label">Previous</span><span class="title">greek</span></a>
  <a class="next-link" href="ids.html"><span class="label">Next</span><span class="title">ids</span></a>
</div>
