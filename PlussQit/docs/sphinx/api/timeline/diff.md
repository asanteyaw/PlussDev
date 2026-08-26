# Timeline::diff

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::diff(const Timeline& other) -> Duration
```

## What It Does

Computes temporal distance by subtracting `other` from `self` and returning a
`Duration`.

This is the right surface when your question is not "what are the shifted
timestamps?" but instead:

- how far apart are these instants?
- how many seconds early or late is one event stream versus another?
- what signed lag exists between two aligned temporal payloads?

The result is signed:

- positive values mean `self` is later than `other`
- negative values mean `self` is earlier than `other`

## Parameters

**other**

The timeline to subtract from `self`.

The method follows the same elementwise/broadcasting mental model as the rest
of the temporal arithmetic family. In the common case, you either compare:

- one scalar to one scalar
- one aligned 1D timeline to another aligned 1D timeline

## Returns

`Duration`
    A duration payload describing the signed temporal differences.

## Examples

Example 1. Compute the distance between two scalar instants.

```cpp
auto dt = scalar_later.diff(scalar);
std::cout << pluss::print_note(
    pluss::codeh::_compat::invoke<pluss::Note>("to_note_iso", dt)) << "\n";
std::cout << pluss::series::print(
    pluss::codeh::_compat::invoke<pluss::Series>("total", dt, {"s"})) << "\n";
```

### Output

Compact duration note view:

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"0 01:30:00"
```

Total seconds view:

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5400.0000
```

Example 2. Compute elementwise signed distances across a vector.

```cpp
auto dt = base.diff(other);
std::cout << pluss::print_note(
    pluss::codeh::_compat::invoke<pluss::Note>("to_note_iso", dt)) << "\n";
std::cout << pluss::series::print(
    pluss::codeh::_compat::invoke<pluss::Series>("total", dt, {"s"})) << "\n";
```

### Output

Compact duration note view:

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["0 00:00:00", "-0 00:14:30", "0 03:59:59", "-1 00:00:00"]
```

Total seconds view:

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, -870.0000, 14399.0000, -86400.0000]
```

## Operator Sugar Is the Named Method, Not a Separate Path

`a - b` and `a.diff(b)` compute identically:

```cpp
const auto t0 = pluss::timeline::make("1999-04-17T21:15:23Z");
const auto t1 = pluss::timeline::make("1999-04-18T00:15:23Z"); // t0 + 3h
const auto diff_operator = t1 - t0;
const auto diff_method = t1.diff(t0);
std::cout << "dim=" << diff_operator.dim() << " numel=" << diff_operator.numel() << "\n";
std::cout << pluss::series::print(diff_operator.total().seconds()) << "\n";
std::cout << pluss::series::print(diff_operator.eq(diff_method)) << "\n";
```

### Output

```text
dim=0 numel=1
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=10800.0000

Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

Same broadcasting rules as [`add_duration`](add_duration.md) apply here too
(see that page's "Broadcasting" section): `diff`/`operator-` align shapes via
the same `SeriesIterator::broadcast_shape` model any two `Series` operands
use, so a scalar `Timeline` can be diffed against an N-D one. There is no
`Duration - Timeline` form and no compound assignment -- `diff` always
produces a fresh `Duration`.

## Notes

- `diff()` returns a `Duration`, not a `Timeline`.
- The sign is meaningful. In the verified vector example, `-870` means the
  corresponding `base` timestamp is 14 minutes 30 seconds earlier than
  `other`.
- `NaT` in either operand propagates to `Duration::kNaT` in the result, the
  same as `add_duration`/`sub_duration`.
- If what you actually want is to move timestamps forward or backward, use
  [`add_duration`](add_duration.md) or [`sub_duration`](sub_duration.md)
  instead.
- Operator sugar also exists: `a - b` for timeline-minus-timeline yielding a
  duration, identical to `diff`.

## See Also

- [`Timeline::add_duration`](add_duration.md)
- [`Timeline::sub_duration`](sub_duration.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sub_duration.html">
    <span class="label">Previous</span>
    <span class="title">sub_duration</span>
  </a>
  <a class="next-link" href="eq.html">
    <span class="label">Next</span>
    <span class="title">eq</span>
  </a>
</div>
