<!-- pluss-hand-authored -->
# Note::join_flat

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::string Note::join_flat(std::string_view separator = " ") const
```

## Materialize One Standard String

`join_flat()` copies all cells in row-major order into an owning
`std::string`. It is the direct bridge to a standard C++ consumer that wants
one contiguous character or byte sequence rather than another `Note`. Its
default separator is one ASCII space; pass `""` to concatenate without a
boundary or another string to choose the boundary explicitly.

The method is shape-agnostic: scalars, vectors, matrices, and tensors are all
read as the same flat sequence used by `at(linear)` and `vector()`. The returned
string owns its bytes and remains valid after the source Note is destroyed.
Embedded zero bytes are preserved because `std::string` and `std::string_view`
are length-aware.

Unlike [`join()`](join.md), `join_flat()` accepts both UTF-8 and bytes-mode
Notes and returns no encoding tag. This makes it appropriate for assembling a
binary payload, but the caller must remember whether the resulting string is
text or arbitrary bytes. Zero cells yield an empty string. Undefined Notes
reject because a standard string has no representation for an absent Note.

The method performs one result allocation strategy and copies `O(total cell
bytes + separator bytes)`. It does not mutate or decode the source.

## Example 1: Use The Default Space

```cpp
const auto words = pluss::note::make({"red", "green", "blue"});
std::cout << "default=" << words.join_flat() << '\n';
```

### Output

```text
default=red green blue
```

## Example 2: Choose A Separator And Flatten A Matrix

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << "custom=" << grid.join_flat("/") << '\n';
```

### Output

```text
custom=a/b/c/d
```

Passing `""` is the standard-string counterpart of calling `join()` with no
argument.

## Example 3: The Result Owns Its Bytes

```cpp
std::string retained;
{
  retained = pluss::note::make({"left", "right"}).join_flat("+");
}
std::cout << "lifetime=" << retained << '\n';
```

### Output

```text
lifetime=left+right
```

## Example 4: Preserve Binary Zeros

```cpp
const auto raw = pluss::note::bytes(
    std::vector<std::string>{std::string("A\0", 2), "B"},
    std::array<std::int64_t, 1>{2});
const auto joined = raw.join_flat("|");

std::cout << "raw size=" << joined.size() << " values="
          << static_cast<int>(static_cast<unsigned char>(joined[0])) << ','
          << static_cast<int>(static_cast<unsigned char>(joined[1])) << ','
          << static_cast<int>(static_cast<unsigned char>(joined[2])) << ','
          << static_cast<int>(static_cast<unsigned char>(joined[3])) << '\n';
```

### Output

```text
raw size=4 values=65,0,124,66
```

## Example 5: Zero Cells Yield An Empty String

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{},
    std::array<std::int64_t, 2>{2, 0});
std::cout << "zero cells bytes=" << empty.join_flat(",").size() << '\n';
```

### Output

```text
zero cells bytes=0
```

## Example 6: Undefined Rejects

```cpp
try { (void)pluss::Note::undefined().join_flat(","); }
catch (const pluss::Error&) {
  std::cout << "join_flat undefined: rejected\n";
}
```

### Output

```text
join_flat undefined: rejected
```

## See Also

- [`join`](join.md): return a scalar UTF-8 `Note`; default to no separator.
- [`host`](host.md): copy cells separately while retaining rank metadata.
- [`vector`](vector.md): allocate only borrowed views, not copied bytes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="join.html"><span class="label">Previous</span><span class="title">join</span></a>
  <a class="next-link" href="json_escape.html"><span class="label">Next</span><span class="title">json_escape</span></a>
</div>
