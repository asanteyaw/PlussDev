<!-- pluss-hand-authored -->
# Note::integers

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::span<const std::int64_t> Note::integers() const
```

## Borrow The Int64 Storage Without Conversion

`integers()` returns a read-only span over an Int64 Note's native contiguous
lanes. It is intended for host algorithms, range-based loops, bulk I/O, and
foreign kernels that consume signed 64-bit integers. No allocation or copy is
performed, and values are not parsed from text or converted from Float64.

Measurements such as `lengths_chars()`, `count_substring()`, and
`regex().count()` return Int64 Notes, so their results can move through Note
shape operations before this explicit host boundary is taken. Use
`int_item()` only for a one-element result; `integers()` accepts
any rank and cardinality.

### Linear Order And Views

The returned span lists lanes in row-major logical order. Rank and extents are
not embedded in `std::span`; query them from the Note. A row or slab returned
by `matrix()` or `slabs()` can expose a smaller span into the same backing
buffer without copying. Such a Note view owns a reference to that buffer and
can outlive the source Note.

The bare span remains non-owning. Keep its Note alive, and do not retain the
span beyond that lifetime. Use [`host`](host.md) when independent ownership is
required.

`integers()` rejects every non-Int64 dtype and rejects undefined handles. A
defined zero-element Int64 Note returns an empty span.

## Example 1: Traverse A Measurement Result

```cpp
const auto counts = pluss::note::make(std::vector<std::int64_t>{2, 5});
const auto lanes = counts.integers();
std::cout << '[';
for (std::size_t i = 0; i < lanes.size(); ++i)
  std::cout << (i ? ", " : "") << lanes[i];
std::cout << "] size=" << lanes.size() << '\n';
```

### Output

```text
[2, 5] size=2
```

## Example 2: Preserve Shape Beside The Flat Span

```cpp
const auto grid = pluss::note::make(
    std::vector<std::int64_t>{1, 2, 3, 4},
    std::array<std::int64_t, 2>{2, 2});
const auto lanes = grid.integers();
std::cout << '[';
for (std::size_t i = 0; i < lanes.size(); ++i)
  std::cout << (i ? ", " : "") << lanes[i];
std::cout << "] shape=[" << grid.size(0) << ", " << grid.size(1) << "]\n";
```

### Output

```text
[1, 2, 3, 4] shape=[2, 2]
```

## Example 3: A Note Row View Keeps Shared Storage Alive

```cpp
pluss::Note row = pluss::Note::undefined();
{
  const auto matrix = pluss::note::make(
      std::vector<std::int64_t>{10, 11, 12, 20, 21, 22},
      std::array<std::int64_t, 2>{2, 3});
  row = matrix.matrix()[1];
}

const auto lanes = row.integers();
std::cout << '[';
for (std::size_t i = 0; i < lanes.size(); ++i)
  std::cout << (i ? ", " : "") << lanes[i];
std::cout << "] retained-after-source\n";
```

### Output

```text
[20, 21, 22] retained-after-source
```

The `row` Note owns a shared reference. If only `lanes` had escaped the scope,
accessing it would be invalid.

## Example 4: Dtype Mismatch Is Not An Implicit Cast

```cpp
try {
  (void)pluss::note::make(std::vector<bool>{true, false}).integers();
} catch (const pluss::Error& error) {
  const std::string message(error.what());
  std::cout << message.substr(0, message.find('\n')) << '\n';
}
```

### Output

```text
Note storage does not contain Int64 lanes
```

## Example 5: Undefined Has No Integer Buffer

```cpp
try {
  (void)pluss::Note::undefined().integers();
} catch (const pluss::Error& error) {
  const std::string message(error.what());
  std::cout << message.substr(0, message.find('\n')) << '\n';
}
```

### Output

```text
Note::integers() on undefined Note
```

## See Also

- [`Note::is_int64`](is_int64.md): test the dtype without throwing.
- [`Note::host`](host.md): materialize owning rank-aware host data.
- `Note::int_item()`: extract the sole lane from a one-element Int64 Note.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="int_item.html"><span class="label">Previous</span><span class="title">int_item</span></a>
  <a class="next-link" href="is_alnum.html"><span class="label">Next</span><span class="title">is_alnum</span></a>
</div>
