<!-- pluss-hand-authored -->
# Note::bools

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::span<const std::uint8_t> Note::bools() const
```

## Borrow The Native Boolean Lanes

`bools()` exposes a Bool Note's contiguous lanes as a read-only borrowed span.
It is the low-level boundary for algorithms that need direct iteration, a data
pointer, or interoperability with code accepting `std::span`. The operation
does not allocate, copy, stringify, flatten the Note object, or change its
shape metadata.

Each lane is a canonical byte: `0` means false and `1` means true. The element
type is `std::uint8_t`, not C++ `bool`, so the storage is genuinely contiguous
and addressable; it does not use the packed proxy representation associated
with `std::vector<bool>`. Treat values as truth values rather than arbitrary
bytes. The returned span is const and cannot mutate the Note.

### Order And Shape

The span is always one-dimensional because it is a memory view. Lanes appear
in row-major logical order, while the original rank and extents remain on the
Note. Use `sizes()` or `size(dim)` beside the span when an external kernel also
needs shape. A scalar produces one lane; a defined empty shape produces an
empty span.

### Lifetime And Ownership

The span does not own storage. Keep the source Note, or another Note sharing
the same backing buffer, alive for every access through the span. Copying a
Note is cheap and keeps the buffer alive; copying only the span does not. Use
[`host`](host.md) when the receiving code needs an independent owning copy.

`bools()` requires a defined Bool Note. Text, Bytes, Int64, and Float64 Notes
are rejected instead of being converted. Call [`is_bool`](is_bool.md) when the
dtype is not statically known.

## Example 1: Iterate Native Truth Values

```cpp
const auto flags = pluss::note::make(std::vector<bool>{true, false});
const auto lanes = flags.bools();

std::cout << '[';
for (std::size_t i = 0; i < lanes.size(); ++i)
  std::cout << (i ? ", " : "") << (lanes[i] ? "true" : "false");
std::cout << "] size=" << lanes.size() << '\n';
```

### Output

```text
[true, false] size=2
```

## Example 2: A Matrix Is Exposed In Row-Major Order

```cpp
const auto grid = pluss::note::make(
    std::vector<bool>{true, false, false, true},
    std::array<std::int64_t, 2>{2, 2});
const auto lanes = grid.bools();

std::cout << '[';
for (std::size_t i = 0; i < lanes.size(); ++i)
  std::cout << (i ? ", " : "") << (lanes[i] ? "true" : "false");
std::cout << "] shape=[" << grid.size(0) << ", " << grid.size(1) << "]\n";
```

### Output

```text
[true, false, false, true] shape=[2, 2]
```

The span has four lanes; `[2, 2]` remains metadata on `grid`.

## Example 3: Note Copies Share The Borrowed Buffer

```cpp
const auto flags = pluss::note::make(std::vector<bool>{true, false});
const auto copy = flags;
std::cout << std::boolalpha
          << "shared=" << (flags.bools().data() == copy.bools().data()) << '\n';
```

### Output

```text
shared=true
```

Either Note keeps the buffer alive. A span alone does not.

## Example 4: Defined Empty Storage Produces An Empty Span

```cpp
const auto empty = pluss::note::make(
    std::vector<bool>{}, std::array<std::int64_t, 2>{0, 3});
std::cout << "empty lanes=" << empty.bools().size()
          << " shape=[" << empty.size(0) << ", " << empty.size(1) << "]\n";
```

### Output

```text
empty lanes=0 shape=[0, 3]
```

## Example 5: Wrong Dtype And Undefined Are Distinct Failures

```cpp
for (const auto& value : {pluss::note::make("text"),
                          pluss::Note::undefined()}) {
  try {
    (void)value.bools();
  } catch (const pluss::Error& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << '\n';
  }
}
```

### Output

```text
Note storage does not contain Bool lanes
Note::bools() on undefined Note
```

## See Also

- [`Note::is_bool`](is_bool.md): check the dtype without throwing.
- [`Note::any`](any.md) and [`Note::all`](all.md): reduce Bool lanes while
  remaining in the Note family.
- [`Note::host`](host.md): obtain owning host storage.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bool_item.html"><span class="label">Previous</span><span class="title">bool_item</span></a>
  <a class="next-link" href="capitalize.html"><span class="label">Next</span><span class="title">capitalize</span></a>
</div>
