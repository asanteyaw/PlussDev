<!-- pluss-hand-authored -->
# Note::int_item

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::int64_t Note::int_item() const
```

## Extract One Exact Int64 Lane

`int_item()` copies the only lane of an Int64 Note into a native
`std::int64_t`. It is designed for scalar measurements, counts, positions, or
configuration values that have completed their vectorized work and must enter
ordinary C++ code.

Acceptance depends on the number of elements, not the number of axes. Every
defined Int64 Note with `numel() == 1` is valid, including shapes `[]`, `[1]`,
`[1, 1]`, and `[1, 1, 1]`. The method does not squeeze or otherwise change the
source geometry; it only copies its one value.

The complete signed 64-bit range is preserved exactly. There is no formatting,
allocation, narrowing, or borrowed lifetime. The returned integer remains
valid after the Note is destroyed.

The dtype contract is strict. Bool and Float64 values are not promoted or
converted, and numeric-looking text is not parsed. Use the operation that
deliberately constructs or computes an Int64 Note first. Empty, multi-element,
wrong-dtype, and undefined inputs throw before returning a partial result.

Use `int_item()` for one final value. Use [`integers()`](integers.md) when an
algorithm needs a contiguous view of every lane, and keep the result as a Note
when further shape-preserving operations remain.

## Example 1: Rank Zero And A One-Cell Matrix

```cpp
const auto scalar = pluss::note::make(std::int64_t{7});
const auto matrix = pluss::note::make(
    std::vector<std::int64_t>{-42}, std::array<std::int64_t, 2>{1, 1});

std::cout << "scalar=" << scalar.int_item()
          << " rank2=" << matrix.int_item() << '\n';
```

### Output

```text
scalar=7 rank2=-42
```

## Example 2: Extract A Scalar Measurement

```cpp
const auto count = pluss::note::make("café").lengths_chars();
std::cout << "measurement=" << count.int_item() << '\n';
```

### Output

```text
measurement=4
```

The character measurement returns an Int64 Note first. This preserves the
closed-type pipeline until the host integer is actually required.

## Example 3: Preserve Both Signed Limits

```cpp
const auto low = pluss::note::make(std::numeric_limits<std::int64_t>::min());
const auto high = pluss::note::make(std::numeric_limits<std::int64_t>::max());

std::cout << "limits=" << low.int_item() << ',' << high.int_item() << '\n';
```

### Output

```text
limits=-9223372036854775808,9223372036854775807
```

## Example 4: Cardinality Must Be Exactly One

```cpp
auto report = [](std::string_view label, const pluss::Note& value) {
  try { (void)value.int_item(); }
  catch (const pluss::Error&) { std::cout << label << ": rejected\n"; }
};

report("int_item empty", pluss::note::make(std::vector<std::int64_t>{}));
report("int_item multiple", pluss::note::make(std::vector<std::int64_t>{2, 5}));
```

### Output

```text
int_item empty: rejected
int_item multiple: rejected
```

## Example 5: Float64 Is Not Implicitly Truncated

```cpp
try {
  (void)pluss::note::make(1.0).int_item();
} catch (const pluss::Error&) {
  std::cout << "int_item wrong dtype: rejected\n";
}
```

### Output

```text
int_item wrong dtype: rejected
```

## Example 6: Undefined Has No Integer Storage

```cpp
try {
  (void)pluss::Note::undefined().int_item();
} catch (const pluss::Error&) {
  std::cout << "int_item undefined: rejected\n";
}
```

### Output

```text
int_item undefined: rejected
```

## See Also

- [`integers`](integers.md): borrow all native Int64 lanes.
- [`is_int64`](is_int64.md): inspect the dtype without throwing.
- [`numel`](numel.md): inspect cardinality before extraction.
- [`scalar`](scalar.md): borrow one Text or Bytes cell instead.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="indent.html"><span class="label">Previous</span><span class="title">indent</span></a>
  <a class="next-link" href="integers.html"><span class="label">Next</span><span class="title">integers</span></a>
</div>
