<!-- pluss-hand-authored -->
# Note::squeeze

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::squeeze(std::int64_t dim) const
```

## Drop A Size-1 Axis

`squeeze(dim)` removes axis `dim` **if its size is 1**, lowering the rank by
one. Use it to undo an [`unsqueeze`](unsqueeze.md), to peel a leading batch
axis of length 1 back off a result, or to turn a `1 × n` / `n × 1` shape
into a plain `n`.

Nothing about the text changes: cells are shared with the source, the
[bytes-vs-UTF-8](is_bytes.md) mode carries over, and only the shape metadata
is rewritten.

### The `dim` contract

- `dim` counts from the front (`0`) or, when negative, from the back
  (`-1` is the last axis). After that wrap it must land in `[0, dim())`,
  otherwise the call raises `dim out of range`.
- **If axis `dim` is not size 1, the `Note` is returned unchanged** — this is
  not an error. `squeeze` only ever removes a length-1 axis.
- On a 0-D `Note` there is no axis to address and the `Note` is returned
  unchanged.

This method never removes more than one axis and takes no "squeeze every
size-1 axis" form; call it once per axis you want gone.

## Example 1: Drop A Leading `1`

```cpp
const auto row = pluss::make_note({{"x", "y", "z"}});   // shape [1, 3]
std::cout << pluss::note::print(row.squeeze(0)) << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["x", "y", "z"]
```

## Example 2: Drop A Trailing `1`

```cpp
const auto col = pluss::make_note({"a", "b", "c", "d", "e", "f"}).reshape({6, 1});
std::cout << pluss::note::print(col.squeeze(1)) << "\n";
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["a", "b", "c", "d", "e", "f"]
```

## Example 3: A Negative Axis

```cpp
const auto row = pluss::make_note({"a", "b", "c", "d", "e", "f"}).reshape({1, 6});
std::cout << pluss::note::print(row.squeeze(-2)) << "\n";   // -2 is axis 0
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["a", "b", "c", "d", "e", "f"]
```

## Example 4: The Axis Is Not Size 1 — No Change

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.squeeze(0)) << "\n";      // axis 0 has size 2

std::cout << pluss::note::print(pluss::make_note("solo").squeeze(0)) << "\n";  // scalar
```

### Output

```text
Note(shape=[2, 3], dim=2, numel=6, encoding=utf8)
[
  ["a", "b", "c"], 
  ["d", "e", "f"]
]
Note(shape=[], dim=0, numel=1, encoding=utf8)
"solo"
```

## See Also

- [`Note::unsqueeze`](unsqueeze.md): the inverse — insert a size-1 axis.
- [`Note::reshape`](reshape.md): restate the whole shape when more than one
  axis changes.
- [`Note::flatten`](flatten.md): collapse *every* axis, not just a size-1
  one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split_whitespace.html">
    <span class="label">Previous</span>
    <span class="title">split_whitespace</span>
  </a>
  <a class="next-link" href="starts_with.html">
    <span class="label">Next</span>
    <span class="title">starts_with</span>
  </a>
</div>
