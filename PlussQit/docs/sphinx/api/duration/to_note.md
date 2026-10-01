<!-- pluss-hand-authored -->

# Duration::to_note

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Note Duration::to_note() const;
```

## Human-Readable Duration Text

`to_note()` converts every lane to fixed-field text of the form
`D days HH:MM:SS.nnnnnnnnn`. It is intended for tabular display, labels, and
text processing where aligned fields and explicit nanosecond precision are
more useful than a compact interchange notation.

Negative spans use floor-normalized fields. Therefore `-1 ns` is represented
as `-1 days 23:59:59.999999999`, not as a separate minus sign applied to every
field. This is the same canonical decomposition exposed by `days()`,
`hours()`, and the remaining component methods. `NaT` becomes the text lane
`"NaT"`.

The result is a newly allocated UTF-8 `Note` with exactly the receiver's rank,
shape, and lane order. It is text, not a borrowed view and not a reversible
change to the receiver. Use `to_note_iso()` when another system expects ISO
duration syntax instead.

## Examples

Example 1. Positive, negative, zero, and absent values show the complete text
contract.

```cpp
auto values = pluss::duration::nanos(
    {93'784'005'006'007LL, -1, 0, pluss::Duration::kNaT});
std::cout << values.to_note();
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["1 days 02:03:04.005006007", "-1 days 23:59:59.999999999", "0 days 00:00:00.000000000", "NaT"]
```

Example 2. N-D structure survives conversion.

```cpp
auto matrix = pluss::duration::nanos(
    {0, -1, 1'000'000'000, 86'400'000'000'000LL}, {2,2});
std::cout << matrix.to_note();
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["0 days 00:00:00.000000000", "-1 days 23:59:59.999999999"],
  ["0 days 00:00:01.000000000", "1 days 00:00:00.000000000"]
]
```

Example 3. Scalar input produces a scalar `Note`, not a one-element vector.

```cpp
auto label = pluss::duration::minutes(90).to_note();
// label.dim() == 0, label.numel() == 1
```

## See Also

- [`to_note_iso()`](to_note_iso.md): compact standards-oriented text.
- [`operators`](operators.md): direct stream rendering without constructing a `Note`.
