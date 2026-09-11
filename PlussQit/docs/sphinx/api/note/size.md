# Note::size

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::int64_t Note::size(std::int64_t dim) const
```

## Read One Axis Length

`size(dim)` returns the number of positions along one axis without allocating a
shape vector. It is the focused query for shape-dependent code: row count is
`size(0)` on a matrix, column count is `size(1)`, and `size(-1)` always names
the final axis.

Negative dimensions are normalized by adding `this->dim()`. Consequently, a
rank-2 Note accepts `0`, `1`, `-2`, and `-1`. Any dimension outside that set
raises `pluss::Error`; it does not return a fabricated zero. An undefined Note
also raises because it has no geometry. A legitimate zero extent is returned
normally, which lets callers distinguish shape `[2, 0]` from an invalid axis.

The query is constant-time and allocation-free. Use `sizes()` when every extent
is needed together.

## Example 1: Positive And Negative Dimensions

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d", "e", "f"},
    std::array<std::int64_t, 2>{2, 3});

std::cout << "matrix.size(0)=" << matrix.size(0) << '\n'
          << "matrix.size(1)=" << matrix.size(1) << '\n'
          << "matrix.size(-1)=" << matrix.size(-1) << '\n'
          << "matrix.size(-2)=" << matrix.size(-2) << '\n';
```

### Output

```text
matrix.size(0)=2
matrix.size(1)=3
matrix.size(-1)=3
matrix.size(-2)=2
```

## Example 2: Zero Is A Real Extent

```cpp
const auto empty = pluss::note::make(
    std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
std::cout << "rows=" << empty.size(0)
          << " columns=" << empty.size(1)
          << " cells=" << empty.numel() << '\n';
```

### Output

```text
rows=2 columns=0 cells=0
```

## Example 3: Invalid Dimensions And Absence Reject

```cpp
const auto matrix = pluss::note::make({{"a", "b"}});
const auto absent = pluss::Note::undefined();

auto rejected = [](std::string_view label, auto&& call) {
  try { call(); std::cout << label << ": accepted\n"; }
  catch (const pluss::Error&) { std::cout << label << ": rejected\n"; }
};
rejected("matrix.size(2)", [&] { (void)matrix.size(2); });
rejected("matrix.size(-3)", [&] { (void)matrix.size(-3); });
rejected("undefined.size(0)", [&] { (void)absent.size(0); });
```

### Output

```text
matrix.size(2): rejected
matrix.size(-3): rejected
undefined.size(0): rejected
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html"><span class="label">Previous</span><span class="title">select</span></a>
  <a class="next-link" href="sizes.html"><span class="label">Next</span><span class="title">sizes</span></a>
</div>
<!-- pluss-hand-authored -->
