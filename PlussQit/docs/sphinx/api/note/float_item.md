<!-- pluss-hand-authored -->
# Note::float_item

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
double Note::float_item() const
```

## Extract One Float64 Lane Without Rounding It

`float_item()` copies the sole lane of a Float64 Note into a native `double`.
It is the scalar host boundary for scores and other continuous results. The
method performs no text formatting, decimal rounding, allocation, or dtype
conversion; the returned value carries the stored IEEE 754 bits into ordinary
C++ computation.

Like the other typed item accessors, it is cardinality-based. A defined
Float64 Note is accepted whenever `numel() == 1`, whether its shape is `[]`,
`[1]`, `[1, 1]`, or `[1, 1, 1]`. Rank is retained on the source and is not part
of the extraction test.

Finite values, signed zero, infinities, and NaNs pass through unchanged. A
default stream may print fewer digits than are needed to expose the stored
binary value; use `max_digits10` when a round-trippable decimal rendering
matters. NaN equality remains false under ordinary IEEE comparison, so use
`std::isnan()` to inspect a returned NaN.

Only Float64 Notes are accepted. Int64 is not promoted, Bool is not mapped to
zero or one, and text is not parsed. Defined empty Notes, multi-element Notes,
and undefined handles are also rejected. Validation occurs before extraction,
so no result is produced on failure.

Use `float_item()` once a pipeline has intentionally produced one score. Use
[`reals()`](reals.md) for a zero-copy span over multiple lanes.

## Example 1: Rank Zero And A Rank-Three Singleton

```cpp
const auto scalar = pluss::note::make(0.125);
const auto rank3 = pluss::note::make(
    std::vector<double>{1.25}, std::array<std::int64_t, 3>{1, 1, 1});

std::cout << "scalar=" << scalar.float_item()
          << " rank3=" << rank3.float_item() << '\n';
```

### Output

```text
scalar=0.125 rank3=1.25
```

## Example 2: Reveal Full Stored Precision

```cpp
const auto value = pluss::note::make(1.0 / 10.0);
std::cout << std::setprecision(std::numeric_limits<double>::max_digits10)
          << "precision=" << value.float_item() << '\n';
```

### Output

```text
precision=0.10000000000000001
```

The longer decimal is not introduced by `float_item()`; it exposes the exact
Float64 approximation that was already stored.

## Example 3: Preserve IEEE Special Values

```cpp
const auto negative_zero = pluss::note::make(-0.0).float_item();
const auto infinity = pluss::note::make(
    std::numeric_limits<double>::infinity()).float_item();
const auto nan = pluss::note::make(
    std::numeric_limits<double>::quiet_NaN()).float_item();

std::cout << std::boolalpha
          << "special=" << std::signbit(negative_zero) << ','
          << std::isinf(infinity) << ',' << std::isnan(nan) << '\n';
```

### Output

```text
special=true,true,true
```

## Example 4: Cardinality Must Be Exactly One

```cpp
auto report = [](std::string_view label, const pluss::Note& value) {
  try { (void)value.float_item(); }
  catch (const pluss::Error&) { std::cout << label << ": rejected\n"; }
};

report("float_item empty", pluss::note::make(std::vector<double>{}));
report("float_item multiple", pluss::note::make(std::vector<double>{0.25, 0.75}));
```

### Output

```text
float_item empty: rejected
float_item multiple: rejected
```

## Example 5: Int64 Is Not Implicitly Promoted

```cpp
try {
  (void)pluss::note::make(std::int64_t{1}).float_item();
} catch (const pluss::Error&) {
  std::cout << "float_item wrong dtype: rejected\n";
}
```

### Output

```text
float_item wrong dtype: rejected
```

## Example 6: Undefined Has No Float64 Storage

```cpp
try {
  (void)pluss::Note::undefined().float_item();
} catch (const pluss::Error&) {
  std::cout << "float_item undefined: rejected\n";
}
```

### Output

```text
float_item undefined: rejected
```

## See Also

- [`reals`](reals.md): borrow all native Float64 lanes.
- [`is_float64`](is_float64.md): inspect the dtype without throwing.
- [`numel`](numel.md): inspect cardinality before extraction.
- [`scalar`](scalar.md): borrow one Text or Bytes cell instead.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flatten.html"><span class="label">Previous</span><span class="title">flatten</span></a>
  <a class="next-link" href="for_each_flat.html"><span class="label">Next</span><span class="title">for_each_flat</span></a>
</div>
