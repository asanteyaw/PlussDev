<!-- pluss-hand-authored -->

# Queue operators

[Back to Queue API Reference](../../queue_api_reference)

## Signature

```cpp
Queue operator+() const;
Queue operator-() const;

Queue operator+(const Queue& rhs) const;
Queue operator-(const Queue& rhs) const;
Queue operator*(const Queue& rhs) const;
Queue operator/(const Queue& rhs) const;
Queue operator%(const Queue& rhs) const;

Queue operator==(const Queue& rhs) const;
Queue operator!=(const Queue& rhs) const;
Queue operator<(const Queue& rhs) const;
Queue operator<=(const Queue& rhs) const;
Queue operator>(const Queue& rhs) const;
Queue operator>=(const Queue& rhs) const;
```

## What It Does

Unary `+` is a no-op -- it returns `*this` unchanged, for every kind. Unary
`-` calls `invoke("neg")`, which only `Series` and `Duration` support;
calling it on a `Note`- or `Timeline`-holding `Queue` throws.

The binary arithmetic and comparison operators first check that both sides
have the same `kind()` -- a mismatch throws before anything else happens.
With matching kinds, `Series`, `Timeline`, and `Duration` payloads route
straight through [`invoke`](invoke.md) with the matching operation name
(`"add"`, `"sub"`, `"mul"`, `"div"`, `"mod"`, `"eq"`,
`"ne"`, `"lt"`, `"le"`, `"gt"`, `"ge"`). `Note` payloads are the exception:
only `+` is supported (text concatenation, via `Note`'s own `operator+`),
and all six comparisons are supported too (each wrapped so the result comes
back as a proper boolean `Series` rather than `Note`'s own byte-mask
representation) -- but `-`, `*`, `/`, and `%` all throw for `Note`
payloads, since none of them are meaningful operations on text.

## Parameters

**rhs** (`const Queue&`)

The right-hand operand. Must have the same `kind()` as the left-hand side.

## Returns

A new `Queue`, with the arithmetic operators returning the same kind as
both operands and the comparison operators returning a `Series`-holding
`Queue` of booleans in every case (including when both operands are
`Note`).

## Errors

Throws when the two operands have different kinds, or when the operation
isn't supported for the operands' shared kind (any of `-`, `*`, `/`, `%` on
`Note`; unary `-` on `Note` or `Timeline`).

## Examples

Example 1. Unary `+`/`-` and the binary arithmetic and comparison operators
on two `Series`-holding `Queue`s.

```cpp
pluss::Queue a(pluss::make_series({1.0, 2.0, 3.0}));
pluss::Queue b(pluss::make_series({10.0, 20.0, 30.0}));
std::cout << "+a (unary, no-op): " << pluss::series::print((+a).series()) << '\n';
std::cout << "-a (unary neg): " << pluss::series::print((-a).series()) << '\n';
std::cout << "a + b: " << pluss::series::print((a + b).series()) << '\n';
std::cout << "a * b: " << pluss::series::print((a * b).series()) << '\n';
std::cout << "a < b: " << pluss::series::print((a < b).series()) << '\n';
std::cout << "a == a: " << pluss::series::print((a == a).series()) << '\n';
```

### Output

```text
+a (unary, no-op): Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 2.0000, 3.0000]
-a (unary neg): Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.0000, -2.0000, -3.0000]
a + b: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[11.0000, 22.0000, 33.0000]
a * b: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10.0000, 40.0000, 90.0000]
a < b: Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, true, true]
a == a: Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, true, true]
```

Example 2. `Note` payloads: `+` concatenates text and succeeds; `*` is not
a meaningful text operation and throws.

```cpp
pluss::Queue note_a(pluss::make_note({"x"}));
pluss::Queue note_b(pluss::make_note({"y"}));
pluss::Queue note_sum = note_a + note_b;
std::cout << "note + note: " << note_sum.note() << '\n';
try {
  pluss::Queue bad = note_a * note_b;
  std::cout << "note * note: did not throw\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << "note * note threw: " << msg.substr(0, msg.find('\n')) << '\n';
}
```

### Output

```text
note + note: Note(shape=[1], dim=1, numel=1, encoding=utf8)
["xy"]
note * note threw: Queue::operator* failed: Queue::operator*: unsupported for Note payloads
```

`"x" + "y"` concatenates to `"xy"`, the same as calling `Note`'s own
`operator+` directly.

## See Also

- [`invoke`](invoke.md): what every operator except `Note`'s `+` and the
  comparisons forwards to.
- [`where`](where.md): the one `Series`/`Timeline`/`Duration` operation
  that is a static method instead of an operator.
