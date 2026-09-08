<!-- pluss-hand-authored -->
# pluss::note::arange

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::arange(string_view start, string_view end,
                         std::int64_t step = 1, bool inclusive = true)
```

## A Stepped Run Of Single Characters

`arange()` builds a rank-`1` `Note` whose cells are consecutive single
characters, walking the ASCII code-point line from `start` toward `end` in
increments of `step`. It is the text analogue of `pluss::series::arange` and
of Python's `range` — a generator of an ordered sequence — specialised to
one-character strings.

Typical uses are building an alphabet or digit axis (`"a"`..`"z"`,
`"0"`..`"9"`), a header row of column letters, or a small fixed lookup of
single-character keys, without writing the run out by hand.

### Parameters

- **`start`, `end`** — each must be exactly one ASCII character (code point
  `0x00`–`0x7F`). They are read as byte values; the sequence runs over those
  values.
- **`step`** — the code-point stride. Must not be `0`. A positive `step`
  ascends from `start`; a negative `step` descends. `step` may be larger
  than `1` to skip characters (`"a"`, `"c"`, `"e"`, …).
- **`inclusive`** — whether `end` itself may appear. It can only ever appear
  when the stride lands on it exactly; when `step` steps past `end` without
  hitting it, `inclusive` makes no difference.

The result is always rank `1` and **UTF-8 mode**. If the direction of `step`
points away from `end` (ascending `step` with `start` already past `end`, or
vice versa), the sequence is empty and a zero-length `Note` is returned —
this is not an error.

Because `start` and `end` are ASCII bytes, `arange` does not walk Unicode
letters beyond ASCII; for a non-ASCII character list, build it with
[`make`](make.md) or [`from_words`](from_words.md).

## Example 1: A Contiguous Letter Run

```cpp
std::cout << pluss::note::print(pluss::note::arange("a", "e")) << "\n";
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["a", "b", "c", "d", "e"]
```

Default `step = 1`, default `inclusive = true`, so `end` (`"e"`) is
included.

## Example 2: A Stride Greater Than One

```cpp
const pluss::Note n = pluss::note::arange("a", "i", 2);
std::cout << n << "\n";
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["a", "c", "e", "g", "i"]
```

Every second character. `"i"` is reached exactly, so it appears.

## Example 3: Excluding The Endpoint

```cpp
pluss::note::print(std::cout, pluss::note::arange("a", "e", 1, false)) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["a", "b", "c", "d"]
```

`inclusive = false` drops `end` when the walk would otherwise land on it.

## Example 4: `inclusive` Is Moot When The Stride Overshoots

```cpp
std::cout << pluss::note::print(pluss::note::arange("a", "f", 2)) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["a", "c", "e"]
```

The stride goes `a, c, e, g` — it never lands on `"f"`, so `end` is absent
regardless of `inclusive`.

## Example 5: A Descending Run

```cpp
const pluss::Note n = pluss::note::arange("e", "a", -1);
std::cout << n << "\n";
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["e", "d", "c", "b", "a"]
```

A negative `step` counts down.

## Example 6: A Digit Axis With A Stride

```cpp
std::cout << pluss::note::print(pluss::note::arange("0", "9", 3)) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["0", "3", "6", "9"]
```

`arange` works over any ASCII range, not just letters.

## Example 7: A Direction Mismatch Yields An Empty `Note`

```cpp
const pluss::Note n = pluss::note::arange("e", "a");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[0], dim=1, numel=0, encoding=utf8)
[]
```

A positive `step` cannot move from `"e"` toward `"a"`, so the sequence is
empty. Not an error — a valid zero-length rank-`1` `Note`.

## Example 8: A Zero Step Is Rejected

```cpp
try {
  pluss::note::arange("a", "e", 0);
} catch (const pluss::Error& e) {
  const std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::note::arange: step cannot be 0
```

## See Also

- [`pluss::note::rand`](rand.md): random letters instead of a fixed run.
- [`pluss::note::perm`](perm.md): combinations or permutations of a word's
  characters.
- [`pluss::note::make`](make.md): write the sequence out explicitly when it
  is short or non-ASCII.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../note_factory_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Note Factory API Reference</span>
  </a>
  <a class="next-link" href="bytes.html">
    <span class="label">Next</span>
    <span class="title">bytes</span>
  </a>
</div>
