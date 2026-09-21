<!-- pluss-hand-authored -->
# pluss::note::unpack

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signature

```cpp
Note pluss::note::unpack(std::span<const std::byte> encoded)
```

## Reconstruct A Packed Note Exactly

`unpack()` decodes the stable binary envelope produced by `Note::pack()` and
constructs a new owning `Note`. It is the inverse of `pack()`: defined state,
dtype, rank, every extent, lane order, text and byte lengths, and native numeric
bit patterns survive the round trip.

Use it when a packed Note has crossed a file, cache, message, or other
byte-oriented boundary and should re-enter the closed Note API. It is not a
general text parser and does not accept JSON, CSV, a raw memory dump, or an
arbitrary byte `Note`. The input must be one complete `pack()` envelope.

The parameter is a read-only `std::span<const std::byte>`. A
`std::vector<std::byte>` returned by `pack()` converts directly to that span;
subspans and fixed byte arrays can also be passed. Decoding always copies or
moves decoded content into fresh Note-owned storage. The result does not borrow
the input buffer and remains valid after that buffer is destroyed.

## What Version 1 Preserves

| Property | Decoding contract |
|---|---|
| Definition state | An undefined Note round-trips as undefined |
| Dtype | Text, Bytes, Bool, Int64, and Float64 remain distinct |
| Shape | Rank zero through three and zero extents are preserved |
| Text and Bytes | Each cell's explicit byte length is preserved, including embedded zero bytes |
| Bool | Only canonical lanes `0` and `1` are accepted |
| Int64 | The exact signed 64-bit pattern is restored |
| Float64 | The exact IEEE bit pattern is restored, including signed zero, infinities, and NaN payloads |

The envelope begins with the four-byte magic `PNOT`, a format version, dtype,
rank, flags, element count, and three extent slots. Fixed-width integers are
stored little-endian. Version 1 uses a 40-byte header; payload size then depends
on dtype and content. Treat those details as a persistence contract, not as an
invitation to construct envelopes manually: use `pack()` so flags, cardinality,
and lane encoding remain canonical.

## Validation And Failure Atomicity

`unpack()` validates the complete envelope before returning a usable value. It
rejects:

- truncated headers, length tables, cells, or numeric payloads
- an invalid magic or unsupported format version
- unknown dtype tags or nonzero unsupported flags
- rank greater than three or nonzero unused extent slots
- element counts inconsistent with the shape
- shape multiplication that overflows Int64
- cell lengths outside addressable memory
- Bool bytes other than `0` and `1`
- any trailing byte after the expected payload

Every rejection throws `pluss::Error`; no partial Note escapes. Unsupported
future versions fail explicitly rather than being interpreted as version 1.

Decoding is `O(encoded.size())` and allocates storage proportional to the
decoded Note. Text and Bytes additionally build cell boundaries. The operation
does not compress, encrypt, authenticate, normalize, transcode, or validate
UTF-8. Apply transport security and integrity checks outside this function when
the byte stream crosses a trust boundary.

## Example 1: The Ordinary Pack/Unpack Round Trip

```cpp
const auto source = pluss::note::make({"draft", "review", "publish"});
const std::vector<std::byte> encoded = source.pack();
const pluss::Note restored = pluss::note::unpack(encoded);

std::cout << restored << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["draft", "review", "publish"]
```

The vector converts to the span parameter without exposing its storage to the
returned Note.

## Example 2: Every Closed Dtype And Matrix Shape

```cpp
const std::array<std::int64_t, 2> shape{2, 2};
const std::vector<pluss::Note> sources{
    pluss::note::bytes(std::vector<std::string>{"a", "b", "c", "d"}, shape),
    pluss::note::make(std::vector<bool>{true, false, false, true}, shape),
    pluss::note::make(std::vector<std::int64_t>{-1, 0, 7, 9}, shape),
    pluss::note::make(std::vector<double>{-0.0, 1.25, 2.5, 4.0}, shape),
};

for (const auto& source : sources) {
  const auto restored = pluss::note::unpack(source.pack());
  std::cout << pluss::dtype_name(restored.dtype())
            << " shape=[" << restored.size(0) << ", " << restored.size(1)
            << "]\n";
}
```

### Output

```text
Bytes shape=[2, 2]
Bool shape=[2, 2]
Int64 shape=[2, 2]
Float64 shape=[2, 2]
```

No dtype is converted through a textual representation.

## Example 3: Undefined Is A Serializable State

```cpp
const auto restored = pluss::note::unpack(pluss::Note::undefined().pack());
std::cout << std::boolalpha << "defined=" << restored.defined() << '\n';
```

### Output

```text
defined=false
```

This differs from a defined empty Note, whose dtype and shape remain present.

## Example 4: Empty Geometry Retains Rank And Extents

```cpp
const auto source = pluss::note::make(
    std::vector<bool>{}, std::array<std::int64_t, 2>{0, 5});
const auto encoded = source.pack();
const auto restored = pluss::note::unpack(encoded);

std::cout << "dtype=" << pluss::dtype_name(restored.dtype())
          << " shape=[" << restored.size(0) << ", " << restored.size(1)
          << "] numel=" << restored.numel()
          << " bytes=" << encoded.size() << '\n';
```

### Output

```text
dtype=Bool shape=[0, 5] numel=0 bytes=40
```

An empty matrix does not collapse into a vector or undefined Note.

## Example 5: Embedded Zero Bytes Are Length-Preserving

```cpp
const std::string cell{"a\0b", 3};
const auto source = pluss::note::make(std::vector<std::string>{cell, "tail"});
const auto restored = pluss::note::unpack(source.pack());

std::cout << "length=" << restored.at(0).size()
          << " middle=" << static_cast<int>(restored.at(0)[1])
          << " second=" << restored.at(1) << '\n';
```

### Output

```text
length=3 middle=0 second=tail
```

Cells are length-delimited; decoding never treats a zero byte as a terminator.

## Example 6: Floating-Point Bits Are Exact

```cpp
const auto source = pluss::note::make(std::vector<double>{
    -0.0, std::numeric_limits<double>::quiet_NaN()});
const auto restored = pluss::note::unpack(source.pack());

std::cout << std::hex << std::showbase
          << "negzero=" << std::bit_cast<std::uint64_t>(restored.reals()[0])
          << " nan=" << std::bit_cast<std::uint64_t>(restored.reals()[1])
          << '\n';
```

### Output

```text
negzero=0x8000000000000000 nan=0x7ff8000000000000
```

Printing decimal values would hide the signed-zero distinction, so this
example inspects the restored bit patterns directly.

## Example 7: The Result Owns Its Storage

```cpp
auto restored = pluss::Note::undefined();
{
  const auto temporary = pluss::note::make("alpha").pack();
  restored = pluss::note::unpack(temporary);
}
std::cout << "owned after buffer: " << restored.scalar() << '\n';
```

### Output

```text
owned after buffer: alpha
```

Destroying the encoded vector does not invalidate the decoded Note.

## Example 8: Inspect The Envelope Identity

```cpp
const auto encoded = pluss::note::make({true, false}).pack();
std::cout << "magic=";
for (std::size_t i = 0; i < 4; ++i)
  std::cout << std::to_integer<char>(encoded[i]);
std::cout << " version=" << std::to_integer<unsigned>(encoded[4])
          << " rank=" << std::to_integer<unsigned>(encoded[6])
          << " bytes=" << encoded.size() << '\n';
```

### Output

```text
magic=PNOT version=1 rank=1 bytes=42
```

The 40-byte header plus two Bool lanes produces 42 bytes.

## Example 9: Corrupt And Noncanonical Inputs Are Rejected

```cpp
const auto valid = pluss::note::make({true, false}).pack();

auto check = [](std::string_view label, const auto& bytes) {
  try {
    (void)pluss::note::unpack(bytes);
    std::cout << label << ": accepted\n";
  } catch (const pluss::Error&) {
    std::cout << label << ": rejected\n";
  }
};

auto bad_magic = valid;
bad_magic[0] = std::byte{0};
check("bad magic", bad_magic);

auto bad_version = valid;
bad_version[4] = std::byte{99};
check("bad version", bad_version);

auto bad_bool = valid;
bad_bool.back() = std::byte{2};
check("noncanonical bool", bad_bool);

auto trailing = valid;
trailing.push_back(std::byte{0});
check("trailing byte", trailing);

check("truncated",
      std::span<const std::byte>(valid.data(), valid.size() - 1));
```

### Output

```text
bad magic: rejected
bad version: rejected
noncanonical bool: rejected
trailing byte: rejected
truncated: rejected
```

## See Also

- `Note::pack()`: produce the only envelope accepted by this factory.
- [`pluss::note::make`](make.md): construct from typed values already in memory.
- [`pluss::note::bytes`](bytes.md): construct a Bytes Note from raw cell data.
- [`Note::host`](../note/host.md): copy a Note into ordinary typed C++ containers.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html"><span class="label">Previous</span><span class="title">select</span></a>
  <a class="next-link" href="../../note_factory_api_reference.html"><span class="label">Next</span><span class="title">Note Factory</span></a>
</div>
