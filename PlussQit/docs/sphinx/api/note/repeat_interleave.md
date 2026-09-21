<!-- pluss-hand-authored -->
# Note::repeat_interleave

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::repeat_interleave(std::int64_t repeats, std::int64_t dim) const
```

## Repeat Each Lane Along One Axis

`repeat_interleave(repeats, dim)` duplicates each slice at `dim` consecutively
before advancing to the next slice. If a vector is `{"a", "b"}`, a repeat
count of three produces `{"a", "a", "a", "b", "b", "b"}`.

This differs from [`repeat`](repeat.md), which copies the complete axis pattern
and would produce `{"a", "b", "a", "b", "a", "b"}`. Choose interleaving when
each observation needs a fixed number of adjacent copies; choose `repeat` when
the whole block needs to recur.

## Parameters And Shape

| Parameter | Meaning |
|---|---|
| `repeats` | Nonnegative number of consecutive copies made for every slice. |
| `dim` | Axis whose slices are expanded; negative axes normalize from the end. |

For a non-scalar receiver, rank is preserved and only one extent changes:

```text
output_shape[dim] = input_shape[dim] * repeats
```

All other extents remain unchanged. `dim` must identify an existing axis. For
a scalar, `0` and `-1` identify its conceptual single lane; the result is a
rank-one Note of shape `[repeats]`. Other scalar axes reject.

Zero is valid and changes the selected extent to zero. A negative count, an
invalid axis, or a shape product that overflows signed 64-bit cardinality
rejects before allocation. Undefined input propagates as undefined.

The method accepts one uniform count for the complete axis. It does not accept
a separate count per lane. When lane-specific counts are required, construct
the desired Int64 positions and use [`take`](take.md).

## Storage And Cost

The result preserves Text, Bytes, Bool, Int64, or Float64 dtype. Text and Bytes
allocate repeated span metadata while sharing the original packed byte arena;
native dtypes allocate aligned contiguous output lanes. The receiver is never
mutated.

Source positions are calculated while destination lanes are written, avoiding
an output-sized temporary index vector. Runtime and destination metadata are
`O(output.numel())`.

## Example 1: Interleave A Vector

```cpp
const auto values = pluss::note::make({"a", "b"});
std::cout << pluss::note::print(values.repeat_interleave(3, 0)) << '\n';
```

### Output

```text
Note(shape=[6], dim=1, numel=6, encoding=utf8)
["a", "a", "a", "b", "b", "b"]
```

## Example 2: Repeat Complete Rows Consecutively

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.repeat_interleave(2, 0)) << '\n';
```

### Output

```text
Note(shape=[4, 2], dim=2, numel=8, encoding=utf8)
[
  ["a", "b"], 
  ["a", "b"], 
  ["c", "d"], 
  ["c", "d"]
]
```

Each row is one axis-zero slice, so each complete row appears twice before the
next input row.

## Example 3: Repeat Columns With A Negative Axis

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.repeat_interleave(2, -1)) << '\n';
```

### Output

```text
Note(shape=[2, 4], dim=2, numel=8, encoding=utf8)
[
  ["a", "a", "b", "b"], 
  ["c", "c", "d", "d"]
]
```

For a matrix, `-1` is axis one, the final axis.

## Example 4: Promote A Scalar Along Either Valid Axis Spelling

```cpp
const auto scalar = pluss::note::make("x");
std::cout << "zero\n"
          << pluss::note::print(scalar.repeat_interleave(3, 0)) << '\n';
std::cout << "minus one\n"
          << pluss::note::print(scalar.repeat_interleave(2, -1)) << '\n';
```

### Output

```text
zero
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["x", "x", "x"]
minus one
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["x", "x"]
```

## Example 5: Zero Empties Only The Selected Axis

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"a", "b", "c", "d"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.repeat_interleave(0, 1)) << '\n';
```

### Output

```text
Note(shape=[2, 0], dim=2, numel=0, encoding=utf8)
[]
```

## Example 6: Float64 Remains Float64

```cpp
const auto values = pluss::note::make(std::vector<double>{1.5, 2.5});
std::cout << pluss::note::print(values.repeat_interleave(2, 0)) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, dtype=Float64)
[1.5, 1.5, 2.5, 2.5]
```

## Example 7: Bytes Stay Bytes

```cpp
const auto raw = pluss::note::bytes(
    std::vector<std::string>{"A", "B"},
    std::array<std::int64_t, 1>{2});
std::cout << pluss::note::print(raw.repeat_interleave(2, 0)) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=bytes)
[b"A", b"A", b"B", b"B"]
```

## Example 8: Undefined Propagates

```cpp
std::cout << std::boolalpha
          << "defined="
          << pluss::Note::undefined().repeat_interleave(2, 0).defined()
          << '\n';
```

### Output

```text
defined=false
```

## Example 9: Counts, Axes, And Cardinality Are Checked

```cpp
#include <limits>

const auto vector = pluss::note::make({"a", "b"});
const auto matrix = vector.repeat({2, 1});

try { (void)vector.repeat_interleave(-1, 0); }
catch (const pluss::Error&) { std::cout << "negative: rejected\n"; }
try { (void)matrix.repeat_interleave(2, 2); }
catch (const pluss::Error&) { std::cout << "axis: rejected\n"; }
try { (void)pluss::note::make("x").repeat_interleave(2, 1); }
catch (const pluss::Error&) { std::cout << "scalar axis: rejected\n"; }
try {
  (void)vector.repeat_interleave(
      std::numeric_limits<std::int64_t>::max(), 0);
} catch (const pluss::Error&) { std::cout << "overflow: rejected\n"; }
```

### Output

```text
negative: rejected
axis: rejected
scalar axis: rejected
overflow: rejected
```

## See Also

- [`repeat`](repeat.md): repeat complete patterns along one or more axes.
- [`take`](take.md): gather arbitrary positions, including lane-specific
  repetition patterns.
- [`operators`](operators.md): repeat bytes inside each cell with `operator*`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="repeat.html"><span class="label">Previous</span><span class="title">repeat</span></a>
  <a class="next-link" href="replace.html"><span class="label">Next</span><span class="title">replace</span></a>
</div>
