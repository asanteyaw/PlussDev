<!-- pluss-hand-authored -->
# Note::is_int64

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
bool Note::is_int64() const noexcept
```

## Recognize Native Integer Notes

`is_int64()` is true when every lane uses signed 64-bit integer storage. It
does not parse text, round floating-point lanes, or inspect whether values are
mathematically integral. Measurements such as character lengths and regex
counts return this dtype so their shape remains inside the Note family.

After a successful check, `integers()` gives a borrowed contiguous
`std::span<const std::int64_t>`. The query itself is constant-time,
allocation-free, independent of rank and shape, and `noexcept`. Undefined
returns `false`.

## Example 1: Separate Integer And Floating Storage

```cpp
const auto counts = pluss::note::make(std::vector<std::int64_t>{2, 5});
const auto floats = pluss::note::make(std::vector<double>{2.0, 5.0});
const auto absent = pluss::Note::undefined();
std::cout << std::boolalpha
          << "counts=" << counts.is_int64()
          << " floats=" << floats.is_int64()
          << " absent=" << absent.is_int64() << '\n';
```

### Output

```text
counts=true floats=false absent=false
```

Equal printed magnitudes do not imply equal dtypes.

## Example 2: Measurements Stay In The Note Family

```cpp
const auto lengths = pluss::note::make("alpha").lengths_chars();
std::cout << std::boolalpha << "measurement=" << lengths.is_int64()
          << " value=" << lengths.int_item() << '\n';
```

### Output

```text
measurement=true value=5
```

## Example 3: Shape Does Not Participate

```cpp
const auto grid = pluss::note::make(
    std::vector<std::int64_t>{1, 2, 3, 4},
    std::array<std::int64_t, 2>{2, 2});
std::cout << std::boolalpha << grid.is_int64()
          << " dim=" << grid.dim() << '\n';
```

### Output

```text
true dim=2
```

## See Also

- [`Note::dtype`](dtype.md): retrieve the full dtype enum.
- `Note::integers()`: borrow all Int64 lanes without copying.
- [`Note::is_float64`](is_float64.md): distinguish continuous numeric storage.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_float64.html"><span class="label">Previous</span><span class="title">is_float64</span></a>
  <a class="next-link" href="is_lower.html"><span class="label">Next</span><span class="title">is_lower</span></a>
</div>
