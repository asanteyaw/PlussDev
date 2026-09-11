<!-- pluss-hand-authored -->
# Note::sizes_view

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::span<const std::int64_t> Note::sizes_view() const
```

## Inspect Geometry Without Allocating

`sizes_view()` returns a read-only span over the receiver's extents. Its length
equals `dim()`, and entry `d` equals `size(d)`. A scalar therefore returns a
valid empty span, a vector returns one extent, and a rank-three Note returns
three. Zero extents are retained exactly, so shape `[2, 0]` remains distinguishable
from `[0, 2]` even though both Notes contain no cells.

The span points into the Note's implementation object. Calling the method is
constant-time and allocation-free, but the span does not own that object. Do
not retain it after the last Note sharing the implementation is destroyed or
reassigned. Use [`sizes()`](sizes.md) when the extents need independent
lifetime or mutation; that method returns a copied `std::vector<std::int64_t>`.

The elements are `const`, so callers cannot corrupt Note geometry through the
view. Repeated calls on an unchanged Note refer to the same extent storage.
Neither operation inspects text bytes. An undefined Note rejects rather than
returning an empty span, because an empty span is already the valid shape of a
defined scalar.

## Example 1: Scalar Shape Is An Empty Span

```cpp
const auto shape = pluss::note::make("alpha").sizes_view();
std::cout << "scalar=[] size=" << shape.size() << '\n';
```

### Output

```text
scalar=[] size=0
```

The empty span means rank zero, not absence and not zero cells.

## Example 2: Read Matrix Extents

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d", "e", "f"},
    std::array<std::int64_t, 2>{2, 3});
const auto shape = grid.sizes_view();
std::cout << "grid=[" << shape[0] << ", " << shape[1] << "]\n";
```

### Output

```text
grid=[2, 3]
```

## Example 3: Zero Extents Remain Visible

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{},
    std::array<std::int64_t, 2>{2, 0});
const auto shape = empty.sizes_view();
std::cout << "empty=[" << shape[0] << ", " << shape[1] << "]\n";
```

### Output

```text
empty=[2, 0]
```

## Example 4: Choose Borrowed Or Independent Geometry

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d", "e", "f"},
    std::array<std::int64_t, 2>{2, 3});
const auto view = grid.sizes_view();
const auto copy = grid.sizes();

std::cout << std::boolalpha
          << "view data stable=" << (view.data() == grid.sizes_view().data())
          << " copy independent=" << (copy.data() != view.data()) << '\n';
```

### Output

```text
view data stable=true copy independent=true
```

Use the span for immediate iteration or API calls that consume a span. Use the
vector when storing shape beyond the receiver's lifetime.

## Example 5: Undefined Has No Geometry

```cpp
try { (void)pluss::Note::undefined().sizes_view(); }
catch (const pluss::Error&) {
  std::cout << "sizes_view undefined: rejected\n";
}
```

### Output

```text
sizes_view undefined: rejected
```

## See Also

- [`sizes`](sizes.md): return an owning, mutable copy of all extents.
- [`size`](size.md): query one axis with negative-axis normalization.
- [`dim`](dim.md): return the number of extents.
- [`vector`](vector.md): flatten cells while discarding their geometry.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sizes.html"><span class="label">Previous</span><span class="title">sizes</span></a>
  <a class="next-link" href="sort.html"><span class="label">Next</span><span class="title">sort</span></a>
</div>
