<!-- pluss-hand-authored -->
# Note::for_each_flat

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
template<class Fn>
  requires std::invocable<Fn&, std::string_view>
void Note::for_each_flat(Fn&& fn) const
```

## Visit Every Text Or Byte Cell

`for_each_flat()` invokes `fn(cell)` once for each Text or Bytes cell. The
argument is a borrowed `std::string_view`; no cell string is allocated or
decoded. Use the method when a callback expresses the work more directly than
an explicit iterator loop: accumulating metrics, feeding a parser, writing to
a stream, hashing cells, or copying into an external representation.

Traversal is flat and row-major for every rank. A scalar invokes the callback
once. A defined empty Note invokes it zero times. The method rejects an
undefined handle and native Bool, Int64, and Float64 Notes before invoking the
callback at all.

The callable may capture state and may return any value; return values are
discarded. If it throws, traversal stops immediately and the same exception
propagates to the caller. Effects already performed by earlier invocations are
not rolled back. That behavior makes early termination possible, but callers
must provide their own transactional boundary when partial external writes
would be unsafe.

`for_each_flat()` itself allocates nothing and performs one indirect callable
invocation per cell. A direct range-based loop can be preferable in a very hot
path when the compiler can inline the loop body more clearly; both traverse
the same packed arena and have `O(numel())` time.

## Example 1: Aggregate Without Materializing A Vector

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{"r0c0", "r0c1", "r0c2", "r1c0", "r1c1", "r1c2"},
    std::array<std::int64_t, 2>{2, 3});
std::size_t cells = 0;
std::size_t bytes = 0;
grid.for_each_flat([&](std::string_view cell) {
  ++cells;
  bytes += cell.size();
});
std::cout << "cells=" << cells << " bytes=" << bytes << '\n';
```

### Output

```text
cells=6 bytes=24
```

The callback sees six cells, not two row Notes.

## Example 2: Stream Cells In Logical Order

```cpp
const auto words = pluss::note::make({"north", "east", "south", "west"});
bool first = true;
words.for_each_flat([&](std::string_view word) {
  std::cout << (first ? "" : " | ") << word;
  first = false;
});
std::cout << '\n';
```

### Output

```text
north | east | south | west
```

## Example 3: Stop By Throwing

```cpp
const auto words = pluss::note::make({"north", "east", "south", "west"});
try {
  words.for_each_flat([](std::string_view cell) {
    if (cell == "south") throw std::runtime_error("stop at south");
  });
} catch (const std::runtime_error& error) {
  std::cout << "callback error=" << error.what() << '\n';
}
```

### Output

```text
callback error=stop at south
```

This is exception propagation, not a hidden boolean-break protocol. Use an
explicit iterator loop when ordinary `break` is clearer.

## Example 4: Empty Notes Make No Calls

```cpp
const auto empty = pluss::note::make(std::vector<std::string>{});
std::size_t calls = 0;
empty.for_each_flat([&](std::string_view) { ++calls; });
std::cout << "empty calls=" << calls << '\n';
```

### Output

```text
empty calls=0
```

## Example 5: Undefined Is Not A Defined Empty Note

```cpp
try {
  pluss::Note::undefined().for_each_flat([](std::string_view) {});
} catch (const pluss::Error&) {
  std::cout << "undefined callback: rejected\n";
}
```

### Output

```text
undefined callback: rejected
```

## Example 6: Traverse Native Values Natively

```cpp
const auto values = pluss::note::make(std::vector<std::int64_t>{3, 1, 4});
std::cout << "integers=";
for (const auto value : values.integers()) std::cout << value << ' ';
std::cout << '\n';
```

### Output

```text
integers=3 1 4 
```

Using `integers()` avoids both textual formatting and variant dispatch.

## See Also

- [`begin`](begin.md): write a loop with `break`, standard algorithms, or iterator state.
- [`copy_flat_to`](copy_flat_to.md): forward each view to an output iterator.
- [`vector`](vector.md): allocate one host vector of borrowed views.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="float_item.html"><span class="label">Previous</span><span class="title">float_item</span></a>
  <a class="next-link" href="from_latin1.html"><span class="label">Next</span><span class="title">from_latin1</span></a>
</div>
