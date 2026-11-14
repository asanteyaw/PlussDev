<!-- pluss-hand-authored -->

# Series operators

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::operator~() const
Series Series::operator&(const Series& other) const
Series Series::operator|(const Series& other) const

Series& Series::operator&=(const Series& other)
Series& Series::operator|=(const Series& other)
```

## Bitwise Symbols

The bitwise symbols are concise spellings of the corresponding named methods:

| Expression | Named equivalent | Boolean meaning | Integer meaning |
|---|---|---|---|
| `left & right` | `left.bitwise_and(right)` | both conditions are true | retain bits set in both operands |
| `left \| right` | `left.bitwise_or(right)` | either condition is true | retain bits set in either operand |
| `~value` | `value.bitwise_not()` | invert each truth value | complement every stored bit |

All three operators accept Boolean and integral Series. Floating-point and
complex Series are rejected instead of being silently converted to integers.
Normal Series broadcasting applies to the binary forms.

`&` and `|` are eager elementwise operators. They do not short-circuit like
the scalar C++ operators `&&` and `||`; every lane is evaluated and returned
in a new Series. Parenthesize comparisons before composing them because C++
operator precedence still applies:

```cpp
const pluss::Series selected = (price > 10.0) & (price < 20.0);
```

## Boolean Masks

Example 1. Compose two conditions and invert one mask. For Boolean inputs,
`&`, `|`, and `~` provide the compact mask algebra used by filtering and
selection code.

```cpp
#include <pluss/pluss.h>

#include <iostream>

int main() {
  const pluss::Series left =
      pluss::make_series({true, true, false, false});
  const pluss::Series right =
      pluss::make_series({true, false, true, false});

  std::cout << "and = " << (left & right) << "\n\n";
  std::cout << "or = " << (left | right) << "\n\n";
  std::cout << "not = " << (~left) << '\n';
}
```

### Output

```text
and = pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, false, false]

or = pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, false]

not = pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, false, true, true]
```

For a Boolean Series, `~mask`, `mask.bitwise_not()`, and
`mask.logical_not()` produce the same values. `~` delegates specifically to
`bitwise_not()`; the equivalence follows from the Boolean dtype rather than
from a second dispatch rule.

## Integer Bits

Example 2. The same symbols retain their ordinary bitwise meaning for integer
Series. They do not convert integers into truth values.

```cpp
#include <pluss/pluss.h>

#include <iostream>

int main() {
  const pluss::Series left = pluss::make_series({6, 5});
  const pluss::Series right = pluss::make_series({3, 10});

  std::cout << "and = " << (left & right) << "\n\n";
  std::cout << "or = " << (left | right) << "\n\n";
  std::cout << "not = " << (~left) << '\n';
}
```

### Output

```text
and = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[2, 0]

or = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[7, 15]

not = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[-7, -6]
```

The negative complement values follow the standard two's-complement identity
`~x == -x - 1`.

## Bitwise Not Versus Logical Not

Example 3. `~` and `logical_not()` intentionally diverge for non-Boolean
integers. The former preserves the integer dtype and flips bits; the latter
converts each value to truth and returns a Boolean inverse.

```cpp
#include <pluss/pluss.h>

#include <iostream>

int main() {
  const pluss::Series values = pluss::make_series({2, 0});

  std::cout << "bitwise = " << (~values) << "\n\n";
  std::cout << "logical = " << values.logical_not() << '\n';
}
```

### Output

```text
bitwise = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[-3, -1]

logical = pluss::Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]
```

Use `~` when the input is already a Boolean mask or when integer bit patterns
are the subject of the operation. Use `logical_not()` when numeric values must
first be interpreted as false for zero and true for nonzero.

## Related Pages

- [`bitwise_and`](bitwise_and.md)
- [`bitwise_or`](bitwise_or.md)
- [`bitwise_not`](bitwise_not.md)
- [`logical_and`](logical_and.md)
- [`logical_or`](logical_or.md)
- [`logical_not`](logical_not.md)

