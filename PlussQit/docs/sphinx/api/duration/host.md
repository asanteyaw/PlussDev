<!-- pluss-hand-authored -->

# Duration::host

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
std::vector<std::int64_t> Duration::host() const
```

## What It Extracts

`host()` copies every lane into an owning vector of signed 64-bit epoch-neutral
nanosecond counts. It is the explicit boundary for code that needs standard C++
storage, serialization input, or interoperability with an API that does not
understand `Duration`.

The result is flat and follows logical row-major order. Shape is deliberately
not embedded in the vector; retain [`sizes()`](sizes.md) separately when the
original N-D arrangement must be reconstructed. `NaT` is preserved as
`Duration::kNaT`, and values remain exact beyond the integer precision of a
floating-point representation.

Unlike `sizes()`, the returned vector owns its allocation. Mutating it cannot
change the Duration. Extraction is therefore O(`numel`) in time and memory;
prefer Duration operations when computation can remain inside Pluss.

## Examples

Example 1. Preserve exact nanoseconds, including `NaT`.

```cpp
const auto value = pluss::duration::nanos(
    {9'007'199'254'740'993LL, -1LL, pluss::Duration::kNaT});
const auto out = value.host();
std::cout << '[' << out[0] << ',' << out[1] << ',' << out[2] << "]\n";
```

### Output

```text
[9007199254740993,-1,-9223372036854775808]
```

Example 2. A matrix is flattened in logical row-major order and converted to
nanoseconds regardless of its construction unit.

```cpp
const auto value = pluss::make_duration(
    {1,2,3,4}, {2,2}, pluss::duration::Unit::Seconds);
const auto out = value.host();
std::cout << '[' << out[0] << ',' << out[1] << ','
          << out[2] << ',' << out[3] << "]\n";
```

### Output

```text
[1000000000,2000000000,3000000000,4000000000]
```

Example 3. The returned vector is independent storage.

```cpp
const auto value = pluss::duration::minutes({15,30,45});
auto out = value.host();
out[0] = 0;
std::cout << "copy_first=" << out[0]
          << " source_first=" << value.host()[0] << "\n";
```

### Output

```text
copy_first=0 source_first=900000000000
```

## Failure Modes

- The returned vector has no shape metadata; flattening is intentional.
- `NaT` remains the sentinel integer. Test with `isnat()` before handing data
  to software that may treat the minimum signed integer as an ordinary span.
- Large arrays require a full host allocation and copy.

## See Also

- [`sizes`](sizes.md): preserve N-D reconstruction metadata.
- [`total().nanoseconds()`](total_nanoseconds.md): remain in `Series` rather
  than crossing to an STL container.
- [`isnat`](isnat.md): identify sentinel lanes before extraction.
