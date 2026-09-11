<!-- pluss-hand-authored -->
# Note::is_utf8_valid

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::uint8_t> Note::is_utf8_valid() const
```

## Test Whether Each Cell Is Well-Formed UTF-8

`is_utf8_valid()` reports, for each cell, whether its bytes form a valid
UTF-8 sequence — every multi-byte character correctly introduced and
continued, no stray continuation bytes, no over-long encodings. Use it
before converting a bytes-mode `Note` to text, or to find the rows in an
imported dataset that need repair.

A UTF-8-mode `Note` will normally be all `1` (its cells were built as text).
The check earns its keep on a **bytes-mode** `Note`, or after a lossy
transcode, where individual cells may hold arbitrary bytes. An empty cell is
`1`.

### Return value

A flat `std::vector<std::uint8_t>` of `0`/`1`, one per cell, row-major,
length `numel()`.

## Example 1: A Bytes-Mode `Note` With A Bad Cell

```cpp
const auto n = pluss::note::bytes({"abc", "café", "\xFF\xFE"});
const auto mask = n.is_utf8_valid();
std::cout << "[";
for (std::size_t i = 0; i < mask.size(); ++i)
  std::cout << (i ? ", " : "") << static_cast<int>(mask[i]);
std::cout << "]\n";
```

### Output

```text
[1, 1, 0]
```

`"abc"` and the valid UTF-8 bytes of `"café"` pass; `\xFF\xFE` is not
well-formed UTF-8.

## See Also

- [`Note::is_ascii`](is_ascii.md): the stronger check — ASCII only.
- [`Note::is_bytes`](is_bytes.md): whether the `Note` is in bytes mode at
  all.
- `pluss::coden::Note::to_utf8_lossy` / `to_utf8_strict`: convert bytes to
  text, replacing or rejecting invalid sequences.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_upper.html">
    <span class="label">Previous</span>
    <span class="title">is_upper</span>
  </a>
  <a class="next-link" href="is_vector.html">
    <span class="label">Next</span>
    <span class="title">is_vector</span>
  </a>
</div>
