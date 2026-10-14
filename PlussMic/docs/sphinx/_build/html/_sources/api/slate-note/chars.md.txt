<!-- pluss-hand-authored -->

# SlateNoteAccessor::chars

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signature

```cpp
SlateNoteAccessor::chars(std::int64_t start,
                          std::optional<std::int64_t> stop = std::nullopt,
                          std::int64_t step = 1) const -> Slate
```

## What It Does

Slices the bound slate's text the same way Python slices a string —
`chars(start, stop, step)` mirrors `s[start:stop:step]` exactly, including negative
indices (counted from the end) and a negative step (walking the string backwards). A
direct forward to [`Slate::chars()`](../slate/chars.md). Requires the bound slate
to be `Note`-kind. Each cell is split into individual characters (Unicode
codepoints, the same unit [`length()`](length.md) counts), the slice is applied
per-cell using that cell's own length, and the selected characters are rejoined into
the output string.

Because the slice bounds are evaluated per-cell using each cell's own length, cells
of different lengths in the same slate can each keep a different *number* of
characters from the same `(start, stop, step)` call.

## Parameters

**start** (`std::int64_t`)

Index of the first character to include. Negative values count from the end of the
cell (`-1` is the last character). Out-of-range values are clamped, never throw.

**stop** (`std::optional<std::int64_t>`, default `std::nullopt`)

Index one past the last character to include (exclusive), or the point to stop just
before when walking backwards. When omitted, defaults to "the end of the string" if
`step > 0`, or "all the way to the start" if `step < 0`.

**step** (`std::int64_t`, default `1`)

Stride between characters. `1` takes every character in order; `2` takes every other
character; a negative step walks backwards. Must not be `0`.

## Returns

`Slate`
    A new `Note`-kind slate, same length as the bound slate, holding the sliced
    text.

## Examples

Example 1. A plain forward range: `chars(0, 5)` keeps the first five characters of
each cell (or fewer, if the cell is shorter).

```cpp
using namespace pluss::table;

auto words = Slate(pluss::make_note({"hello world", "Accra", "ab", "x"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
auto out1 = words.nt().chars(0, 5);
std::cout << out1;
```

### Output

```text
0  hello
1  Accra
2  ab
3  NULL

ptype: Note
```

Row 0's first five characters are `"hello"`. Row 1 (`"Accra"`, exactly five
characters) comes back unchanged. Row 2 (`"ab"`, only two characters) is also
unchanged — `stop` is clamped to the cell's own length rather than padding or
erroring. Row 3 is invalid and stays `NULL`.

Example 2. A negative `step` reverses each cell — the idiomatic way to reverse a
string with this method, exactly like Python's `s[::-1]`.

```cpp
auto out2 = words.nt().chars(-1, std::nullopt, -1);
std::cout << out2;
```

### Output

```text
0  dlrow olleh
1  arccA
2  ba
3  NULL

ptype: Note
```

Example 3. `step = 0` throws — there's no sensible stride of zero.

```cpp
try {
  auto bad = words.nt().chars(0, std::nullopt, 0);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::chars: step cannot be zero
```

Example 4. A non-`Note` slate throws — the same message
[`Slate::chars()`](../slate/chars.md) produces directly.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().chars(0, 2);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::chars: expected Note payload
```

## Notes

- `SlateNoteAccessor::chars()` is a one-line forward to
  [`Slate::chars()`](../slate/chars.md) — identical behavior and throws.
- Slices by Unicode codepoint, the same unit [`length()`](length.md) counts.
- Out-of-range `start`/`stop` values are clamped rather than throwing — only
  `step == 0` throws.
- A null cell on input stays null on output.

## See Also

- [`Slate::chars`](../slate/chars.md): the direct, non-accessor form this method
  forwards to.
- [`SlateNoteAccessor::length`](length.md): the character count that `chars()`'s
  implicit bounds are computed against.
- [`SlateNoteAccessor::pad`](pad.md): the complementary operation — grow a cell to a
  target width instead of slicing it down.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="casefold.html">
    <span class="label">Previous</span>
    <span class="title">casefold</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
