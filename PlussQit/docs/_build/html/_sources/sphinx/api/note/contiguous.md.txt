<!-- pluss-hand-authored -->
# Note::contiguous

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::contiguous() const
```

## Always Already Contiguous

`contiguous()` returns the same `Note`, unchanged. A `Note` stores its cells
as a single flat offset table in row-major order, so there is no such thing
as a non-contiguous `Note` to repack — every `Note`, including one produced
by [`transpose`](transpose.md), [`permute`](permute.md),
[`select`](../../note_api_reference.md), or [`narrow`](../../note_api_reference.md),
already has its cells laid out in reading order.

The method exists for **parity** with array APIs where a view can be strided
and a later step needs a dense copy. Writing `note.contiguous()` in code
ported from such an API is harmless — it costs nothing and changes nothing —
but it is never *required* on a `Note`.

## Example: A No-Op, Even After A Transpose

```cpp
const auto m = pluss::make_note({{"a", "b", "c"}, {"d", "e", "f"}});
std::cout << pluss::note::print(m.transpose(0, 1).contiguous()) << "\n";
```

### Output

```text
Note(shape=[3, 2], dim=2, numel=6, encoding=utf8)
[
  ["a", "d"], 
  ["b", "e"], 
  ["c", "f"]
]
```

The transposed `Note` is returned exactly as `transpose` produced it —
`contiguous()` adds nothing.

## See Also

- [`Note::transpose`](transpose.md) / [`Note::permute`](permute.md): axis
  reorders whose results are already contiguous.
- [`Note::reshape`](reshape.md): change the shape of those same cells.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contains_any_digit.html">
    <span class="label">Previous</span>
    <span class="title">contains_any_digit</span>
  </a>
  <a class="next-link" href="count_substring.html">
    <span class="label">Next</span>
    <span class="title">count_substring</span>
  </a>
</div>
