<!-- pluss-hand-authored -->
# Note::at

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::string_view Note::at(std::int64_t linear) const
```

## Borrow One Cell By Its Flat Position

`at(linear)` is the direct, allocation-free way to inspect one cell when its
row-major position is already known. It ignores axis boundaries: a shape
`[2, 3]` Note is read as positions `0..5`, with the final coordinate changing
fastest. Use [`at_indices()`](at_indices.md) when the coordinate itself carries
meaning, and [`cell()`](cell.md) when the result must outlive every Note that
shares the source storage.

The returned `std::string_view` refers to the cell's exact stored bytes. It
does not copy, append a terminator, validate UTF-8, or stop at an embedded zero
byte. The view remains valid only while at least one owning Note or `TextRef`
keeps that storage alive. Materialize `std::string(view)` before the last owner
dies if independent lifetime is required.

`linear` must be in `[0, numel())`. Unlike [`take()`](take.md), this scalar
accessor does not normalize negative positions; `-1` is an error, not the last
cell. Undefined and zero-element Notes have no valid position. Access is
constant-time and performs no allocation or byte traversal.

## Example 1: Read A Matrix In Row-Major Order

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{
        "r0c0", "r0c1", "r0c2", "r1c0", "r1c1", "r1c2"},
    std::array<std::int64_t, 2>{2, 3});

std::cout << "at(0)=" << grid.at(0) << '\n'
          << "at(4)=" << grid.at(4) << '\n'
          << "at(5)=" << grid.at(5) << '\n';
```

### Output

```text
at(0)=r0c0
at(4)=r1c1
at(5)=r1c2
```

The shape remains `[2, 3]`; only the lookup interprets it as a flat sequence.

## Example 2: A Scalar Still Has One Cell

```cpp
const auto scalar = pluss::note::make("solo");
std::cout << scalar.at(0) << '\n';
```

### Output

```text
solo
```

A rank-zero Note has `numel() == 1`, so position zero is valid even though it
has no axes.

## Example 3: Embedded Zero Bytes Are Not Terminators

```cpp
const std::array<std::byte, 3> raw{
    std::byte{'A'}, std::byte{0}, std::byte{'B'}};
const auto view = pluss::note::bytes(raw).at(0);

std::cout << "bytes=" << view.size()
          << " values=" << static_cast<int>(static_cast<unsigned char>(view[0]))
          << ',' << static_cast<int>(static_cast<unsigned char>(view[1]))
          << ',' << static_cast<int>(static_cast<unsigned char>(view[2])) << '\n';
```

### Output

```text
bytes=3 values=65,0,66
```

Use `size()` and byte-aware consumers rather than C-string functions for raw
byte Notes.

## Example 4: Copy A Borrow Before Storage Expires

```cpp
std::string retained;
{
  const auto temporary = pluss::note::make({"alpha", "beta"});
  retained = std::string(temporary.at(1));
}
std::cout << retained << '\n';
```

### Output

```text
beta
```

For a zero-copy retained handle instead, assign `temporary.cell(1)` to a
`pluss::TextRef`.

## Example 5: Invalid Positions Reject

```cpp
const auto values = pluss::note::make({"a", "b"});
auto rejected = [](std::int64_t index) {
  try { (void)pluss::note::make({"a", "b"}).at(index); }
  catch (const pluss::Error&) {
    std::cout << "at(" << index << "): rejected\n";
  }
};
rejected(-1);
rejected(2);
```

### Output

```text
at(-1): rejected
at(2): rejected
```

## See Also

- [`at_indices`](at_indices.md): borrow by a full coordinate.
- [`cell`](cell.md): retain one flat cell with shared ownership.
- [`take`](take.md): gather several flat positions into a new Note.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align_tokens.html"><span class="label">Previous</span><span class="title">align_tokens</span></a>
  <a class="next-link" href="at_indices.html"><span class="label">Next</span><span class="title">at_indices</span></a>
</div>
