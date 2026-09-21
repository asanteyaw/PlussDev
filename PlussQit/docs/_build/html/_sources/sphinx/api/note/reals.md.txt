<!-- pluss-hand-authored -->
# Note::reals

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::span<const double> Note::reals() const
```

## Borrow The Float64 Lanes At Full Precision

`reals()` exposes a Float64 Note's contiguous `double` lanes through a
read-only span. It performs no allocation, conversion, rounding, or text
formatting. This is the appropriate boundary when a numeric algorithm should
consume continuous Note results without first building a `std::vector<double>`.

The span preserves the stored IEEE 754 values, including signed zero,
infinities, and NaN payload bits. Printed representations may shorten decimal
digits, but `reals()` accesses the original doubles. It never converts Int64
lanes even when their values would be exactly representable.

### Shape And Lifetime

Lanes are presented in row-major logical order. Shape remains on the Note, so
pass `sizes_view()` alongside `reals()` when an external operation needs both.
A scalar has one lane and a defined empty Float64 Note has an empty span.

The span borrows memory. Keep the source Note or a storage-sharing Note view
alive until the consumer finishes. `host()` is the owning alternative. Since
the span element type is const, callers cannot mutate the Note through it.

`reals()` accepts only defined Float64 Notes. Use [`is_float64`](is_float64.md)
before calling it when the dtype is dynamic.

## Example 1: Consume Float64 Lanes Directly

```cpp
const auto scores = pluss::note::make(std::vector<double>{0.25, 0.75});
const auto lanes = scores.reals();
std::cout << '[';
for (std::size_t i = 0; i < lanes.size(); ++i)
  std::cout << (i ? ", " : "") << lanes[i];
std::cout << "] size=" << lanes.size() << '\n';
```

### Output

```text
[0.25, 0.75] size=2
```

## Example 2: Higher-Rank Data Is Row-Major

```cpp
const auto grid = pluss::note::make(
    std::vector<double>{0.5, 1.5, 2.5, 3.5},
    std::array<std::int64_t, 2>{2, 2});
const auto lanes = grid.reals();
std::cout << '[';
for (std::size_t i = 0; i < lanes.size(); ++i)
  std::cout << (i ? ", " : "") << lanes[i];
std::cout << "] shape=[" << grid.size(0) << ", " << grid.size(1) << "]\n";
```

### Output

```text
[0.5, 1.5, 2.5, 3.5] shape=[2, 2]
```

## Example 3: Empty Float64 Storage Is A Valid Empty Span

```cpp
const auto empty = pluss::note::make(
    std::vector<double>{}, std::array<std::int64_t, 1>{0});
std::cout << "empty lanes=" << empty.reals().size() << '\n';
```

### Output

```text
empty lanes=0
```

This differs from an undefined Note: the empty object has dtype Float64 and
shape `[0]`.

## Example 4: Int64 Is Rejected Rather Than Converted

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).reals();
} catch (const pluss::Error& error) {
  const std::string message(error.what());
  std::cout << message.substr(0, message.find('\n')) << '\n';
}
```

### Output

```text
Note storage does not contain Float64 lanes
```

## Example 5: Undefined Has No Floating-Point Buffer

```cpp
try {
  (void)pluss::Note::undefined().reals();
} catch (const pluss::Error& error) {
  const std::string message(error.what());
  std::cout << message.substr(0, message.find('\n')) << '\n';
}
```

### Output

```text
Note::reals() on undefined Note
```

## See Also

- [`Note::is_float64`](is_float64.md): test the dtype without throwing.
- [`Note::host`](host.md): obtain an owning host representation.
- `Note::float_item()`: extract the sole lane from a one-element Float64 Note.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quote.html"><span class="label">Previous</span><span class="title">quote</span></a>
  <a class="next-link" href="regex.html"><span class="label">Next</span><span class="title">regex</span></a>
</div>
