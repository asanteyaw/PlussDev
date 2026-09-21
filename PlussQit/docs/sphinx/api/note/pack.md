# Note::pack

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<std::byte> Note::pack() const
```

## Encode The Complete Note State

`pack()` serializes a Note into an owning, versioned byte vector. It is the
closed Note boundary for caches, files, messages, and other systems that carry
bytes rather than C++ objects. The inverse factory is
[`pluss::note::unpack()`](../note_factory/unpack.md).

The encoding preserves more than displayed values: defined state, dtype, rank,
every extent, flat lane order, embedded zero bytes, and exact native numeric
bits all survive a round trip. A rank-2 empty Note therefore remains rank 2,
and `-0.0` does not become `0.0`. Text and Bytes remain different dtypes even
when their lane bytes happen to match.

Version 1 begins with a 40-byte header. Its `PNOT` magic, version, dtype, rank,
flags, element count, and three extent slots are encoded in a defined
little-endian layout. Text and Bytes payloads are length-delimited; Bool uses
canonical zero/one bytes; Int64 and Float64 store exact 64-bit patterns. Callers
should treat this as a format contract and use `unpack()` rather than manually
decoding or constructing envelopes.

The returned vector owns its bytes and is independent of the Note. Encoding is
`O(numel + payload bytes)` and necessarily allocates the complete envelope.
`pack()` performs no compression, encryption, authentication, or text
normalization. Apply those concerns around the returned bytes when persistence
or transport requires them.

## Example 1: Inspect A Matrix Envelope And Round Trip It

```cpp
const auto source = pluss::note::make(
    std::vector<std::int64_t>{-1, 0, 7, 9},
    std::array<std::int64_t, 2>{2, 2});
const auto encoded = source.pack();

std::cout << "magic=";
for (std::size_t i = 0; i < 4; ++i)
  std::cout << static_cast<char>(std::to_integer<unsigned char>(encoded[i]));
std::cout << " version=" << std::to_integer<unsigned>(encoded[4])
          << " rank=" << std::to_integer<unsigned>(encoded[6])
          << " bytes=" << encoded.size() << '\n';
std::cout << "restored=" << pluss::note::unpack(encoded) << '\n';
```

### Output

```text
magic=PNOT version=1 rank=2 bytes=72
restored=Note(shape=[2, 2], dim=2, numel=4, dtype=Int64)
[
  [-1, 0], 
  [7, 9]
]
```

The 32-byte payload contains four exact Int64 lanes after the 40-byte header.

## Example 2: Undefined Is A Serializable State

```cpp
const auto encoded = pluss::Note::undefined().pack();
const auto restored = pluss::note::unpack(encoded);

std::cout << std::boolalpha
          << "undefined bytes=" << encoded.size()
          << " restored-defined=" << restored.defined() << '\n';
```

### Output

```text
undefined bytes=40 restored-defined=false
```

Undefined is not encoded as an empty Text Note. Its state is explicit in the
header.

## Example 3: Empty Geometry Keeps Its Rank

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
const auto encoded = empty.pack();
const auto restored = pluss::note::unpack(encoded);

std::cout << "empty bytes=" << encoded.size()
          << " shape=[" << restored.size(0) << ", " << restored.size(1)
          << "]\n";
```

### Output

```text
empty bytes=40 shape=[2, 0]
```

No payload is needed, but the header still carries Text dtype and both extents.

## Example 4: Floating-Point Bits Are Not Reformatted

```cpp
const auto source = pluss::note::make(std::vector<double>{
    -0.0, std::bit_cast<double>(UINT64_C(0x7ff8000000000000))});
const auto restored = pluss::note::unpack(source.pack());

std::cout << std::hex << std::setfill('0')
          << "float bits=0x" << std::setw(16)
          << std::bit_cast<std::uint64_t>(restored.reals()[0])
          << ",0x" << std::setw(16)
          << std::bit_cast<std::uint64_t>(restored.reals()[1]) << '\n';
```

### Output

```text
float bits=0x8000000000000000,0x7ff8000000000000
```

This is why `pack()` is suitable for exact Note round trips while printed text
is not.

## Failure And Trust Boundary

`pack()` validates that the in-memory Note has a supported closed dtype and
geometry before writing. `unpack()` performs the adversarial checks: bad magic,
unknown versions or dtype tags, inconsistent shapes, truncated payloads,
invalid Bool lanes, overflow, and trailing bytes are rejected with
`pluss::Error`. A successful decode owns fresh Note storage and does not borrow
the encoded span.

## See Also

- [`pluss::note::unpack`](../note_factory/unpack.md): validate and reconstruct a packed Note.
- [`debug`](debug.md): bounded human-readable diagnostics, not serialization.
- [`host`](host.md): explicit host extraction for direct C++ consumption.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="owned.html"><span class="label">Previous</span><span class="title">owned</span></a>
  <a class="next-link" href="permute.html"><span class="label">Next</span><span class="title">permute</span></a>
</div>
<!-- pluss-hand-authored -->
