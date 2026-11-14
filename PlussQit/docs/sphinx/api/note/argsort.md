<!-- pluss-hand-authored -->
# Note::argsort

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::argsort(bool descending = false, bool stable = true) const
```

## Return Positions Instead Of Moving Values

`argsort()` returns the linear positions that would arrange a scalar or vector
`Note` in value order. The result is always a rank-one Int64 `Note`: applying
it with [`take`](take.md) performs the actual reordering while preserving the
receiver's dtype.

Use `argsort()` when positions matter independently of values: carrying the
same permutation into another aligned object, retaining an original-to-sorted
mapping, or inspecting where equal values came from. Use [`sort`](sort.md)
when only the reordered Note is needed. `sort(dim)` supports matrix and
rank-three lanes; this `argsort()` overload intentionally accepts only rank
zero and rank one because it returns one unambiguous linear permutation and
has no axis parameter.

## Parameters And Ordering

| Parameter | Meaning | Default |
|---|---|---|
| `descending` | Reverse the value order. | `false` |
| `stable` | Preserve original position order among equivalent values. | `true` |

Ordering depends on the homogeneous receiver dtype:

| Dtype | Ascending order |
|---|---|
| Text | exact unsigned UTF-8 byte sequence; no collation, case folding, or normalization |
| Bytes | exact unsigned byte sequence, including embedded zero bytes |
| Bool | `false` before `true` |
| Int64 | signed integer order |
| Float64 | numeric values in ascending order, followed by NaNs |

Descending reverses the value classes, so Float64 NaNs precede numeric values.
NaNs are equivalent to one another for ordering; with `stable=true`, multiple
NaN positions retain their input order. Signed zeros compare as equivalent.

With `stable=false`, equal values may appear in any relative order. The output
is still a valid permutation, but code must not rely on tie positions. This
mode can avoid stability bookkeeping in the sorting implementation.

The receiver is never copied or rearranged. The method allocates `numel()`
contiguous Int64 positions and sorts those positions while reading receiver
lanes. Its memory cost is `O(n)` and comparison cost is `O(n log n)` for `n`
values. Text and Bytes comparisons borrow the packed arena; they do not copy
cell payloads.

## Example 1: Stable Ascending Positions

```cpp
const auto words = pluss::note::make({"b", "a", "b", ""});
std::cout << pluss::note::print(words.argsort()) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, dtype=Int64)
[3, 1, 0, 2]
```

The empty string is first, then `"a"`. The equal `"b"` values retain positions
`0, 2` because stability is enabled by default.

## Example 2: Apply The Permutation Without Leaving Note

```cpp
const auto words = pluss::note::make({"b", "a", "b", ""});
const auto order = words.argsort();
std::cout << pluss::note::print(words.take(order)) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["", "a", "b", "b"]
```

`order` is an Int64 Note and `take()` accepts it directly. No standard-library
index vector or numeric `Series` is required.

## Example 3: Descending Stable Order

```cpp
const auto words = pluss::note::make({"b", "a", "b", ""});
std::cout << pluss::note::print(words.argsort(true, true)) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, dtype=Int64)
[0, 2, 1, 3]
```

The duplicate positions remain `0, 2`; only value-class order reverses.

## Example 4: Request Unstable Sorting When Ties Do Not Exist

```cpp
const auto words = pluss::note::make({"delta", "alpha", "charlie", "bravo"});
std::cout << pluss::note::print(words.argsort(false, false)) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, dtype=Int64)
[1, 3, 2, 0]
```

With unique values, stability cannot affect the observable permutation.

## Example 5: Raw Bytes Use Unsigned Byte Order

```cpp
const auto raw = pluss::note::bytes(
    std::vector<std::string>{"\xFF", "\x01", "\x7F"},
    std::array<std::int64_t, 1>{3});
std::cout << pluss::note::print(raw.argsort()) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, dtype=Int64)
[1, 2, 0]
```

The leading bytes are `0x01`, `0x7F`, and `0xFF`; no text decoding occurs.

## Example 6: Bool And Int64 Ordering

```cpp
std::cout << "Bool\n"
          << pluss::note::print(
                              pluss::note::make({true, false, true, false}).argsort())
          << '\n';
std::cout << "Int64\n"
          << pluss::note::print(
                              pluss::note::make(
                                  std::vector<std::int64_t>{10, -2, 10, 0}).argsort())
          << '\n';
```

### Output

```text
Bool
Note(shape=[4], dim=1, numel=4, dtype=Int64)
[1, 3, 0, 2]
Int64
Note(shape=[4], dim=1, numel=4, dtype=Int64)
[1, 3, 0, 2]
```

Stability keeps the two false positions and the two integer-`10` positions in
their original relative order.

## Example 7: Float64 NaN Placement

```cpp
#include <limits>

const auto values = pluss::note::make(std::vector<double>{
    2.0, std::numeric_limits<double>::quiet_NaN(), -1.0,
    std::numeric_limits<double>::quiet_NaN()});

std::cout << "ascending\n" << pluss::note::print(values.argsort()) << '\n';
std::cout << "descending\n"
          << pluss::note::print(values.argsort(true, true)) << '\n';
```

### Output

```text
ascending
Note(shape=[4], dim=1, numel=4, dtype=Int64)
[2, 0, 1, 3]
descending
Note(shape=[4], dim=1, numel=4, dtype=Int64)
[1, 3, 0, 2]
```

Ascending places `-1`, then `2`, then both NaNs. Descending places the stable
NaN pair first, followed by `2` and `-1`.

## Example 8: Scalar And Empty Results Are Still Vectors

```cpp
std::cout << "scalar\n"
          << pluss::note::print(pluss::note::make("scalar").argsort()) << '\n';
std::cout << "empty\n"
          << pluss::note::print(
                              pluss::note::make(std::vector<std::string>{}).argsort())
          << '\n';
```

### Output

```text
scalar
Note(shape=[1], dim=1, numel=1, dtype=Int64)
[0]
empty
Note(shape=[0], dim=1, numel=0, dtype=Int64)
[]
```

A scalar has one ordering position, `0`. An empty vector has the unique empty
permutation.

## Example 9: Invalid Rank And Absence Reject

```cpp
try {
  (void)pluss::note::make({"b", "a", "d", "c"})
      .reshape({2, 2}).argsort();
} catch (const pluss::Error&) { std::cout << "matrix: rejected\n"; }

try { (void)pluss::Note::undefined().argsort(); }
catch (const pluss::Error&) { std::cout << "undefined: rejected\n"; }
```

### Output

```text
matrix: rejected
undefined: rejected
```

## See Also

- [`sort`](sort.md): return values ordered along a selected axis.
- [`take`](take.md): apply an Int64 ordering Note to values.
- [`operators`](operators.md): comparison semantics shared by value ordering.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="any.html"><span class="label">Previous</span><span class="title">any</span></a>
  <a class="next-link" href="at.html"><span class="label">Next</span><span class="title">at</span></a>
</div>
