<!-- pluss-hand-authored -->

# Duration::to_note_iso

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Note Duration::to_note_iso() const;
```

## ISO Duration Projection

`to_note_iso()` converts each elapsed value to ISO duration text: `P` starts
the duration, `D` carries whole days, `T` introduces time fields, and `H`, `M`,
and `S` label those fields. Zero is `PT0S`; a negative duration has one leading
minus sign, such as `-PT0.000000001S`.

Zero-valued fields are omitted unless needed to express zero itself. Fractional
seconds retain every significant nanosecond digit and discard only trailing
zeros. The conversion is lossless for finite `Duration` lanes at nanosecond
precision. `NaT` is emitted as `"NaT"`, since absence is not an ISO duration.

The result is an owning UTF-8 `Note` with the same shape and lane order as the
receiver. This method differs semantically from `to_note()`; it is not an alias
for the fixed-field display form.

## Examples

Example 1. Composite, negative, zero, and absent values.

```cpp
auto values = pluss::duration::nanos(
    {93'784'005'006'007LL, -1, 0, pluss::Duration::kNaT});
std::cout << values.to_note_iso();
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["P1DT2H3M4.005006007S", "-PT0.000000001S", "PT0S", "NaT"]
```

Example 2. Exact whole units omit redundant fields.

```cpp
auto matrix = pluss::duration::nanos(
    {0, -1, 1'000'000'000, 86'400'000'000'000LL}, {2,2});
std::cout << matrix.to_note_iso();
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["PT0S", "-PT0.000000001S"],
  ["PT1S", "P1D"]
]
```

Example 3. Fractional precision contracts only on the right.

```cpp
auto values = pluss::duration::nanos({1'500'000'000, 1'000'000'001});
auto text = values.to_note_iso(); // ["PT1.5S", "PT1.000000001S"]
```

## See Also

- [`to_note()`](to_note.md): aligned day-and-clock display text.
- [`host()`](host.md): exact numeric nanoseconds at an explicit host boundary.
