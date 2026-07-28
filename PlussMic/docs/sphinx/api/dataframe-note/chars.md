<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::chars

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::chars(std::int64_t start,
                              std::optional<std::int64_t> stop = std::nullopt,
                              std::int64_t step = 1) const -> DataFrame
```

## What It Does

Slices every selected text feature's cells the same way Python slices a
string — `chars(start, stop, step)` mirrors `s[start:stop:step]` exactly,
including negative indices (counted from the end) and a negative step
(walking the string backwards). Each cell is split into individual
characters (Unicode codepoints, same unit [`length()`](length.md) counts
in), the slice is applied per-cell using that cell's own length, and the
selected characters are rejoined into the output string.

Because the slice bounds are evaluated per-cell using each cell's own
length, cells of different lengths in the same feature can each keep a
different *number* of characters from the same `(start, stop, step)`
call — exactly as it would if you ran the equivalent Python slice
separately over each string.

## Parameters

**start** (`std::int64_t`)
    Index of the first character to include. Negative values count from the
    end of the cell (`-1` is the last character). Out-of-range values are
    clamped, never throw.

**stop** (`std::optional<std::int64_t>`, default `std::nullopt`)
    Index one past the last character to include (exclusive), or the point
    to stop just before when walking backwards. When omitted, defaults to
    "the end of the string" if `step > 0`, or "all the way to the start" if
    `step < 0` — i.e., `chars(0)` returns the whole cell and `chars(-1,
    std::nullopt, -1)` reverses it.

**step** (`std::int64_t`, default `1`)
    Stride between characters. `1` takes every character in order; `2`
    takes every other character; a negative step walks backwards. Must not
    be `0` — `chars(0, std::nullopt, 0)` throws
    `"Thread::chars: step cannot be zero"`.

## Returns

`DataFrame`
    A new frame with every selected feature's text replaced by the sliced
    substring.

## Examples

The examples below share one input frame:

```cpp
using namespace pluss::table;

auto words = DataFrame({
    {"text", Thread(pluss::make_note(
        {"hello world", "Accra", "ab", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << words;
```

### Output (before, shared by every example)

```text
==================
     | text       
------------------
0    | hello world
1    | Accra      
2    | ab         
3    | NULL       
------------------
[4 cs x 1 feat]
```

Example 1. A plain forward range: `chars(0, 5)` keeps the first five
characters of each cell (or fewer, if the cell is shorter).

```cpp
auto out1 = words.nt().chars(0, 5);
std::cout << out1;
```

### Output

```text
============
     | text 
------------
0    | hello
1    | Accra
2    | ab   
3    | NULL 
------------
[4 cs x 1 feat]
```

Row 0's first five characters are `"hello"`. Row 1 (`"Accra"`, exactly five
characters) comes back unchanged. Row 2 (`"ab"`, only two characters) is
also unchanged — `stop` is clamped to the cell's own length rather than
padding or erroring. Row 3 is invalid and stays `NULL`.

Example 2. A negative `start` counts from the end: `chars(-5)` keeps the
last five characters of each cell (with `stop` defaulted to "the end").

```cpp
auto out2 = words.nt().chars(-5);
std::cout << out2;
```

### Output

```text
============
     | text 
------------
0    | world
1    | Accra
2    | ab   
3    | NULL 
------------
[4 cs x 1 feat]
```

Row 0's last five characters are `"world"`. Row 1, exactly five characters
long, is unchanged (`-5` resolves to index 0). Row 2, only two characters
long, is also unchanged — `-5` clamps to index 0 once it's out of range.

Example 3. A negative `step` walks backwards — `chars(-1, std::nullopt,
-1)` is the idiomatic way to reverse a string with this method, exactly
like Python's `s[::-1]`.

```cpp
auto out3 = words.nt().chars(-1, std::nullopt, -1);
std::cout << out3;
```

### Output

```text
==================
     | text       
------------------
0    | dlrow olleh
1    | arccA      
2    | ba         
3    | NULL       
------------------
[4 cs x 1 feat]
```

Every valid cell comes back fully reversed, character by character
(including the space in row 0, which reverses along with the letters
around it).

Example 4. A `step` greater than 1 takes every Nth character —
`chars(0, std::nullopt, 2)` keeps every other character starting from the
first.

```cpp
auto out4 = words.nt().chars(0, std::nullopt, 2);
std::cout << out4;
```

### Output

```text
=============
     | text  
-------------
0    | hlowrd
1    | Aca   
2    | a     
3    | NULL  
-------------
[4 cs x 1 feat]
```

Row 0 keeps characters at positions 0, 2, 4, 6, 8, 10 (`h`, `l`, `o`, `w`,
`r`, `d`) — the space at position 5 is skipped since it falls on an odd
index. Row 2 (`"ab"`) keeps just position 0 (`"a"`); position 2 is past the
end of a two-character string, so the stride stops there.

Example 5. `step = 0` throws — there's no sensible stride of zero.

```cpp
try {
  auto bad = words.nt().chars(0, std::nullopt, 0);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::chars: step cannot be zero
```

## Notes

- Slices by Unicode codepoint, the same unit [`length()`](length.md)
  counts — see that page's caveat about codepoints vs. raw UTF-8 storage
  bytes.
- Out-of-range `start`/`stop` values are clamped rather than throwing —
  only `step == 0` throws.
- Bounds are evaluated per-cell against that cell's own length, so a single
  call can keep a different number of characters from cells of different
  lengths (Examples 1 and 2).
- A null cell on input stays null on output.

## See Also

- [`DataFrameNoteAccessor::length`](length.md): the character count that
  `chars()`'s implicit bounds (and any manual slicing math) are computed
  against.
- [`DataFrameNoteAccessor::pad`](pad.md): the complementary operation —
  grow a cell to a target width instead of slicing it down.
- [`DataFrameNoteAccessor::split`](split.md) / [`partition()`](partition.md):
  delimiter-based ways to break a cell into pieces, instead of
  position-based.

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
